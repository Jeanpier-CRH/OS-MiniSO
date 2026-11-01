#include "core/MiniSO.h"

#include <gtest/gtest.h>

using namespace miniso;

TEST(Oom, StressTriggersSimulatedKiller) {
    MiniSO system;
    system.boot();
    auto result = system.runMiniStress(40, 16ull * 1024ull * 1024ull, 0, 1);
    EXPECT_TRUE(result.ok);
    EXPECT_GT(result.value, 0);
    bool sawOom = false;
    for (const auto& event : system.events().snapshot()) {
        if (event.type == EventType::OOMTriggered) {
            sawOom = true;
        }
    }
    EXPECT_TRUE(sawOom || result.value < 40);
}

TEST(Oom, SystemProcessIsProtected) {
    MiniSO system;
    system.boot();
    auto victim = system.memory().chooseVictim(system.processes(false));
    if (victim.victimPid > 0) {
        auto pcb = system.getProcess(victim.victimPid);
        ASSERT_TRUE(pcb);
        EXPECT_FALSE(pcb->systemProcess);
    }
}
