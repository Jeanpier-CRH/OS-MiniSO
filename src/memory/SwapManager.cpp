#include "memory/SwapManager.h"

#include <sstream>

namespace miniso {

SwapManager::SwapManager(std::size_t slotCount) {
    reset(slotCount);
}

void SwapManager::reset(std::size_t slotCount) {
    slots_.assign(slotCount, SwapSlot{});
    for (std::size_t i = 0; i < slots_.size(); ++i) {
        slots_[i].index = static_cast<int>(i);
    }
}

ValueResult<int> SwapManager::store(int processId, int pageNumber) {
    for (auto& slot : slots_) {
        if (!slot.used) {
            slot.used = true;
            slot.processId = processId;
            slot.pageNumber = pageNumber;
            return ValueResult<int>::success(slot.index);
        }
    }
    return ValueResult<int>::failure("Swap is full");
}

OperationResult SwapManager::release(int slot) {
    if (slot < 0 || static_cast<std::size_t>(slot) >= slots_.size()) {
        return OperationResult::failure("Invalid swap slot");
    }
    slots_[static_cast<std::size_t>(slot)] = SwapSlot{slot, false, -1, -1};
    return OperationResult::success();
}

OperationResult SwapManager::releaseProcess(int processId) {
    for (auto& slot : slots_) {
        if (slot.used && slot.processId == processId) {
            slot.used = false;
            slot.processId = -1;
            slot.pageNumber = -1;
        }
    }
    return OperationResult::success();
}

std::optional<SwapSlot> SwapManager::get(int slot) const {
    if (slot < 0 || static_cast<std::size_t>(slot) >= slots_.size()) {
        return std::nullopt;
    }
    return slots_[static_cast<std::size_t>(slot)];
}

std::vector<SwapSlot> SwapManager::slots() const {
    return slots_;
}

std::size_t SwapManager::usedCount() const {
    std::size_t count = 0;
    for (const auto& slot : slots_) {
        if (slot.used) {
            ++count;
        }
    }
    return count;
}

std::string SwapManager::dump() const {
    std::ostringstream out;
    for (const auto& slot : slots_) {
        if (slot.used) {
            out << "swap[" << slot.index << "] pid=" << slot.processId
                << " page=" << slot.pageNumber << '\n';
        }
    }
    return out.str();
}

}  // namespace miniso
