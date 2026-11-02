#include "core/MiniSO.h"

#include <mutex>
#include <sstream>

namespace miniso {
namespace {

SystemConfig withBuiltBinaries(SystemConfig config) {
#ifdef MINISO_DEFAULT_RUNNER
    config.processRunnerPath = MINISO_DEFAULT_RUNNER;
#endif
#ifdef MINISO_DEFAULT_CHILD
    config.processChildPath = MINISO_DEFAULT_CHILD;
#endif
    return config;
}

}  // namespace

MiniSO::MiniSO(SystemConfig config)
    : config_(withBuiltBinaries(std::move(config))),
      events_(config_.eventCapacity),
      logger_(events_),
      processes_(events_, config_.firstUserPid),
      memory_(events_, config_),
      scheduler_(events_, processes_, config_.quantum),
      producerConsumer_(events_),
      renderer_(events_, memory_),
      linuxRunner_(events_, config_),
      applications_(*this) {}

void MiniSO::boot() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (booted_) {
        return;
    }
    createSystemProcesses();
    booted_ = true;
    events_.publish(EventType::Info, "MiniSO", "System boot complete");
}

void MiniSO::createSystemProcesses() {
    struct Spec {
        int pid;
        const char* name;
        std::size_t memory;
    };
    const Spec specs[] = {
        {1, "MiniKernel", config_.systemProcessMemoryBytes},
        {2, "MemoryManager", config_.systemProcessMemoryBytes},
        {3, "ProcessScheduler", config_.systemProcessMemoryBytes},
        {4, "EventLogger", config_.systemProcessMemoryBytes},
        {5, "DesktopService", config_.systemProcessMemoryBytes},
        {6, "ThreadMonitor", config_.systemProcessMemoryBytes},
    };
    for (const auto& spec : specs) {
        CreateProcessRequest request;
        request.name = spec.name;
        request.logicalMemorySize = spec.memory;
        request.priority = 10;
        request.cpuBurst = 0;
        request.parentPid = spec.pid == 1 ? 0 : 1;
        request.systemProcess = true;
        request.forcedPid = spec.pid;
        auto created = processes_.create(request);
        if (!created.ok) {
            continue;
        }
        auto mem = memory_.allocateProcessMemory(spec.pid, spec.memory);
        if (mem.ok) {
            syncProcessMemory(spec.pid);
        }
        processes_.setState(spec.pid, ProcessState::Ready, "system service");
    }
}

void MiniSO::syncProcessMemory(int pid) {
    auto table = memory_.pageTable(pid);
    int frames = 0;
    int pages = 0;
    std::size_t resident = 0;
    if (table) {
        pages = static_cast<int>(table->entries.size());
        for (const auto& entry : table->entries) {
            if (entry.present) {
                ++frames;
                resident += table->pageSize;
            }
        }
    }
    processes_.updateMemoryStats(pid, resident, pages, frames);
    auto partition = memory_.partitions().findByProcess(pid);
    if (partition) {
        processes_.setPartition(pid, partition->startAddress, partition->size);
    } else {
        processes_.setPartition(pid, std::nullopt, std::nullopt);
    }
}

OperationResult MiniSO::admitProcess(int pid) {
    auto ready = processes_.setState(pid, ProcessState::Ready, "admitted");
    if (!ready.ok) {
        return ready;
    }
    auto pcb = processes_.get(pid);
    if (pcb && !pcb->systemProcess && pcb->remainingCpuTime > 0) {
        scheduler_.enqueue(pid);
    }
    return OperationResult::success();
}

void MiniSO::shutdown() {
    stopScheduler();
    producerConsumer_.stop();
    auto alive = processes_.listAlive();
    for (const auto& pcb : alive) {
        if (!pcb.systemProcess) {
            terminateProcess(pcb.pid, true, "shutdown");
        }
    }
    for (int pid = 6; pid >= 1; --pid) {
        terminateProcess(pid, false, "shutdown");
    }
    events_.publish(EventType::Info, "MiniSO", "System shutdown complete");
}

