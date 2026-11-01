#pragma once

#include "processes/ProcessState.h"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace miniso {

struct ProcessControlBlock {
    int pid = -1;
    int parentPid = 0;

    std::string name;
    ProcessState state = ProcessState::New;

    int priority = 0;

    std::size_t logicalMemorySize = 0;
    std::size_t residentMemorySize = 0;

    int requiredPages = 0;
    int assignedFrames = 0;

    int logicalThreadCount = 1;

    std::uint64_t totalCpuTime = 0;
    std::uint64_t remainingCpuTime = 0;
    std::uint64_t quantumRemaining = 0;

    std::chrono::system_clock::time_point creationTime{};
    std::optional<std::chrono::system_clock::time_point> terminationTime;

    std::vector<int> childPids;

    std::string blockingReason;
    int exitCode = 0;
    bool systemProcess = false;

    std::optional<std::size_t> partitionStart;
    std::optional<std::size_t> partitionSize;

    std::string applicationId;
    std::string terminationReason;
};

}  // namespace miniso
