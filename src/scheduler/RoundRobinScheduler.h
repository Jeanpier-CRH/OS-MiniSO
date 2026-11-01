#pragma once

#include "core/EventSystem.h"
#include "processes/ProcessManager.h"

#include <atomic>
#include <cstdint>
#include <deque>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

namespace miniso {

struct SchedulerSnapshot {
    std::uint64_t clock = 0;
    int quantum = 3;
    int runningPid = -1;
    std::vector<int> readyQueue;
    bool idle = true;
};

class RoundRobinScheduler {
public:
    RoundRobinScheduler(EventSystem& events, ProcessManager& processes, int quantum);
    ~RoundRobinScheduler();

    RoundRobinScheduler(const RoundRobinScheduler&) = delete;
    RoundRobinScheduler& operator=(const RoundRobinScheduler&) = delete;

    void setQuantum(int quantum);
    int quantum() const;
    void setTickDelayMs(int milliseconds);
    void enqueue(int pid);
    void dequeue(int pid);
    void block(int pid, const std::string& reason);
    OperationResult unblock(int pid);
    OperationResult tick();
    void start();
    void stop();
    SchedulerSnapshot snapshot() const;
    std::uint64_t clock() const;

private:
    void loop();

    EventSystem& events_;
    ProcessManager& processes_;
    mutable std::mutex mutex_;
    int quantum_;
    int tickDelayMs_ = 50;
    std::uint64_t clock_ = 0;
    int runningPid_ = -1;
    std::deque<int> ready_;
    std::atomic<bool> runningThread_{false};
    std::thread worker_;
};

}  // namespace miniso
