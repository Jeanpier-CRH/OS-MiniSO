#pragma once

#include "core/OperationResult.h"
#include "processes/ProcessControlBlock.h"

#include <map>
#include <string>
#include <vector>

namespace miniso {

class MiniSO;

struct ApplicationInstance {
    std::string appId;
    int pid = -1;
    std::vector<int> extraPids;
    std::string title;
    std::string payload;
};

class ApplicationManager {
public:
    explicit ApplicationManager(MiniSO& system);

    ValueResult<ApplicationInstance> open(const std::string& appId);
    OperationResult close(int pid);
    OperationResult addBrowserTab(int browserPid, const std::string& title);
    OperationResult closeBrowserTab(int tabPid);
    std::vector<ApplicationInstance> list() const;
    ApplicationInstance* findByPid(int pid);

private:
    MiniSO& system_;
    std::map<int, ApplicationInstance> instances_;
};

}  // namespace miniso
