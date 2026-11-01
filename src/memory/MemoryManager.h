#pragma once

#include "core/EventSystem.h"
#include "core/SystemConfig.h"
#include "memory/OOMManager.h"
#include "memory/PagingManager.h"
#include "memory/PartitionManager.h"
#include "processes/ProcessControlBlock.h"

#include <mutex>
#include <string>

namespace miniso {

struct MemoryStatus {
    PartitionStats partition;
    PagingStats paging;
    std::size_t internalFragmentation = 0;
};

class MemoryManager {
public:
    MemoryManager(EventSystem& events, const SystemConfig& config);

    ValueResult<PageTable> allocateProcessMemory(int pid, std::size_t logicalSize);
    OperationResult releaseProcessMemory(int pid);
    ValueResult<MemoryBlock> allocatePartition(std::size_t size, int pid);
    OperationResult releasePartition(std::size_t start);
    AddressTranslation translate(int pid, std::size_t logicalAddress) const;
    OperationResult recoverPage(int pid, int pageNumber);
    OperationResult forceEvict(int pid, int pageNumber);
    OperationResult pinPage(int pid, int pageNumber);
    OperationResult unpinPage(int pid, int pageNumber);
    OomDecision chooseVictim(const std::vector<ProcessControlBlock>& processes) const;
    MemoryStatus status() const;
    std::optional<PageTable> pageTable(int pid) const;
    PartitionManager& partitions() { return partitions_; }
    PagingManager& paging() { return paging_; }
    const PartitionManager& partitions() const { return partitions_; }
    const PagingManager& paging() const { return paging_; }

private:
    EventSystem& events_;
    mutable std::mutex mutex_;
    PartitionManager partitions_;
    PagingManager paging_;
    OOMManager oom_;
};

}  // namespace miniso
