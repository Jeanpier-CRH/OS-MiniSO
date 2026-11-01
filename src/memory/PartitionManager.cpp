#include "memory/PartitionManager.h"

#include <algorithm>
#include <sstream>
#include <stdexcept>

namespace miniso {

PartitionManager::PartitionManager(std::size_t totalBytes) {
    reset(totalBytes);
}

void PartitionManager::reset(std::size_t totalBytes) {
    total_ = totalBytes;
    blocks_.clear();
    if (totalBytes > 0) {
        blocks_.push_back(MemoryBlock{0, totalBytes, false, -1});
    }
}

ValueResult<MemoryBlock> PartitionManager::allocate(std::size_t size, int processId) {
    if (size == 0) {
        return ValueResult<MemoryBlock>::failure("Block size must be greater than zero");
    }
    for (std::size_t i = 0; i < blocks_.size(); ++i) {
        auto& block = blocks_[i];
        if (!block.allocated && block.size >= size) {
            MemoryBlock allocated{block.startAddress, size, true, processId};
            const std::size_t leftover = block.size - size;
            if (leftover == 0) {
                block = allocated;
            } else {
                block.size = size;
                block.allocated = true;
                block.processId = processId;
                blocks_.insert(blocks_.begin() + static_cast<std::ptrdiff_t>(i) + 1,
                               MemoryBlock{allocated.startAddress + size, leftover, false, -1});
            }
            assertInvariants();
            return ValueResult<MemoryBlock>::success(allocated);
        }
    }
    if (wouldFailDueToFragmentation(size)) {
        return ValueResult<MemoryBlock>::failure(
            "External fragmentation: free memory exists but no contiguous block is large enough");
    }
    return ValueResult<MemoryBlock>::failure("Insufficient partition memory");
}

OperationResult PartitionManager::releaseByAddress(std::size_t startAddress) {
    for (auto& block : blocks_) {
        if (block.startAddress == startAddress) {
            if (!block.allocated) {
                return OperationResult::failure("Double free of partition block");
            }
            block.allocated = false;
            block.processId = -1;
            coalesce();
            assertInvariants();
            return OperationResult::success();
        }
    }
    return OperationResult::failure("Unknown partition address");
}

OperationResult PartitionManager::releaseByProcess(int processId) {
    bool found = false;
    for (auto& block : blocks_) {
        if (block.allocated && block.processId == processId) {
            block.allocated = false;
            block.processId = -1;
            found = true;
        }
    }
    if (!found) {
        return OperationResult::failure("No partition allocated for process");
    }
    coalesce();
    assertInvariants();
    return OperationResult::success();
}

PartitionStats PartitionManager::stats(std::size_t pendingRequest) const {
    PartitionStats result;
    result.total = total_;
    for (const auto& block : blocks_) {
        if (block.allocated) {
            result.used += block.size;
        } else {
            result.free += block.size;
            result.freeBlocks += 1;
            result.largestFree = std::max(result.largestFree, block.size);
        }
    }
    if (pendingRequest > 0) {
        result.externalFragmentation = result.freeBlocks > 1 && result.largestFree < pendingRequest;
    } else {
        result.externalFragmentation = result.freeBlocks > 1 && result.used > 0;
    }
    return result;
}

std::vector<MemoryBlock> PartitionManager::blocks() const {
    return blocks_;
}

std::optional<MemoryBlock> PartitionManager::findByProcess(int processId) const {
    for (const auto& block : blocks_) {
        if (block.allocated && block.processId == processId) {
            return block;
        }
    }
    return std::nullopt;
}

std::string PartitionManager::dump() const {
    std::ostringstream out;
    for (const auto& block : blocks_) {
        out << "[" << block.startAddress << "-" << (block.startAddress + block.size - 1) << " "
            << (block.allocated ? "USED pid=" + std::to_string(block.processId) : "FREE")
            << " size=" << block.size << "]\n";
    }
    return out.str();
}

bool PartitionManager::wouldFailDueToFragmentation(std::size_t size) const {
    const auto current = stats(size);
    return current.externalFragmentation;
}

void PartitionManager::coalesce() {
    std::vector<MemoryBlock> merged;
    for (const auto& block : blocks_) {
        if (!merged.empty() && !merged.back().allocated && !block.allocated) {
            merged.back().size += block.size;
        } else {
            merged.push_back(block);
        }
    }
    blocks_ = std::move(merged);
}

void PartitionManager::assertInvariants() const {
    std::size_t cursor = 0;
    std::size_t sum = 0;
    for (const auto& block : blocks_) {
        if (block.size == 0) {
            throw std::logic_error("Zero-sized partition block");
        }
        if (block.startAddress != cursor) {
            throw std::logic_error("Partition hole or overlap detected");
        }
        cursor += block.size;
        sum += block.size;
    }
    if (sum != total_) {
        throw std::logic_error("Partition total size mismatch");
    }
}

}  // namespace miniso
