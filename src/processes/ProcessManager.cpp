#include "processes/ProcessManager.h"

#include <algorithm>
#include <sstream>

namespace miniso {

ProcessManager::ProcessManager(EventSystem& events, int firstUserPid)
    : events_(events), nextUserPid_(firstUserPid) {}

ValueResult<ProcessControlBlock> ProcessManager::create(const CreateProcessRequest& request) {
    std::lock_guard<std::mutex> lock(mutex_);
    ProcessControlBlock pcb;
    pcb.pid = request.forcedPid > 0 ? request.forcedPid : nextUserPid_++;
    if (processes_.count(pcb.pid) != 0) {
        return ValueResult<ProcessControlBlock>::failure("PID already in use");
    }
    if (!request.systemProcess && request.parentPid != 0 && processes_.count(request.parentPid) == 0) {
        return ValueResult<ProcessControlBlock>::failure("Parent PID does not exist");
    }
    pcb.parentPid = request.parentPid;
    pcb.name = request.name.empty() ? "unnamed" : request.name;
    pcb.state = ProcessState::New;
    pcb.priority = request.priority;
    pcb.logicalMemorySize = request.logicalMemorySize;
    pcb.logicalThreadCount = std::max(1, request.logicalThreadCount);
    pcb.remainingCpuTime = request.cpuBurst;
    pcb.creationTime = std::chrono::system_clock::now();
    pcb.systemProcess = request.systemProcess;
    pcb.applicationId = request.applicationId;

    processes_[pcb.pid] = pcb;
    historyOrder_.push_back(pcb.pid);

    if (pcb.parentPid != 0 && processes_.count(pcb.parentPid) != 0) {
        auto& parent = processes_[pcb.parentPid];
        if (std::find(parent.childPids.begin(), parent.childPids.end(), pcb.pid) ==
            parent.childPids.end()) {
            parent.childPids.push_back(pcb.pid);
        }
    }

    events_.publish(EventType::ProcessCreated, "ProcessManager",
                    "Created process " + pcb.name, pcb.pid);
    return ValueResult<ProcessControlBlock>::success(pcb);
}

OperationResult ProcessManager::setState(int pid, ProcessState state, const std::string& reason) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = processes_.find(pid);
    if (it == processes_.end()) {
        return OperationResult::failure("Unknown PID");
    }
    if (!isValidStateTransition(it->second.state, state)) {
        return OperationResult::failure("Invalid transition from " +
                                        processStateName(it->second.state) + " to " +
                                        processStateName(state));
    }
    const auto previous = it->second.state;
    it->second.state = state;
    if (state == ProcessState::Blocked) {
        it->second.blockingReason = reason;
    } else if (state != ProcessState::Terminated) {
        it->second.blockingReason.clear();
    }
    events_.publish(EventType::ProcessStateChanged, "ProcessManager",
                    processStateName(previous) + " -> " + processStateName(state) +
                        (reason.empty() ? "" : " (" + reason + ")"),
                    pid);
    return OperationResult::success();
}

OperationResult ProcessManager::addChild(int parentPid, int childPid) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto parent = processes_.find(parentPid);
    auto child = processes_.find(childPid);
    if (parent == processes_.end() || child == processes_.end()) {
        return OperationResult::failure("Unknown parent or child PID");
    }
    child->second.parentPid = parentPid;
    if (std::find(parent->second.childPids.begin(), parent->second.childPids.end(), childPid) ==
        parent->second.childPids.end()) {
        parent->second.childPids.push_back(childPid);
    }
    return OperationResult::success();
}

OperationResult ProcessManager::removeChild(int parentPid, int childPid) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto parent = processes_.find(parentPid);
    if (parent == processes_.end()) {
        return OperationResult::failure("Unknown parent PID");
    }
    auto& children = parent->second.childPids;
    children.erase(std::remove(children.begin(), children.end(), childPid), children.end());
    return OperationResult::success();
}

