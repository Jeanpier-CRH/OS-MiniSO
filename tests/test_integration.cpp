#include "core/MiniSO.h"

#include <gtest/gtest.h>

using namespace miniso;

TEST(Integration, OpenCloseApplicationUpdatesMemoryAndLog) {
    MiniSO system;
    system.boot();
    const auto before = system.memoryStatus().paging.usedFrames;
    auto editor = system.openApplication("MiniEditor");
    ASSERT_TRUE(editor.ok);
    EXPECT_GT(system.memoryStatus().paging.usedFrames, before);
    EXPECT_TRUE(system.closeApplication(editor.value.pid).ok);
    auto pcb = system.getProcess(editor.value.pid);
    ASSERT_TRUE(pcb);
    EXPECT_EQ(pcb->state, ProcessState::Terminated);
    EXPECT_FALSE(system.logger().exportAll().empty());
}

TEST(Integration, ScenarioAAndLinuxDemo) {
    MiniSO system;
    system.boot();
    const auto reportA = system.runScenarioA();
    EXPECT_NE(reportA.find("MiniBrowser"), std::string::npos);
    const auto reportD = system.runScenarioD();
    EXPECT_NE(reportD.find("produced="), std::string::npos);
    auto linux = system.runLinuxDemo();
    ASSERT_TRUE(linux.ok) << linux.message;
    EXPECT_GE(linux.value.children.size(), 2u);
}

TEST(Integration, ShutdownStopsSystem) {
    MiniSO system;
    system.boot();
    ASSERT_TRUE(system.openApplication("MiniCalculator").ok);
    system.shutdown();
    for (const auto& pcb : system.processes(true)) {
        EXPECT_EQ(pcb.state, ProcessState::Terminated);
    }
}
