#pragma once

#include <cstddef>
#include <optional>
#include <vector>

namespace miniso {

struct PageTableEntry {
    int pageNumber = 0;
    int frameNumber = -1;
    int swapSlot = -1;
    bool present = false;
    bool dirty = false;
    bool referenced = false;
    int pinCount = 0;
};

struct PageTable {
    int processId = -1;
    std::size_t logicalSize = 0;
    std::size_t pageSize = 0;
    std::vector<PageTableEntry> entries;

    std::size_t internalFragmentation() const {
        if (entries.empty() || pageSize == 0) {
            return 0;
        }
        const std::size_t allocated = entries.size() * pageSize;
        if (allocated <= logicalSize) {
            return 0;
        }
        return allocated - logicalSize;
    }
};

struct AddressTranslation {
    bool ok = false;
    int pageNumber = -1;
    std::size_t offset = 0;
    int frameNumber = -1;
    std::size_t physicalAddress = 0;
    bool pageFault = false;
    bool inSwap = false;
    std::string explanation;
};

}  // namespace miniso