OperationResult ProcessManager::markTerminated(int pid, int exitCode, const std::string& reason) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = processes_.find(pid);
    if (it == processes_.end()) {
        return OperationResult::failure("Unknown PID");
    }
    if (it->second.state == ProcessState::Terminated) {
        return OperationResult::success("Already terminated");
    }
    if (!isValidStateTransition(it->second.state, ProcessState::Terminated)) {
        return OperationResult::failure("Cannot terminate from current state");
    }
    it->second.state = ProcessState::Terminated;
    it->second.exitCode = exitCode;
    it->second.terminationReason = reason;
    it->second.terminationTime = std::chrono::system_clock::now();
    it->second.remainingCpuTime = 0;
    it->second.quantumRemaining = 0;
    events_.publish(EventType::ProcessTerminated, "ProcessManager",
                    "Terminated: " + reason, pid);
    return OperationResult::success();
}

OperationResult ProcessManager::updateMemoryStats(int pid, std::size_t resident, int pages, int frames) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = processes_.find(pid);
    if (it == processes_.end()) {
        return OperationResult::failure("Unknown PID");
    }
    it->second.residentMemorySize = resident;
    it->second.requiredPages = pages;
    it->second.assignedFrames = frames;
    return OperationResult::success();
}

OperationResult ProcessManager::setPartition(int pid, std::optional<std::size_t> start,
                                             std::optional<std::size_t> size) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = processes_.find(pid);
    if (it == processes_.end()) {
        return OperationResult::failure("Unknown PID");
    }
    it->second.partitionStart = start;
    it->second.partitionSize = size;
    return OperationResult::success();
}

OperationResult ProcessManager::consumeCpu(int pid, std::uint64_t units) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = processes_.find(pid);
    if (it == processes_.end()) {
        return OperationResult::failure("Unknown PID");
    }
    const auto used = std::min(units, it->second.remainingCpuTime);
    it->second.totalCpuTime += used;
    it->second.remainingCpuTime -= used;
    if (it->second.quantumRemaining >= used) {
        it->second.quantumRemaining -= used;
    } else {
        it->second.quantumRemaining = 0;
    }
    return OperationResult::success();
}

OperationResult ProcessManager::resetQuantum(int pid, std::uint64_t quantum) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = processes_.find(pid);
    if (it == processes_.end()) {
        return OperationResult::failure("Unknown PID");
    }
    it->second.quantumRemaining = quantum;
    return OperationResult::success();
}

std::optional<ProcessControlBlock> ProcessManager::get(int pid) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = processes_.find(pid);
    if (it == processes_.end()) {
        return std::nullopt;
    }
    return it->second;
}

std::vector<ProcessControlBlock> ProcessManager::listAlive() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<ProcessControlBlock> result;
    for (const auto& [pid, pcb] : processes_) {
        if (pcb.state != ProcessState::Terminated) {
            result.push_back(pcb);
        }
    }
    return result;
}

std::vector<ProcessControlBlock> ProcessManager::listAll() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<ProcessControlBlock> result;
    for (const auto& [pid, pcb] : processes_) {
        result.push_back(pcb);
    }
    return result;
}

std::vector<ProcessControlBlock> ProcessManager::history() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<ProcessControlBlock> result;
    for (int pid : historyOrder_) {
        result.push_back(processes_.at(pid));
    }
    return result;
}

std::string ProcessManager::treeText(int rootPid) const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (processes_.count(rootPid) == 0) {
        return "(empty)\n";
    }
    return treeNode(rootPid, "", true);
}

std::string ProcessManager::treeNode(int pid, const std::string& prefix, bool last) const {
    const auto& pcb = processes_.at(pid);
    std::ostringstream out;
    out << prefix << (prefix.empty() ? "" : (last ? "+-- " : "|-- "))
        << "PID " << pcb.pid << " " << pcb.name << " [" << processStateName(pcb.state) << "]\n";
    const std::string childPrefix = prefix + (prefix.empty() ? "" : (last ? "    " : "|   "));
    std::vector<int> children;
    for (int child : pcb.childPids) {
        if (processes_.count(child) != 0) {
            children.push_back(child);
        }
    }
    for (std::size_t i = 0; i < children.size(); ++i) {
        out << treeNode(children[i], childPrefix, i + 1 == children.size());
    }
    return out.str();
}

bool ProcessManager::exists(int pid) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return processes_.count(pid) != 0;
}

bool ProcessManager::isSystemProcess(int pid) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = processes_.find(pid);
    return it != processes_.end() && it->second.systemProcess;
}

int ProcessManager::nextUserPid() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return nextUserPid_;
}

}  // namespace miniso
