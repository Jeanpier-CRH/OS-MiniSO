#include "memory/PagingManager.h"

#include <algorithm>
#include <sstream>

namespace miniso {

PagingManager::PagingManager(EventSystem& events,
                             std::size_t ramBytes,
                             std::size_t pageSize,
                             std::size_t swapPages)
    : events_(events), swap_(swapPages) {
    reset(ramBytes, pageSize, swapPages);
}

void PagingManager::reset(std::size_t ramBytes, std::size_t pageSize, std::size_t swapPages) {
    ramBytes_ = ramBytes;
    pageSize_ = pageSize == 0 ? 1 : pageSize;
    const int frameCount = static_cast<int>(ramBytes_ / pageSize_);
    frames_.assign(static_cast<std::size_t>(frameCount), PhysicalFrame{});
    for (int i = 0; i < frameCount; ++i) {
        frames_[static_cast<std::size_t>(i)].number = i;
        frames_[static_cast<std::size_t>(i)].free = true;
    }
    fifo_.clear();
    tables_.clear();
    swap_.reset(swapPages);
}

int PagingManager::requiredPages(std::size_t logicalSize) const {
    if (logicalSize == 0) {
        return 0;
    }
    return static_cast<int>((logicalSize + pageSize_ - 1) / pageSize_);
}

int PagingManager::findFreeFrame() const {
    for (const auto& frame : frames_) {
        if (frame.free) {
            return frame.number;
        }
    }
    return -1;
}

ValueResult<PageTable> PagingManager::allocateProcess(int pid, std::size_t logicalSize) {
    if (tables_.count(pid) != 0) {
        return ValueResult<PageTable>::failure("Page table already exists for process");
    }
    PageTable table;
    table.processId = pid;
    table.logicalSize = logicalSize;
    table.pageSize = pageSize_;
    const int pages = requiredPages(logicalSize);
    const auto current = stats();
    const int backingFree = current.freeFrames + static_cast<int>(current.swapCapacity - current.swapUsed);
    if (pages > backingFree) {
        return ValueResult<PageTable>::failure(
            "Insufficient simulated RAM and swap to back the requested pages");
    }
    table.entries.resize(static_cast<std::size_t>(pages));
    for (int i = 0; i < pages; ++i) {
        table.entries[static_cast<std::size_t>(i)].pageNumber = i;
    }
    tables_[pid] = table;
    for (int i = 0; i < pages; ++i) {
        auto loaded = loadPage(pid, i);
        if (!loaded.ok) {
            break;
        }
    }
    return ValueResult<PageTable>::success(tables_[pid]);
}

OperationResult PagingManager::loadPage(int pid, int pageNumber) {
    auto tableIt = tables_.find(pid);
    if (tableIt == tables_.end()) {
        return OperationResult::failure("Unknown page table");
    }
    if (pageNumber < 0 || static_cast<std::size_t>(pageNumber) >= tableIt->second.entries.size()) {
        return OperationResult::failure("Invalid page number");
    }
    auto& entry = tableIt->second.entries[static_cast<std::size_t>(pageNumber)];
    if (entry.present) {
        return OperationResult::success("Already resident");
    }
    int frame = findFreeFrame();
    if (frame < 0) {
        auto evicted = evictOldestEligible();
        if (!evicted.ok) {
            return evicted;
        }
        frame = findFreeFrame();
        if (frame < 0) {
            return OperationResult::failure("No evictable frame available");
        }
    }
    auto& physical = frames_[static_cast<std::size_t>(frame)];
    physical.free = false;
    physical.processId = pid;
    physical.pageNumber = pageNumber;
    entry.present = true;
    entry.frameNumber = frame;
    entry.referenced = true;
    fifo_.push_back(frame);
    events_.publish(EventType::PageLoaded, "PagingManager",
                    "Loaded page " + std::to_string(pageNumber) + " into frame " + std::to_string(frame),
                    pid);
    events_.publish(EventType::PageMapped, "PagingManager",
                    "Mapped page " + std::to_string(pageNumber) + " -> frame " + std::to_string(frame),
                    pid);
    return OperationResult::success();
}

OperationResult PagingManager::evictOldestEligible() {
    for (auto it = fifo_.begin(); it != fifo_.end(); ++it) {
        const int frame = *it;
        auto& physical = frames_[static_cast<std::size_t>(frame)];
        if (physical.free) {
            continue;
        }
        auto tableIt = tables_.find(physical.processId);
        if (tableIt == tables_.end()) {
            continue;
        }
        auto& entry = tableIt->second.entries[static_cast<std::size_t>(physical.pageNumber)];
        if (entry.pinCount > 0) {
            continue;
        }
        if (entry.swapSlot < 0) {
            auto stored = swap_.store(physical.processId, physical.pageNumber);
            if (!stored.ok) {
                return OperationResult::failure(stored.message);
            }
            entry.swapSlot = stored.value;
        }
        events_.publish(EventType::PageEvicted, "PagingManager",
                        "FIFO evicted page " + std::to_string(physical.pageNumber) + " from frame " +
                            std::to_string(frame),
                        physical.processId);
        entry.present = false;
        entry.frameNumber = -1;
        physical.free = true;
        physical.processId = -1;
        physical.pageNumber = -1;
        fifo_.erase(it);
        return OperationResult::success();
    }
    return OperationResult::failure("All resident pages are pinned");
}

OperationResult PagingManager::evictUntilFreeFrame() {
    if (findFreeFrame() >= 0) {
        return OperationResult::success();
    }
    return evictOldestEligible();
}

OperationResult PagingManager::forceEvict(int pid, int pageNumber) {
    auto tableIt = tables_.find(pid);
    if (tableIt == tables_.end()) {
        return OperationResult::failure("Unknown page table");
    }
    if (pageNumber < 0 || static_cast<std::size_t>(pageNumber) >= tableIt->second.entries.size()) {
        return OperationResult::failure("Invalid page number");
    }
    auto& entry = tableIt->second.entries[static_cast<std::size_t>(pageNumber)];
    if (!entry.present) {
        return OperationResult::success("Page already not resident");
    }
    if (entry.pinCount > 0) {
        return OperationResult::failure("Page is pinned by an active operation");
    }
    if (entry.swapSlot < 0) {
        auto stored = swap_.store(pid, pageNumber);
        if (!stored.ok) {
            return OperationResult::failure(stored.message);
        }
        entry.swapSlot = stored.value;
    }
    const int frame = entry.frameNumber;
    auto& physical = frames_[static_cast<std::size_t>(frame)];
    physical.free = true;
    physical.processId = -1;
    physical.pageNumber = -1;
    fifo_.erase(std::remove(fifo_.begin(), fifo_.end(), frame), fifo_.end());
    entry.present = false;
    entry.frameNumber = -1;
    events_.publish(EventType::PageEvicted, "PagingManager",
                    "Forced FIFO eviction of page " + std::to_string(pageNumber), pid);
    return OperationResult::success();
}

OperationResult PagingManager::releaseProcess(int pid) {
    auto tableIt = tables_.find(pid);
    if (tableIt == tables_.end()) {
        return OperationResult::failure("No page table for process");
    }
    for (auto& frame : frames_) {
        if (!frame.free && frame.processId == pid) {
            fifo_.erase(std::remove(fifo_.begin(), fifo_.end(), frame.number), fifo_.end());
            frame.free = true;
            frame.processId = -1;
            frame.pageNumber = -1;
        }
    }
    swap_.releaseProcess(pid);
    tables_.erase(tableIt);
    events_.publish(EventType::MemoryReleased, "PagingManager", "Released frames and swap pages", pid);
    return OperationResult::success();
}

AddressTranslation PagingManager::translate(int pid, std::size_t logicalAddress) const {
    AddressTranslation result;
    auto tableIt = tables_.find(pid);
    if (tableIt == tables_.end()) {
        result.explanation = "Process has no page table";
        return result;
    }
    const auto& table = tableIt->second;
    if (logicalAddress >= table.logicalSize && table.logicalSize > 0) {
        result.explanation = "Logical address is outside the process address space";
        return result;
    }
    result.pageNumber = static_cast<int>(logicalAddress / pageSize_);
    result.offset = logicalAddress % pageSize_;
    if (result.pageNumber < 0 || static_cast<std::size_t>(result.pageNumber) >= table.entries.size()) {
        result.explanation = "Page does not belong to the process";
        return result;
    }
    const auto& entry = table.entries[static_cast<std::size_t>(result.pageNumber)];
    if (!entry.present) {
        result.pageFault = true;
        result.inSwap = entry.swapSlot >= 0;
        result.explanation = result.inSwap ? "Page is in swap and must be recovered"
                                           : "No valid translation: page is not resident";
        return result;
    }
    result.ok = true;
    result.frameNumber = entry.frameNumber;
    result.physicalAddress = static_cast<std::size_t>(entry.frameNumber) * pageSize_ + result.offset;
    result.explanation = "page=" + std::to_string(result.pageNumber) +
                         " offset=" + std::to_string(result.offset) +
                         " frame=" + std::to_string(result.frameNumber) +
                         " physical=" + std::to_string(result.physicalAddress);
    return result;
}

OperationResult PagingManager::handlePageFault(int pid, int pageNumber) {
    events_.publish(EventType::PageFault, "PagingManager",
                    "Page fault on page " + std::to_string(pageNumber), pid);
    return loadPage(pid, pageNumber);
}

OperationResult PagingManager::pinPage(int pid, int pageNumber) {
    auto tableIt = tables_.find(pid);
    if (tableIt == tables_.end()) {
        return OperationResult::failure("Unknown page table");
    }
    if (pageNumber < 0 || static_cast<std::size_t>(pageNumber) >= tableIt->second.entries.size()) {
        return OperationResult::failure("Invalid page number");
    }
    auto& entry = tableIt->second.entries[static_cast<std::size_t>(pageNumber)];
    if (!entry.present) {
        auto loaded = loadPage(pid, pageNumber);
        if (!loaded.ok) {
            return loaded;
        }
    }
    ++tableIt->second.entries[static_cast<std::size_t>(pageNumber)].pinCount;
    return OperationResult::success();
}

OperationResult PagingManager::unpinPage(int pid, int pageNumber) {
    auto tableIt = tables_.find(pid);
    if (tableIt == tables_.end()) {
        return OperationResult::failure("Unknown page table");
    }
    auto& entry = tableIt->second.entries[static_cast<std::size_t>(pageNumber)];
    if (entry.pinCount > 0) {
        --entry.pinCount;
    }
    return OperationResult::success();
}

std::optional<PageTable> PagingManager::pageTable(int pid) const {
    auto it = tables_.find(pid);
    if (it == tables_.end()) {
        return std::nullopt;
    }
    return it->second;
}

std::vector<PhysicalFrame> PagingManager::frames() const {
    return frames_;
}

std::vector<int> PagingManager::fifoOrder() const {
    return std::vector<int>(fifo_.begin(), fifo_.end());
}

PagingStats PagingManager::stats() const {
    PagingStats result;
    result.ramBytes = ramBytes_;
    result.pageSize = pageSize_;
    result.frameCount = static_cast<int>(frames_.size());
    for (const auto& frame : frames_) {
        if (frame.free) {
            ++result.freeFrames;
        } else {
            ++result.usedFrames;
        }
    }
    result.swapUsed = swap_.usedCount();
    result.swapCapacity = swap_.capacity();
    return result;
}

std::string PagingManager::dumpFrames() const {
    std::ostringstream out;
    for (const auto& frame : frames_) {
        out << "frame " << frame.number << ": ";
        if (frame.free) {
            out << "FREE\n";
        } else {
            out << "pid=" << frame.processId << " page=" << frame.pageNumber << '\n';
        }
    }
    return out.str();
}

}  // namespace miniso
