#pragma once

#include "core/EventSystem.h"
#include "core/OperationResult.h"

#include <atomic>
#include <cstdint>
#include <pthread.h>
#include <string>
#include <vector>

namespace miniso {

enum class ThreadRunState { Created, Running, Waiting, Finished };

struct ThreadView {
    int logicalId = 0;
    pthread_t nativeId{};
    bool producer = false;
    ThreadRunState state = ThreadRunState::Created;
    int lastItem = -1;
};

struct ProducerConsumerStats {
    int capacity = 0;
    int produced = 0;
    int consumed = 0;
    int inBuffer = 0;
    bool running = false;
    bool productionFinished = false;
    std::vector<int> buffer;
    std::vector<ThreadView> threads;
};

class ProducerConsumer {
public:
    explicit ProducerConsumer(EventSystem& events);
    ~ProducerConsumer();

    ProducerConsumer(const ProducerConsumer&) = delete;
    ProducerConsumer& operator=(const ProducerConsumer&) = delete;

    OperationResult configure(int capacity, int totalItems, int producers = 2, int consumers = 2);
    OperationResult start();
    OperationResult stop();
    bool waitUntilFinished(int timeoutMs = 5000);
    ProducerConsumerStats stats() const;
    bool valid() const;

private:
    struct WorkerArg {
        ProducerConsumer* self = nullptr;
        int logicalId = 0;
        bool producer = false;
    };

    static void* producerEntry(void* arg);
    static void* consumerEntry(void* arg);
    void producerLoop(int logicalId);
    void consumerLoop(int logicalId);
    void setState(int logicalId, ThreadRunState state);

    EventSystem& events_;
    mutable pthread_mutex_t mutex_{};
    pthread_cond_t notEmpty_{};
    pthread_cond_t notFull_{};
    pthread_cond_t done_{};
    bool syncReady_ = false;

    int capacity_ = 4;
    int totalItems_ = 20;
    int producerCount_ = 2;
    int consumerCount_ = 2;
    std::vector<int> buffer_;
    int head_ = 0;
    int tail_ = 0;
    int count_ = 0;
    int nextToProduce_ = 0;
    int produced_ = 0;
    int consumed_ = 0;
    bool running_ = false;
    bool stopRequested_ = false;
    bool productionFinished_ = false;
    std::vector<ThreadView> threads_;
    std::vector<pthread_t> nativeThreads_;
    std::vector<WorkerArg> args_;
    std::vector<int> consumedIds_;
};

}  // namespace miniso
