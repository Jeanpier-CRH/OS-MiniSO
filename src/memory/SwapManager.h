#pragma once

#include "core/OperationResult.h"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace miniso {

struct SwapSlot {
    int index = -1;
    bool used = false;
    int processId = -1;
    int pageNumber = -1;
};

class SwapManager {
public:
    explicit SwapManager(std::size_t slotCount);

    ValueResult<int> store(int processId, int pageNumber);
    OperationResult release(int slot);
    OperationResult releaseProcess(int processId);
    std::optional<SwapSlot> get(int slot) const;
    std::vector<SwapSlot> slots() const;
    std::size_t usedCount() const;
    std::size_t capacity() const { return slots_.size(); }
    void reset(std::size_t slotCount);
    std::string dump() const;

private:
    std::vector<SwapSlot> slots_;
};

}  // namespace miniso