ValueResult<ProcessControlBlock> MiniSO::createProcess(const CreateProcessRequest& request) {
    std::unique_lock<std::mutex> lock(mutex_);
    auto created = processes_.create(request);
    if (!created.ok) {
        return created;
    }
    const int pid = created.value.pid;
    auto mem = memory_.allocateProcessMemory(pid, request.logicalMemorySize);
    if (!mem.ok) {
        auto decision = memory_.chooseVictim(processes_.listAlive());
        events_.publish(EventType::OOMTriggered, "OOMManager", decision.reason, decision.victimPid, -1,
                        EventLevel::Critical);
        if (decision.victimPid > 0 && decision.victimPid != pid) {
            lock.unlock();
            terminateProcess(decision.victimPid, true, "simulated OOM killer");
            lock.lock();
            mem = memory_.allocateProcessMemory(pid, request.logicalMemorySize);
        }
    }
    if (!mem.ok) {
        scheduler_.dequeue(pid);
        memory_.releaseProcessMemory(pid);
        processes_.markTerminated(pid, 1, "memory allocation failed");
        if (request.parentPid != 0) {
            processes_.removeChild(request.parentPid, pid);
        }
        return ValueResult<ProcessControlBlock>::failure(mem.message);
    }
    syncProcessMemory(pid);
    admitProcess(pid);
    auto pcb = processes_.get(pid);
    return ValueResult<ProcessControlBlock>::success(*pcb);
}

OperationResult MiniSO::terminateProcess(int pid, bool cascade, const std::string& reason) {
    auto pcb = processes_.get(pid);
    if (!pcb) {
        return OperationResult::failure("Unknown PID");
    }
    if (pcb->systemProcess && reason != "shutdown") {
        return OperationResult::failure("System processes cannot be terminated by the user");
    }
    if (pcb->state == ProcessState::Terminated) {
        return OperationResult::success();
    }
    const auto children = pcb->childPids;
    if (cascade) {
        for (int child : children) {
            terminateProcess(child, true, reason);
        }
    } else {
        for (int child : children) {
            auto childPcb = processes_.get(child);
            if (!childPcb || childPcb->state == ProcessState::Terminated) {
                continue;
            }
            processes_.removeChild(pid, child);
            processes_.addChild(1, child);
        }
    }
    scheduler_.dequeue(pid);
    memory_.releaseProcessMemory(pid);
    processes_.updateMemoryStats(pid, 0, pcb->requiredPages, 0);
    processes_.setPartition(pid, std::nullopt, std::nullopt);
    if (pcb->parentPid > 0) {
        processes_.removeChild(pcb->parentPid, pid);
    }
    if (pcb->state != ProcessState::Terminated) {
        if (pcb->state == ProcessState::New) {
            processes_.setState(pid, ProcessState::Ready, "prepare terminate");
        }
        if (pcb->state == ProcessState::Running) {
            processes_.setState(pid, ProcessState::Ready, "preempt for terminate");
        }
        auto latest = processes_.get(pid);
        if (latest && latest->state == ProcessState::Blocked) {
            processes_.setState(pid, ProcessState::Ready, "unblock for terminate");
        }
        processes_.markTerminated(pid, 0, reason);
    }
    applications_.close(pid);
    return OperationResult::success();
}

std::optional<ProcessControlBlock> MiniSO::getProcess(int pid) const {
    return processes_.get(pid);
}

std::vector<ProcessControlBlock> MiniSO::processes(bool includeTerminated) const {
    return includeTerminated ? processes_.listAll() : processes_.listAlive();
}

std::string MiniSO::processTree() const {
    return processes_.treeText(1);
}

OperationResult MiniSO::schedulerTick() {
    auto result = scheduler_.tick();
    for (const auto& pcb : processes_.listAll()) {
        if (pcb.state == ProcessState::Terminated && memory_.pageTable(pcb.pid)) {
            memory_.releaseProcessMemory(pcb.pid);
            processes_.updateMemoryStats(pcb.pid, 0, pcb.requiredPages, 0);
        } else if (pcb.state != ProcessState::Terminated) {
            syncProcessMemory(pcb.pid);
        }
    }
    return result;
}

