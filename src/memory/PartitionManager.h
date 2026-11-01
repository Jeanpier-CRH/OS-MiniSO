#pragma once

#include "core/OperationResult.h"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace miniso {

struct MemoryBlock {
    std::size_t startAddress = 0;
    std::size_t size = 0;
    bool allocated = false;
    int processId = -1;
};

struct PartitionStats {
    std::size_t total = 0;
    std::size_t used = 0;
    std::size_t free = 0;
    std::size_t largestFree = 0;
    std::size_t freeBlocks = 0;
    bool externalFragmentation = false;
};

class PartitionManager {
public:
    explicit PartitionManager(std::size_t totalBytes);

    ValueResult<MemoryBlock> allocate(std::size_t size, int processId);
    OperationResult releaseByAddress(std::size_t startAddress);
    OperationResult releaseByProcess(int processId);
    PartitionStats stats(std::size_t pendingRequest = 0) const;
    std::vector<MemoryBlock> blocks() const;
    std::optional<MemoryBlock> findByProcess(int processId) const;
    std::string dump() const;
    bool wouldFailDueToFragmentation(std::size_t size) const;
    void reset(std::size_t totalBytes);

private:
    void coalesce();
    void assertInvariants() const;

    std::vector<MemoryBlock> blocks_;
    std::size_t total_ = 0;
};

}  // namespace miniso
