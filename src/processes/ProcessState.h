#pragma once

#include <string>

namespace miniso {

enum class ProcessState { New, Ready, Running, Blocked, Terminated };

inline std::string processStateName(ProcessState state) {
    switch (state) {
        case ProcessState::New: return "NEW";
        case ProcessState::Ready: return "READY";
        case ProcessState::Running: return "RUNNING";
        case ProcessState::Blocked: return "BLOCKED";
        case ProcessState::Terminated: return "TERMINATED";
    }
    return "UNKNOWN";
}

inline bool isValidStateTransition(ProcessState from, ProcessState to) {
    if (from == to) {
        return true;
    }
    switch (from) {
        case ProcessState::New:
            return to == ProcessState::Ready || to == ProcessState::Terminated;
        case ProcessState::Ready:
            return to == ProcessState::Running || to == ProcessState::Blocked ||
                   to == ProcessState::Terminated;
        case ProcessState::Running:
            return to == ProcessState::Ready || to == ProcessState::Blocked ||
                   to == ProcessState::Terminated;
        case ProcessState::Blocked:
            return to == ProcessState::Ready || to == ProcessState::Terminated;
        case ProcessState::Terminated:
            return false;
    }
    return false;
}

}  // namespace miniso
