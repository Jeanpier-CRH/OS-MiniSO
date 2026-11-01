#pragma once

#include "core/EventSystem.h"
#include "core/OperationResult.h"
#include "memory/PageTable.h"
#include "memory/SwapManager.h"

#include <cstddef>
#include <deque>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace miniso {

struct PhysicalFrame {
    int number = 0;
    bool free = true;
    int processId = -1;
    int pageNumber = -1;
};

struct PagingStats {
    std::size_t ramBytes = 0;
    std::size_t pageSize = 0;
    int frameCount = 0;
    int freeFrames = 0;
    int usedFrames = 0;
    std::size_t swapUsed = 0;
    std::size_t swapCapacity = 0;
};

class PagingManager {
public:
    PagingManager(EventSystem& events, std::size_t ramBytes, std::size_t pageSize, std::size_t swapPages);

    int requiredPages(std::size_t logicalSize) const;
    ValueResult<PageTable> allocateProcess(int pid, std::size_t logicalSize);
    OperationResult releaseProcess(int pid);
    AddressTranslation translate(int pid, std::size_t logicalAddress) const;
    OperationResult handlePageFault(int pid, int pageNumber);
    OperationResult pinPage(int pid, int pageNumber);
    OperationResult unpinPage(int pid, int pageNumber);
    OperationResult evictUntilFreeFrame();
    OperationResult forceEvict(int pid, int pageNumber);
    std::optional<PageTable> pageTable(int pid) const;
    std::vector<PhysicalFrame> frames() const;
    std::vector<int> fifoOrder() const;
    PagingStats stats() const;
    SwapManager& swap() { return swap_; }
    const SwapManager& swap() const { return swap_; }
    std::size_t pageSize() const { return pageSize_; }
    std::string dumpFrames() const;
    void reset(std::size_t ramBytes, std::size_t pageSize, std::size_t swapPages);

private:
    int findFreeFrame() const;
    OperationResult loadPage(int pid, int pageNumber);
    OperationResult evictOldestEligible();

    EventSystem& events_;
    std::size_t ramBytes_ = 0;
    std::size_t pageSize_ = 0;
    std::vector<PhysicalFrame> frames_;
    std::deque<int> fifo_;
    std::map<int, PageTable> tables_;
    SwapManager swap_;
};

}  // namespace miniso
