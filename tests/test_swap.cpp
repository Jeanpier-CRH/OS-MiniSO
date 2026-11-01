#include "core/EventSystem.h"
#include "memory/PagingManager.h"

#include <gtest/gtest.h>

using namespace miniso;

TEST(Swap, FifoEvictionAndRecovery) {
    EventSystem events;
    const std::size_t page = 4 * 1024 * 1024;
    PagingManager paging(events, 8 * 1024 * 1024, page, 8);
    ASSERT_TRUE(paging.allocateProcess(1, 8 * 1024 * 1024).ok);
    ASSERT_TRUE(paging.allocateProcess(2, 8 * 1024 * 1024).ok);
    auto table1 = paging.pageTable(1);
    ASSERT_TRUE(table1);
    bool someEvicted = false;
    for (const auto& entry : table1->entries) {
        if (!entry.present) {
            someEvicted = true;
        }
    }
    EXPECT_TRUE(someEvicted || paging.stats().freeFrames == 0);
    ASSERT_TRUE(paging.forceEvict(2, 0).ok);
    auto fault = paging.translate(2, 0);
    EXPECT_TRUE(fault.pageFault);
    ASSERT_TRUE(paging.handlePageFault(2, 0).ok);
    auto recovered = paging.translate(2, 0);
    EXPECT_TRUE(recovered.ok);
}

TEST(Swap, PinnedPageIsNotEvicted) {
    EventSystem events;
    const std::size_t page = 4 * 1024 * 1024;
    PagingManager paging(events, 4 * 1024 * 1024, page, 4);
    ASSERT_TRUE(paging.allocateProcess(1, page).ok);
    ASSERT_TRUE(paging.pinPage(1, 0).ok);
    EXPECT_FALSE(paging.forceEvict(1, 0).ok);
    ASSERT_TRUE(paging.unpinPage(1, 0).ok);
    EXPECT_TRUE(paging.forceEvict(1, 0).ok);
}
