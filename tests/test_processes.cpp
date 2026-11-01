#include "core/MiniSO.h"

#include <gtest/gtest.h>

using namespace miniso;

TEST(Processes, BootCreatesSystemProcesses) {
    MiniSO system;
    system.boot();
    for (int pid = 1; pid <= 6; ++pid) {
        auto pcb = system.getProcess(pid);
        ASSERT_TRUE(pcb.has_value());
        EXPECT_TRUE(pcb->systemProcess);
        EXPECT_NE(pcb->state, ProcessState::Terminated);
    }
}

TEST(Processes, CreateAssignsPidAndPpid) {
    MiniSO system;
    system.boot();
    CreateProcessRequest request;
    request.name = "alpha";
    request.logicalMemorySize = 4 * 1024 * 1024;
    request.parentPid = 1;
    request.cpuBurst = 5;
    auto first = system.createProcess(request);
    auto second = system.createProcess(request);
    ASSERT_TRUE(first.ok);
    ASSERT_TRUE(second.ok);
    EXPECT_EQ(first.value.pid, 100);
    EXPECT_EQ(second.value.pid, 101);
    EXPECT_EQ(first.value.parentPid, 1);
    EXPECT_EQ(first.value.state, ProcessState::Ready);
}

TEST(Processes, TreeAndTerminateWithChildren) {
    MiniSO system;
    system.boot();
    auto browser = system.openApplication("MiniBrowser");
    ASSERT_TRUE(browser.ok);
    ASSERT_TRUE(system.addBrowserTab(browser.value.pid, "A").ok);
    ASSERT_TRUE(system.addBrowserTab(browser.value.pid, "B").ok);
    const std::string tree = system.processTree();
    EXPECT_NE(tree.find("MiniBrowser"), std::string::npos);
    EXPECT_NE(tree.find("Tab A"), std::string::npos);
    auto terminated = system.terminateProcess(browser.value.pid, true, "test");
    EXPECT_TRUE(terminated.ok);
    auto parent = system.getProcess(browser.value.pid);
    ASSERT_TRUE(parent);
    EXPECT_EQ(parent->state, ProcessState::Terminated);
}

TEST(Processes, RejectsUserKillOfSystemProcess) {
    MiniSO system;
    system.boot();
    auto result = system.terminateProcess(1, false, "user");
    EXPECT_FALSE(result.ok);
}
