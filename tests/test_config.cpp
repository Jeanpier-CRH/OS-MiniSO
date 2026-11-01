#include "core/SystemConfig.h"

#include <gtest/gtest.h>

#include <fstream>
#include <string>

using namespace miniso;

TEST(SystemConfig, LoadsDefaultJsonAndValidatesInvalidValues) {
    SystemConfig config;
    std::string error;
    ASSERT_TRUE(loadSystemConfig(std::string(MINISO_SOURCE_DIR) + "/config/default_config.json", config, &error))
        << error;
    EXPECT_EQ(config.ramBytes, 128ull * 1024ull * 1024ull);
    EXPECT_EQ(config.pageSizeBytes, 4ull * 1024ull * 1024ull);
    EXPECT_EQ(config.quantum, 3);
    EXPECT_EQ(config.firstUserPid, 100);

    const std::string invalidPath = std::string(MINISO_SOURCE_DIR) + "/build/test_invalid_config.json";
    {
        std::ofstream invalid(invalidPath);
        invalid << "{\"ramBytes\":0,\"pageSizeBytes\":4,\"swapPages\":64,\"quantum\":3,"
                   "\"firstUserPid\":100,\"eventCapacity\":1000,"
                   "\"systemProcessMemoryBytes\":4,\"weightMemory\":1.0,"
                   "\"weightPriority\":2.0,\"protectSystemProcesses\":true}";
    }
    SystemConfig invalidConfig;
    EXPECT_FALSE(loadSystemConfig(invalidPath, invalidConfig, &error));
    EXPECT_FALSE(error.empty());
    std::remove(invalidPath.c_str());
}
