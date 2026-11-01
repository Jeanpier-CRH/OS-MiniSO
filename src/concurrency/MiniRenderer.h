#pragma once

#include "core/EventSystem.h"
#include "core/OperationResult.h"

#include <pthread.h>
#include <string>
#include <vector>

namespace miniso {

class MemoryManager;

struct MiniRendererResult {
    int processId = -1;
    int rows = 0;
    int cols = 0;
    double checksum = 0;
    bool pageFaultInjected = false;
    bool recovered = false;
};

class MiniRenderer {
public:
    MiniRenderer(EventSystem& events, MemoryManager& memory);
    ~MiniRenderer();

    ValueResult<MiniRendererResult> run(int processId, int rows, int cols, bool injectPageFault);

private:
    struct Job {
        MiniRenderer* self = nullptr;
        int startRow = 0;
        int endRow = 0;
        int id = 0;
    };

    static void* worker(void* arg);
    void processRange(int startRow, int endRow);

    EventSystem& events_;
    MemoryManager& memory_;
    pthread_mutex_t mutex_{};
    int pid_ = -1;
    int rows_ = 0;
    int cols_ = 0;
    std::vector<double> matrix_;
    double checksum_ = 0;
};

}  // namespace miniso
