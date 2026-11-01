#include "concurrency/ProducerConsumer.h"
#include "core/EventSystem.h"

#include <gtest/gtest.h>

using namespace miniso;

TEST(Concurrency, TwoProducersTwoConsumers) {
    EventSystem events;
    ProducerConsumer pc(events);
    ASSERT_TRUE(pc.configure(4, 40, 2, 2).ok);
    ASSERT_TRUE(pc.start().ok);
    ASSERT_TRUE(pc.waitUntilFinished(8000));
    EXPECT_TRUE(pc.valid());
    auto stats = pc.stats();
    EXPECT_EQ(stats.produced, 40);
    EXPECT_EQ(stats.consumed, 40);
}

TEST(Concurrency, CapacityOneAndZeroItems) {
    EventSystem events;
    ProducerConsumer pc(events);
    ASSERT_TRUE(pc.configure(1, 8, 2, 2).ok);
    ASSERT_TRUE(pc.start().ok);
    ASSERT_TRUE(pc.waitUntilFinished(8000));
    EXPECT_TRUE(pc.valid());

    ProducerConsumer empty(events);
    ASSERT_TRUE(empty.configure(2, 0, 1, 1).ok);
    ASSERT_TRUE(empty.start().ok);
    ASSERT_TRUE(empty.waitUntilFinished(2000));
    EXPECT_EQ(empty.stats().produced, 0);
}

TEST(Concurrency, StopDuringWait) {
    EventSystem events;
    ProducerConsumer pc(events);
    ASSERT_TRUE(pc.configure(1, 1000, 2, 1).ok);
    ASSERT_TRUE(pc.start().ok);
    ASSERT_TRUE(pc.stop().ok);
    auto stats = pc.stats();
    EXPECT_FALSE(stats.running);
}
