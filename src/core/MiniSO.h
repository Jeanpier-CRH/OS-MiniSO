#pragma once

#include "applications/ApplicationManager.h"
#include "concurrency/MiniRenderer.h"
#include "concurrency/ProducerConsumer.h"
#include "core/EventSystem.h"
#include "core/SystemConfig.h"
#include "linux/ProcessRunner.h"
#include "logging/EventLogger.h"
#include "memory/MemoryManager.h"
#include "processes/ProcessManager.h"
#include "scheduler/RoundRobinScheduler.h"

#include <mutex>
#include <string>
#include <vector>

namespace miniso {

struct SystemStatus {
    std::string state = "RUNNING";
    MemoryStatus memory;
    SchedulerSnapshot scheduler;
    int activeProcesses = 0;
};

class MiniSO {
public:
    explicit MiniSO(SystemConfig config = {});

    void boot();
    void shutdown();

    ValueResult<ProcessControlBlock> createProcess(const CreateProcessRequest& request);
    OperationResult terminateProcess(int pid, bool cascade = false, const std::string& reason = "user");
    std::optional<ProcessControlBlock> getProcess(int pid) const;
    std::vector<ProcessControlBlock> processes(bool includeTerminated = false) const;
    std::string processTree() const;

    OperationResult schedulerTick();
    void startScheduler();
    void stopScheduler();
    void setQuantum(int quantum);
    SchedulerSnapshot schedulerSnapshot() const;

    ValueResult<MemoryBlock> allocatePartition(std::size_t size, int pid);
    OperationResult releasePartition(std::size_t start);
    AddressTranslation translateAddress(int pid, std::size_t logicalAddress);
    OperationResult recoverPage(int pid, int pageNumber);
    MemoryStatus memoryStatus() const;
    std::optional<PageTable> pageTable(int pid) const;
    std::string dumpPartitions() const;
    std::string dumpFrames() const;
    std::string dumpSwap() const;

    OperationResult startProducerConsumer(int capacity, int items);
    OperationResult stopProducerConsumer();
    bool waitProducerConsumer(int timeoutMs = 8000);
    ProducerConsumerStats producerConsumerStats() const;

    ValueResult<ApplicationInstance> openApplication(const std::string& appId);
    OperationResult closeApplication(int pid);
    OperationResult addBrowserTab(int browserPid, const std::string& title);
    OperationResult closeBrowserTab(int tabPid);

    ValueResult<MiniRendererResult> runMiniRenderer(int rows, int cols, bool injectFault);
    ValueResult<int> runMiniStress(int count, std::size_t memoryEach, int priority, int threads);
    std::string runScenarioA();
    std::string runScenarioB();
    std::string runScenarioC();
    std::string runScenarioD();

    ValueResult<LinuxRunReport> runLinuxDemo();

    EventSystem& events() { return events_; }
    const EventSystem& events() const { return events_; }
    EventLogger& logger() { return logger_; }
    SystemStatus status() const;
    const SystemConfig& config() const { return config_; }
    MemoryManager& memory() { return memory_; }
    ProcessManager& processManager() { return processes_; }

private:
    void createSystemProcesses();
    void syncProcessMemory(int pid);
    OperationResult admitProcess(int pid);

    SystemConfig config_;
    EventSystem events_;
    EventLogger logger_;
    ProcessManager processes_;
    MemoryManager memory_;
    RoundRobinScheduler scheduler_;
    ProducerConsumer producerConsumer_;
    MiniRenderer renderer_;
    ProcessRunner linuxRunner_;
    ApplicationManager applications_;
    mutable std::mutex mutex_;
    bool booted_ = false;
};

}  // namespace miniso