void MiniSO::startScheduler() {
    scheduler_.start();
}

void MiniSO::stopScheduler() {
    scheduler_.stop();
}

void MiniSO::setQuantum(int quantum) {
    scheduler_.setQuantum(quantum);
}

SchedulerSnapshot MiniSO::schedulerSnapshot() const {
    return scheduler_.snapshot();
}

ValueResult<MemoryBlock> MiniSO::allocatePartition(std::size_t size, int pid) {
    return memory_.allocatePartition(size, pid);
}

OperationResult MiniSO::releasePartition(std::size_t start) {
    return memory_.releasePartition(start);
}

AddressTranslation MiniSO::translateAddress(int pid, std::size_t logicalAddress) {
    auto result = memory_.translate(pid, logicalAddress);
    events_.publish(EventType::AddressTranslated, "MemoryManager", result.explanation, pid);
    return result;
}

OperationResult MiniSO::recoverPage(int pid, int pageNumber) {
    processes_.setState(pid, ProcessState::Blocked, "page fault");
    scheduler_.dequeue(pid);
    auto recovered = memory_.recoverPage(pid, pageNumber);
    if (recovered.ok) {
        processes_.setState(pid, ProcessState::Ready, "page recovered");
        scheduler_.enqueue(pid);
        syncProcessMemory(pid);
    }
    return recovered;
}

MemoryStatus MiniSO::memoryStatus() const {
    return memory_.status();
}

std::optional<PageTable> MiniSO::pageTable(int pid) const {
    return memory_.pageTable(pid);
}

std::string MiniSO::dumpPartitions() const {
    return memory_.partitions().dump();
}

std::string MiniSO::dumpFrames() const {
    return memory_.paging().dumpFrames();
}

std::string MiniSO::dumpSwap() const {
    return memory_.paging().swap().dump();
}

OperationResult MiniSO::startProducerConsumer(int capacity, int items) {
    auto configured = producerConsumer_.configure(capacity, items);
    if (!configured.ok) {
        return configured;
    }
    return producerConsumer_.start();
}

OperationResult MiniSO::stopProducerConsumer() {
    return producerConsumer_.stop();
}

bool MiniSO::waitProducerConsumer(int timeoutMs) {
    return producerConsumer_.waitUntilFinished(timeoutMs);
}

ProducerConsumerStats MiniSO::producerConsumerStats() const {
    return producerConsumer_.stats();
}

ValueResult<ApplicationInstance> MiniSO::openApplication(const std::string& appId) {
    return applications_.open(appId);
}

OperationResult MiniSO::closeApplication(int pid) {
    return terminateProcess(pid, true, "application closed");
}

OperationResult MiniSO::addBrowserTab(int browserPid, const std::string& title) {
    return applications_.addBrowserTab(browserPid, title);
}

OperationResult MiniSO::closeBrowserTab(int tabPid) {
    return applications_.closeBrowserTab(tabPid);
}

ValueResult<MiniRendererResult> MiniSO::runMiniRenderer(int rows, int cols, bool injectFault) {
    CreateProcessRequest request;
    request.name = "MiniRenderer";
    request.logicalMemorySize = static_cast<std::size_t>(rows * cols * 8);
    if (request.logicalMemorySize < config_.pageSizeBytes * 2) {
        request.logicalMemorySize = config_.pageSizeBytes * 2;
    }
    request.cpuBurst = 12;
    request.logicalThreadCount = 4;
    request.applicationId = "MiniRenderer";
    auto process = createProcess(request);
    if (!process.ok) {
        return ValueResult<MiniRendererResult>::failure(process.message);
    }
    auto result = renderer_.run(process.value.pid, rows, cols, injectFault);
    return result;
}

