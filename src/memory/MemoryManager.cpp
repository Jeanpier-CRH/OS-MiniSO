#include "memory/MemoryManager.h"

namespace miniso {

MemoryManager::MemoryManager(EventSystem& events, const SystemConfig& config)
    : events_(events),
      partitions_(config.ramBytes),
      paging_(events, config.ramBytes, config.pageSizeBytes, config.swapPages),
      oom_(config.oom) {}

ValueResult<PageTable> MemoryManager::allocateProcessMemory(int pid, std::size_t logicalSize) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto partition = partitions_.allocate(logicalSize == 0 ? 1 : logicalSize, pid);
    if (!partition.ok) {
        events_.publish(EventType::Error, "MemoryManager", partition.message, pid, -1, EventLevel::Error);
        return ValueResult<PageTable>::failure(partition.message);
    }
    auto pages = paging_.allocateProcess(pid, logicalSize);
    if (!pages.ok) {
        partitions_.releaseByProcess(pid);
        events_.publish(EventType::Error, "MemoryManager", pages.message, pid, -1, EventLevel::Error);
        return pages;
    }
    events_.publish(EventType::MemoryAllocated, "MemoryManager",
                    "Partition and pages allocated", pid);
    return pages;
}

OperationResult MemoryManager::releaseProcessMemory(int pid) {
    std::lock_guard<std::mutex> lock(mutex_);
    partitions_.releaseByProcess(pid);
    return paging_.releaseProcess(pid);
}

ValueResult<MemoryBlock> MemoryManager::allocatePartition(std::size_t size, int pid) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto result = partitions_.allocate(size, pid);
    if (result.ok) {
        events_.publish(EventType::MemoryAllocated, "PartitionManager",
                        "Allocated " + std::to_string(size) + " bytes at " +
                            std::to_string(result.value.startAddress),
                        pid);
    }
    return result;
}

OperationResult MemoryManager::releasePartition(std::size_t start) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto result = partitions_.releaseByAddress(start);
    if (result.ok) {
        events_.publish(EventType::MemoryReleased, "PartitionManager",
                        "Released partition at " + std::to_string(start));
    }
    return result;
}

AddressTranslation MemoryManager::translate(int pid, std::size_t logicalAddress) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return paging_.translate(pid, logicalAddress);
}

OperationResult MemoryManager::recoverPage(int pid, int pageNumber) {
    std::lock_guard<std::mutex> lock(mutex_);
    return paging_.handlePageFault(pid, pageNumber);
}

OperationResult MemoryManager::forceEvict(int pid, int pageNumber) {
    std::lock_guard<std::mutex> lock(mutex_);
    return paging_.forceEvict(pid, pageNumber);
}

OperationResult MemoryManager::pinPage(int pid, int pageNumber) {
    std::lock_guard<std::mutex> lock(mutex_);
    return paging_.pinPage(pid, pageNumber);
}

OperationResult MemoryManager::unpinPage(int pid, int pageNumber) {
    std::lock_guard<std::mutex> lock(mutex_);
    return paging_.unpinPage(pid, pageNumber);
}

OomDecision MemoryManager::chooseVictim(const std::vector<ProcessControlBlock>& processes) const {
    return oom_.selectVictim(processes);
}

MemoryStatus MemoryManager::status() const {
    std::lock_guard<std::mutex> lock(mutex_);
    MemoryStatus status;
    status.partition = partitions_.stats();
    status.paging = paging_.stats();
    return status;
}

std::optional<PageTable> MemoryManager::pageTable(int pid) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return paging_.pageTable(pid);
}

}  // namespace miniso
