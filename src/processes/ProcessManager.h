#pragma once

#include "core/EventSystem.h"
#include "core/OperationResult.h"
#include "processes/ProcessControlBlock.h"

#include <map>
#include <mutex>
#include <string>
#include <vector>

namespace miniso {

struct CreateProcessRequest {
    std::string name;
    std::size_t logicalMemorySize = 0;
    int priority = 0;
    std::uint64_t cpuBurst = 10;
    int logicalThreadCount = 1;
    int parentPid = 1;
    bool systemProcess = false;
    int forcedPid = -1;
    std::string applicationId;
};

class ProcessManager {
public:
    explicit ProcessManager(EventSystem& events, int firstUserPid = 100);

    ValueResult<ProcessControlBlock> create(const CreateProcessRequest& request);
    OperationResult setState(int pid, ProcessState state, const std::string& reason = {});
    OperationResult addChild(int parentPid, int childPid);
    OperationResult removeChild(int parentPid, int childPid);
    OperationResult markTerminated(int pid, int exitCode, const std::string& reason);
    OperationResult updateMemoryStats(int pid, std::size_t resident, int pages, int frames);
    OperationResult setPartition(int pid, std::optional<std::size_t> start, std::optional<std::size_t> size);
    OperationResult consumeCpu(int pid, std::uint64_t units);
    OperationResult resetQuantum(int pid, std::uint64_t quantum);

    std::optional<ProcessControlBlock> get(int pid) const;
    std::vector<ProcessControlBlock> listAlive() const;
    std::vector<ProcessControlBlock> listAll() const;
    std::vector<ProcessControlBlock> history() const;
    std::string treeText(int rootPid = 1) const;
    bool exists(int pid) const;
    bool isSystemProcess(int pid) const;
    int nextUserPid() const;

private:
    std::string treeNode(int pid, const std::string& prefix, bool last) const;

    EventSystem& events_;
    mutable std::mutex mutex_;
    int nextUserPid_;
    std::map<int, ProcessControlBlock> processes_;
    std::vector<int> historyOrder_;
};

}  // namespace miniso
