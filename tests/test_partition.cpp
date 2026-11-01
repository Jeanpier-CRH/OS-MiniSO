#include "memory/PartitionManager.h"

#include <gtest/gtest.h>

using namespace miniso;

TEST(Partition, FirstFitSplitAndCoalesce) {
    PartitionManager manager(100);
    auto a = manager.allocate(30, 1);
    auto b = manager.allocate(20, 2);
    ASSERT_TRUE(a.ok);
    ASSERT_TRUE(b.ok);
    EXPECT_EQ(a.value.startAddress, 0u);
    EXPECT_EQ(b.value.startAddress, 30u);
    EXPECT_TRUE(manager.releaseByAddress(0).ok);
    auto c = manager.allocate(10, 3);
    ASSERT_TRUE(c.ok);
    EXPECT_EQ(c.value.startAddress, 0u);
    EXPECT_TRUE(manager.releaseByProcess(3).ok);
    EXPECT_TRUE(manager.releaseByProcess(2).ok);
    auto stats = manager.stats();
    EXPECT_EQ(stats.free, 100u);
    EXPECT_EQ(stats.freeBlocks, 1u);
}

TEST(Partition, ExternalFragmentation) {
    PartitionManager manager(100);
    ASSERT_TRUE(manager.allocate(40, 1).ok);
    ASSERT_TRUE(manager.allocate(40, 2).ok);
    ASSERT_TRUE(manager.releaseByProcess(1).ok);
    ASSERT_TRUE(manager.allocate(20, 3).ok);
    EXPECT_TRUE(manager.wouldFailDueToFragmentation(50));
    auto fail = manager.allocate(50, 4);
    EXPECT_FALSE(fail.ok);
}

TEST(Partition, RejectsZeroAndDoubleFree) {
    PartitionManager manager(50);
    EXPECT_FALSE(manager.allocate(0, 1).ok);
    auto block = manager.allocate(10, 1);
    ASSERT_TRUE(block.ok);
    EXPECT_TRUE(manager.releaseByAddress(block.value.startAddress).ok);
    EXPECT_FALSE(manager.releaseByAddress(block.value.startAddress).ok);
}
