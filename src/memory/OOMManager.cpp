#include "memory/OOMManager.h"

#include <sstream>

namespace miniso {

OOMManager::OOMManager(OomPolicy policy) : policy_(policy) {}

OomDecision OOMManager::selectVictim(const std::vector<ProcessControlBlock>& processes) const {
    OomDecision decision;
    decision.triggered = true;
    double best = -1.0;
    for (const auto& pcb : processes) {
        if (pcb.state == ProcessState::Terminated) {
            continue;
        }
        if (policy_.protectSystemProcesses && pcb.systemProcess) {
            continue;
        }
        const double memoryScore = static_cast<double>(pcb.logicalMemorySize);
        const double priorityPenalty = static_cast<double>(pcb.priority);
        const double score = policy_.weightMemory * memoryScore - policy_.weightPriority * priorityPenalty;
        if (score > best) {
            best = score;
            decision.victimPid = pcb.pid;
            decision.score = score;
            std::ostringstream reason;
            reason << "OOM victim pid=" << pcb.pid << " name=" << pcb.name
                   << " memory=" << pcb.logicalMemorySize << " priority=" << pcb.priority
                   << " score=" << score;
            decision.reason = reason.str();
        }
    }
    if (decision.victimPid < 0) {
        decision.reason = "No eligible victim (system processes are protected)";
    }
    return decision;
}

}  // namespace miniso
