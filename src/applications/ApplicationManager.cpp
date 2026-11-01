#include "applications/ApplicationManager.h"

#include "core/MiniSO.h"

namespace miniso {
namespace {

CreateProcessRequest makeRequest(const std::string& name, std::size_t memory, int parent, const std::string& appId) {
    CreateProcessRequest request;
    request.name = name;
    request.logicalMemorySize = memory;
    request.priority = 1;
    request.cpuBurst = 18;
    request.parentPid = parent;
    request.applicationId = appId;
    return request;
}

}  // namespace

ApplicationManager::ApplicationManager(MiniSO& system) : system_(system) {}

ValueResult<ApplicationInstance> ApplicationManager::open(const std::string& appId) {
    std::string name = appId;
    std::size_t memory = system_.config().pageSizeBytes;
    if (appId == "MiniBrowser") {
        memory = system_.config().pageSizeBytes * 2;
    } else if (appId == "MiniEditor") {
        memory = system_.config().pageSizeBytes;
    } else if (appId == "MiniPlayer") {
        memory = system_.config().pageSizeBytes;
    } else if (appId == "MiniCalculator") {
        memory = system_.config().pageSizeBytes / 2;
        if (memory == 0) {
            memory = system_.config().pageSizeBytes;
        }
    } else if (appId == "MiniFileManager") {
        memory = system_.config().pageSizeBytes;
    } else if (appId == "MiniStress") {
        memory = system_.config().pageSizeBytes * 2;
    } else {
        return ValueResult<ApplicationInstance>::failure("Unknown application");
    }

    auto created = system_.createProcess(makeRequest(name, memory, 5, appId));
    if (!created.ok) {
        return ValueResult<ApplicationInstance>::failure(created.message);
    }
    ApplicationInstance instance;
    instance.appId = appId;
    instance.pid = created.value.pid;
    instance.title = name;
    if (appId == "MiniEditor") {
        instance.payload = "MiniSO Lab demo document\nRound Robin, paging and POSIX threads.";
    } else if (appId == "MiniCalculator") {
        instance.payload = "0";
    } else if (appId == "MiniFileManager") {
        instance.payload = "/demo\n  notes.txt\n  images/\n  reports/";
    } else if (appId == "MiniPlayer") {
        instance.payload = "paused";
    } else if (appId == "MiniBrowser") {
        instance.payload = "about:miniso";
    }
    instances_[instance.pid] = instance;
    return ValueResult<ApplicationInstance>::success(instance);
}

OperationResult ApplicationManager::close(int pid) {
    instances_.erase(pid);
    return OperationResult::success();
}

OperationResult ApplicationManager::addBrowserTab(int browserPid, const std::string& title) {
    auto parent = system_.getProcess(browserPid);
    if (!parent || parent->applicationId != "MiniBrowser") {
        return OperationResult::failure("PID is not a MiniBrowser process");
    }
    auto created = system_.createProcess(
        makeRequest("Tab " + title, system_.config().pageSizeBytes, browserPid, "MiniBrowserTab"));
    if (!created.ok) {
        return OperationResult::failure(created.message);
    }
    auto it = instances_.find(browserPid);
    if (it != instances_.end()) {
        it->second.extraPids.push_back(created.value.pid);
    }
    return OperationResult::success("Created tab pid=" + std::to_string(created.value.pid));
}

OperationResult ApplicationManager::closeBrowserTab(int tabPid) {
    return system_.terminateProcess(tabPid, false, "tab closed");
}

std::vector<ApplicationInstance> ApplicationManager::list() const {
    std::vector<ApplicationInstance> result;
    for (const auto& [pid, instance] : instances_) {
        result.push_back(instance);
    }
    return result;
}

ApplicationInstance* ApplicationManager::findByPid(int pid) {
    auto it = instances_.find(pid);
    if (it == instances_.end()) {
        return nullptr;
    }
    return &it->second;
}

}  // namespace miniso
