#pragma once

#include <cstddef>
#include <string>

namespace miniso {

struct OomPolicy {
    double weightMemory = 1.0;
    double weightPriority = 2.0;
    bool protectSystemProcesses = true;
};

struct SystemConfig {
    std::size_t ramBytes = 128ull * 1024ull * 1024ull;
    std::size_t pageSizeBytes = 4ull * 1024ull * 1024ull;
    std::size_t swapPages = 64;
    int quantum = 3;
    int firstUserPid = 100;
    std::size_t eventCapacity = 10000;
    std::size_t systemProcessMemoryBytes = 4ull * 1024ull * 1024ull;
    OomPolicy oom;
    std::string processRunnerPath = "process_runner";
    std::string processChildPath = "process_child";
};

inline std::size_t megabytes(std::size_t mb) {
    return mb * 1024ull * 1024ull;
}

inline std::size_t bytesToMb(std::size_t bytes) {
    return bytes / (1024ull * 1024ull);
}

}  // namespace miniso
