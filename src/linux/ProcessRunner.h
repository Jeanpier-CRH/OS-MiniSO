#pragma once

#include "core/EventSystem.h"
#include "core/OperationResult.h"
#include "core/SystemConfig.h"

#include <string>
#include <vector>

namespace miniso {

struct LinuxChildResult {
    int pid = -1;
    int ppid = -1;
    int exitCode = -1;
    bool execFailed = false;
    std::string output;
};

struct LinuxRunReport {
    int runnerPid = -1;
    std::vector<LinuxChildResult> children;
    std::string rawOutput;
};

class ProcessRunner {
public:
    ProcessRunner(EventSystem& events, SystemConfig config);

    ValueResult<LinuxRunReport> runDemo();

private:
    EventSystem& events_;
    SystemConfig config_;
};

}  // namespace miniso
