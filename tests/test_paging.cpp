#include "core/EventSystem.h"
#include "memory/PagingManager.h"

#include <cmath>
#include <gtest/gtest.h>

using namespace miniso;

TEST(Paging, NonContiguousFramesAndTranslation) {
    EventSystem events;
    const std::size_t page = 4 * 1024 * 1024;
    PagingManager paging(events, 12 * 1024 * 1024, page, 16);
    ASSERT_TRUE(paging.allocateProcess(10, page).ok);
    ASSERT_TRUE(paging.allocateProcess(11, page).ok);
    ASSERT_TRUE(paging.allocateProcess(12, page).ok);
    ASSERT_TRUE(paging.releaseProcess(10).ok);
    ASSERT_TRUE(paging.releaseProcess(12).ok);
    auto recycled = paging.allocateProcess(13, 2 * page);
    ASSERT_TRUE(recycled.ok);
    auto table = paging.pageTable(13);
    ASSERT_TRUE(table);
    ASSERT_EQ(table->entries.size(), 2u);
    ASSERT_TRUE(table->entries[0].present);
    ASSERT_TRUE(table->entries[1].present);
    EXPECT_NE(std::abs(table->entries[1].frameNumber - table->entries[0].frameNumber), 1);
}

TEST(Paging, AddressTranslationCases) {
    EventSystem events;
    const std::size_t page = 4 * 1024 * 1024;
    PagingManager paging(events, 32 * 1024 * 1024, page, 16);
    ASSERT_TRUE(paging.allocateProcess(7, 10 * 1024 * 1024).ok);
    auto ok = paging.translate(7, page + 128);
    EXPECT_TRUE(ok.ok);
    EXPECT_EQ(ok.pageNumber, 1);
    EXPECT_EQ(ok.offset, 128u);
    EXPECT_EQ(ok.physicalAddress, static_cast<std::size_t>(ok.frameNumber) * page + 128);

    auto invalid = paging.translate(7, 20 * 1024 * 1024);
    EXPECT_FALSE(invalid.ok);

    ASSERT_TRUE(paging.forceEvict(7, 0).ok);
    auto fault = paging.translate(7, 10);
    EXPECT_TRUE(fault.pageFault);
}

TEST(Paging, InternalFragmentation) {
    EventSystem events;
    const std::size_t page = 4 * 1024 * 1024;
    PagingManager paging(events, 32 * 1024 * 1024, page, 8);
    auto table = paging.allocateProcess(3, 10 * 1024 * 1024);
    ASSERT_TRUE(table.ok);
    EXPECT_EQ(table.value.entries.size(), 3u);
    EXPECT_EQ(table.value.internalFragmentation(), 2 * 1024 * 1024);
}