ValueResult<int> MiniSO::runMiniStress(int count, std::size_t memoryEach, int priority, int threads) {
    int created = 0;
    for (int i = 0; i < count; ++i) {
        CreateProcessRequest request;
        request.name = "MiniStress-" + std::to_string(i + 1);
        request.logicalMemorySize = memoryEach;
        request.priority = priority;
        request.cpuBurst = 8;
        request.logicalThreadCount = threads;
        request.applicationId = "MiniStress";
        auto process = createProcess(request);
        if (!process.ok) {
            return ValueResult<int>::success(created, "Stopped: " + process.message);
        }
        ++created;
    }
    return ValueResult<int>::success(created);
}

std::string MiniSO::runScenarioA() {
    events_.publish(EventType::ScenarioStarted, "ScenarioA", "Lifecycle, parent/child and paging");
    std::ostringstream out;
    auto browser = openApplication("MiniBrowser");
    out << "Open MiniBrowser: " << (browser.ok ? "ok pid=" + std::to_string(browser.value.pid) : browser.message)
        << '\n';
    if (browser.ok) {
        addBrowserTab(browser.value.pid, "Inicio");
        addBrowserTab(browser.value.pid, "Noticias");
        addBrowserTab(browser.value.pid, "Documentacion");
    }
    out << "Process tree:\n" << processTree();
    for (int i = 0; i < 12; ++i) {
        schedulerTick();
    }
    auto all = processes(false);
    for (const auto& pcb : all) {
        if (pcb.name.find("Noticias") != std::string::npos) {
            closeBrowserTab(pcb.pid);
            out << "Closed tab Noticias pid=" << pcb.pid << '\n';
            break;
        }
    }
    if (browser.ok) {
        addBrowserTab(browser.value.pid, "Nueva");
    }
    out << "Frames after recycle:\n" << dumpFrames();
    out << "Paging does not require contiguous frames.\n";
    events_.publish(EventType::ScenarioFinished, "ScenarioA", "Completed");
    return out.str();
}

std::string MiniSO::runScenarioB() {
    events_.publish(EventType::ScenarioStarted, "ScenarioB", "Concurrency and page fault");
    auto result = runMiniRenderer(8, 8, true);
    std::ostringstream out;
    if (!result.ok) {
        out << "Failed: " << result.message << '\n';
        return out.str();
    }
    out << "MiniRenderer pid=" << result.value.processId << " checksum=" << result.value.checksum
        << " recovered=" << result.value.recovered << '\n';
    events_.publish(EventType::ScenarioFinished, "ScenarioB", "Completed");
    return out.str();
}

std::string MiniSO::runScenarioC() {
    events_.publish(EventType::ScenarioStarted, "ScenarioC", "Resource exhaustion");
    auto result = runMiniStress(40, config_.pageSizeBytes * 4, 0, 1);
    std::ostringstream out;
    out << "Created " << result.value << " MiniStress processes. " << result.message << '\n';
    out << "Paging used frames=" << memoryStatus().paging.usedFrames << '\n';
    events_.publish(EventType::ScenarioFinished, "ScenarioC", "Completed");
    return out.str();
}

std::string MiniSO::runScenarioD() {
    events_.publish(EventType::ScenarioStarted, "ScenarioD", "Producer-consumer");
    startProducerConsumer(4, 20);
    const bool finished = waitProducerConsumer(8000);
    auto stats = producerConsumerStats();
    std::ostringstream out;
    out << "produced=" << stats.produced << " consumed=" << stats.consumed
        << " finished=" << finished << " valid=" << producerConsumer_.valid() << '\n';
    events_.publish(EventType::ScenarioFinished, "ScenarioD", out.str());
    return out.str();
}

ValueResult<LinuxRunReport> MiniSO::runLinuxDemo() {
    return linuxRunner_.runDemo();
}

SystemStatus MiniSO::status() const {
    SystemStatus snapshot;
    snapshot.memory = memory_.status();
    snapshot.scheduler = scheduler_.snapshot();
    snapshot.activeProcesses = static_cast<int>(processes_.listAlive().size());
    return snapshot;
}

}  // namespace miniso
