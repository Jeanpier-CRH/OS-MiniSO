#include "scheduler/RoundRobinScheduler.h"

#include <algorithm>
#include <chrono>

namespace miniso {

RoundRobinScheduler::RoundRobinScheduler(EventSystem& events, ProcessManager& processes, int quantum)
    : events_(events), processes_(processes), quantum_(quantum < 1 ? 1 : quantum) {}

RoundRobinScheduler::~RoundRobinScheduler() {
    stop();
}

void RoundRobinScheduler::setQuantum(int quantum) {
    std::lock_guard<std::mutex> lock(mutex_);
    quantum_ = quantum < 1 ? 1 : quantum;
}

int RoundRobinScheduler::quantum() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return quantum_;
}

void RoundRobinScheduler::setTickDelayMs(int milliseconds) {
    tickDelayMs_ = milliseconds < 0 ? 0 : milliseconds;
}

void RoundRobinScheduler::enqueue(int pid) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (std::find(ready_.begin(), ready_.end(), pid) == ready_.end() && pid != runningPid_) {
        ready_.push_back(pid);
    }
}

void RoundRobinScheduler::dequeue(int pid) {
    std::lock_guard<std::mutex> lock(mutex_);
    ready_.erase(std::remove(ready_.begin(), ready_.end(), pid), ready_.end());
    if (runningPid_ == pid) {
        runningPid_ = -1;
    }
}

void RoundRobinScheduler::block(int pid, const std::string& reason) {
    dequeue(pid);
    processes_.setState(pid, ProcessState::Blocked, reason);
}

OperationResult RoundRobinScheduler::unblock(int pid) {
    auto pcb = processes_.get(pid);
    if (!pcb) {
        return OperationResult::failure("Unknown PID");
    }
    if (pcb->state != ProcessState::Blocked) {
        return OperationResult::failure("Process is not blocked");
    }
    auto ready = processes_.setState(pid, ProcessState::Ready, "unblocked");
    if (!ready.ok) {
        return ready;
    }
    enqueue(pid);
    return OperationResult::success();
}

OperationResult RoundRobinScheduler::tick() {
    std::unique_lock<std::mutex> lock(mutex_);
    ++clock_;
    if (runningPid_ < 0) {
        if (ready_.empty()) {
            lock.unlock();
            events_.publish(EventType::SchedulerTick, "Scheduler", "Idle tick");
            return OperationResult::success("idle");
        }
        runningPid_ = ready_.front();
        ready_.pop_front();
        lock.unlock();
        processes_.setState(runningPid_, ProcessState::Running, "dispatch");
        processes_.resetQuantum(runningPid_, static_cast<std::uint64_t>(quantum_));
        lock.lock();
    }

    const int current = runningPid_;
    lock.unlock();

    auto pcb = processes_.get(current);
    if (!pcb || pcb->state == ProcessState::Terminated) {
        lock.lock();
        runningPid_ = -1;
        return OperationResult::success();
    }
    if (pcb->state == ProcessState::Blocked) {
        lock.lock();
        runningPid_ = -1;
        return OperationResult::success();
    }

    processes_.consumeCpu(current, 1);
    pcb = processes_.get(current);
    if (!pcb) {
        return OperationResult::failure("Process disappeared");
    }

    if (pcb->remainingCpuTime == 0) {
        lock.lock();
        runningPid_ = -1;
        lock.unlock();
        processes_.markTerminated(current, 0, "CPU burst completed");
        return OperationResult::success("terminated");
    }

    if (pcb->quantumRemaining == 0) {
        lock.lock();
        runningPid_ = -1;
        ready_.push_back(current);
        lock.unlock();
        processes_.setState(current, ProcessState::Ready, "quantum expired");
        events_.publish(EventType::QuantumExpired, "Scheduler", "Quantum expired", current);
        return OperationResult::success("preempted");
    }

    return OperationResult::success("running");
}

void RoundRobinScheduler::start() {
    bool expected = false;
    if (!runningThread_.compare_exchange_strong(expected, true)) {
        return;
    }
    worker_ = std::thread([this] { loop(); });
}

void RoundRobinScheduler::stop() {
    runningThread_ = false;
    if (worker_.joinable()) {
        worker_.join();
    }
}

void RoundRobinScheduler::loop() {
    while (runningThread_) {
        tick();
        if (tickDelayMs_ > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(tickDelayMs_));
        }
    }
}

SchedulerSnapshot RoundRobinScheduler::snapshot() const {
    std::lock_guard<std::mutex> lock(mutex_);
    SchedulerSnapshot snap;
    snap.clock = clock_;
    snap.quantum = quantum_;
    snap.runningPid = runningPid_;
    snap.readyQueue.assign(ready_.begin(), ready_.end());
    snap.idle = runningPid_ < 0 && ready_.empty();
    return snap;
}

std::uint64_t RoundRobinScheduler::clock() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return clock_;
}

}  // namespace miniso
