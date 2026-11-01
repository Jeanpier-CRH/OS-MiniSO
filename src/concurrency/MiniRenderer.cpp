#include "concurrency/MiniRenderer.h"

#include "memory/MemoryManager.h"

#include <algorithm>

namespace miniso {

MiniRenderer::MiniRenderer(EventSystem& events, MemoryManager& memory)
    : events_(events), memory_(memory) {
    pthread_mutex_init(&mutex_, nullptr);
}

MiniRenderer::~MiniRenderer() {
    pthread_mutex_destroy(&mutex_);
}

void* MiniRenderer::worker(void* arg) {
    auto* job = static_cast<Job*>(arg);
    job->self->processRange(job->startRow, job->endRow);
    job->self->events_.publish(EventType::ThreadFinished, "MiniRenderer", "Worker finished",
                               job->self->pid_, job->id);
    return nullptr;
}

void MiniRenderer::processRange(int startRow, int endRow) {
    double local = 0;
    for (int r = startRow; r < endRow; ++r) {
        for (int c = 0; c < cols_; ++c) {
            const int index = r * cols_ + c;
            matrix_[static_cast<std::size_t>(index)] = static_cast<double>(index % 17) * 0.5;
            local += matrix_[static_cast<std::size_t>(index)];
        }
    }
    pthread_mutex_lock(&mutex_);
    checksum_ += local;
    pthread_mutex_unlock(&mutex_);
}

ValueResult<MiniRendererResult> MiniRenderer::run(int processId, int rows, int cols, bool injectPageFault) {
    if (rows <= 0 || cols <= 0) {
        return ValueResult<MiniRendererResult>::failure("Invalid matrix size");
    }
    pid_ = processId;
    rows_ = rows;
    cols_ = cols;
    matrix_.assign(static_cast<std::size_t>(rows * cols), 0.0);
    checksum_ = 0;

    bool recovered = false;
    if (injectPageFault) {
        auto table = memory_.pageTable(processId);
        if (table && !table->entries.empty()) {
            const int page = static_cast<int>(table->entries.size() - 1);
            memory_.forceEvict(processId, page);
            auto translation = memory_.translate(processId, static_cast<std::size_t>(page) *
                                                                memory_.paging().pageSize());
            if (translation.pageFault) {
                events_.publish(EventType::PageFault, "MiniRenderer",
                                "Controlled access to swapped page", processId);
                auto pin = memory_.pinPage(processId, page);
                recovered = pin.ok;
                memory_.unpinPage(processId, page);
            }
        }
    }

    Job jobs[4];
    pthread_t threads[4]{};
    bool created[4]{};
    const int chunk = (rows_ + 3) / 4;
    for (int i = 0; i < 4; ++i) {
        jobs[i].self = this;
        jobs[i].id = i + 1;
        jobs[i].startRow = i * chunk;
        jobs[i].endRow = std::min(rows_, (i + 1) * chunk);
        events_.publish(EventType::ThreadStarted, "MiniRenderer", "Worker started", processId, jobs[i].id);
        if (pthread_create(&threads[i], nullptr, &MiniRenderer::worker, &jobs[i]) != 0) {
            for (int joined = 0; joined < i; ++joined) {
                if (created[joined]) {
                    pthread_join(threads[joined], nullptr);
                }
            }
            return ValueResult<MiniRendererResult>::failure("pthread_create failed in MiniRenderer");
        }
        created[i] = true;
    }
    for (int i = 0; i < 4; ++i) {
        if (created[i]) {
            pthread_join(threads[i], nullptr);
        }
    }

    MiniRendererResult result;
    result.processId = processId;
    result.rows = rows_;
    result.cols = cols_;
    result.checksum = checksum_;
    result.pageFaultInjected = injectPageFault;
    result.recovered = recovered;
    return ValueResult<MiniRendererResult>::success(result);
}

}  // namespace miniso
