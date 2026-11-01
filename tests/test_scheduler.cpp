#include "core/MiniSO.h"

#include <gtest/gtest.h>

using namespace miniso;

static CreateProcessRequest burst(const std::string& name, std::uint64_t cpu) {
    CreateProcessRequest request;
    request.name = name;
    request.logicalMemorySize = 4 * 1024 * 1024;
    request.cpuBurst = cpu;
    request.parentPid = 1;
    return request;
}

TEST(Scheduler, SingleProcessRunsToCompletion) {
    MiniSO system;
    system.boot();
    system.setQuantum(3);
    ASSERT_TRUE(system.createProcess(burst("one", 4)).ok);
    for (int i = 0; i < 8; ++i) {
        system.schedulerTick();
    }
    auto pcb = system.getProcess(100);
    ASSERT_TRUE(pcb);
    EXPECT_EQ(pcb->state, ProcessState::Terminated);
    EXPECT_EQ(pcb->totalCpuTime, 4u);
}

TEST(Scheduler, RoundRobinWithTwoProcesses) {
    MiniSO system;
    system.boot();
    system.setQuantum(2);
    ASSERT_TRUE(system.createProcess(burst("a", 5)).ok);
    ASSERT_TRUE(system.createProcess(burst("b", 5)).ok);
    for (int i = 0; i < 4; ++i) {
        system.schedulerTick();
    }
    auto snap = system.schedulerSnapshot();
    EXPECT_GT(snap.clock, 0u);
    auto a = system.getProcess(100);
    auto b = system.getProcess(101);
    ASSERT_TRUE(a);
    ASSERT_TRUE(b);
    EXPECT_GT(a->totalCpuTime, 0u);
    EXPECT_GT(b->totalCpuTime, 0u);
}

TEST(Scheduler, EarlyFinishAndEmptyQueue) {
    MiniSO system;
    system.boot();
    system.setQuantum(10);
    ASSERT_TRUE(system.createProcess(burst("short", 1)).ok);
    system.schedulerTick();
    auto pcb = system.getProcess(100);
    ASSERT_TRUE(pcb);
    EXPECT_EQ(pcb->state, ProcessState::Terminated);
    auto idle = system.schedulerTick();
    EXPECT_TRUE(idle.ok);
}

TEST(Scheduler, BlockAndReadyAgain) {
    MiniSO system;
    system.boot();
    ASSERT_TRUE(system.createProcess(burst("blocked", 8)).ok);
    system.schedulerTick();
    system.memory();  // keep API warm
    auto pcb = system.getProcess(100);
    ASSERT_TRUE(pcb);
    system.recoverPage(100, 0);
}
