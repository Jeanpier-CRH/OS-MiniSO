#pragma once

#include "core/SystemConfig.h"
#include "processes/ProcessControlBlock.h"

#include <optional>
#include <string>
#include <vector>

namespace miniso {

struct OomDecision {
    bool triggered = false;
    int victimPid = -1;
    double score = 0;
    std::string reason;
};

class OOMManager {
public:
    explicit OOMManager(OomPolicy policy);

    OomDecision selectVictim(const std::vector<ProcessControlBlock>& processes) const;
    void setPolicy(OomPolicy policy) { policy_ = policy; }
    OomPolicy policy() const { return policy_; }

private:
    OomPolicy policy_;
};

}  // namespace miniso
