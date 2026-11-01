#include "concurrency/ProducerConsumer.h"

#include <cerrno>
#include <ctime>
#include <set>

namespace miniso {

ProducerConsumer::ProducerConsumer(EventSystem& events) : events_(events) {
    pthread_mutex_init(&mutex_, nullptr);
    pthread_cond_init(&notEmpty_, nullptr);
    pthread_cond_init(&notFull_, nullptr);
    pthread_cond_init(&done_, nullptr);
    syncReady_ = true;
}

ProducerConsumer::~ProducerConsumer() {
    stop();
    if (syncReady_) {
        pthread_mutex_destroy(&mutex_);
        pthread_cond_destroy(&notEmpty_);
        pthread_cond_destroy(&notFull_);
        pthread_cond_destroy(&done_);
    }
}

OperationResult ProducerConsumer::configure(int capacity, int totalItems, int producers, int consumers) {
    pthread_mutex_lock(&mutex_);
    if (running_) {
        pthread_mutex_unlock(&mutex_);
        return OperationResult::failure("Stop the scenario before reconfiguring");
    }
    if (capacity < 1 || producers < 1 || consumers < 1 || totalItems < 0) {
        pthread_mutex_unlock(&mutex_);
        return OperationResult::failure("Invalid producer-consumer configuration");
    }
    capacity_ = capacity;
    totalItems_ = totalItems;
    producerCount_ = producers;
    consumerCount_ = consumers;
    buffer_.assign(static_cast<std::size_t>(capacity_), -1);
    pthread_mutex_unlock(&mutex_);
    return OperationResult::success();
}

void* ProducerConsumer::producerEntry(void* arg) {
    auto* worker = static_cast<WorkerArg*>(arg);
    worker->self->producerLoop(worker->logicalId);
    return nullptr;
}

void* ProducerConsumer::consumerEntry(void* arg) {
    auto* worker = static_cast<WorkerArg*>(arg);
    worker->self->consumerLoop(worker->logicalId);
    return nullptr;
}

void ProducerConsumer::setState(int logicalId, ThreadRunState state) {
    for (auto& thread : threads_) {
        if (thread.logicalId == logicalId) {
            thread.state = state;
            return;
        }
    }
}

void ProducerConsumer::producerLoop(int logicalId) {
    events_.publish(EventType::ThreadStarted, "ProducerConsumer", "Producer started", -1, logicalId);
    while (true) {
        pthread_mutex_lock(&mutex_);
        while (running_ && !stopRequested_ && count_ == capacity_ && nextToProduce_ < totalItems_) {
            setState(logicalId, ThreadRunState::Waiting);
            events_.publish(EventType::ThreadWaiting, "ProducerConsumer", "Producer waiting (buffer full)",
                            -1, logicalId);
            pthread_cond_wait(&notFull_, &mutex_);
        }
        if (!running_ || stopRequested_ || nextToProduce_ >= totalItems_) {
            if (nextToProduce_ >= totalItems_) {
                productionFinished_ = true;
            }
            setState(logicalId, ThreadRunState::Finished);
            pthread_cond_broadcast(&notEmpty_);
            pthread_cond_broadcast(&done_);
            pthread_mutex_unlock(&mutex_);
            events_.publish(EventType::ThreadFinished, "ProducerConsumer", "Producer finished", -1, logicalId);
            return;
        }
        const int item = nextToProduce_++;
        buffer_[static_cast<std::size_t>(tail_)] = item;
        tail_ = (tail_ + 1) % capacity_;
        ++count_;
        ++produced_;
        for (auto& thread : threads_) {
            if (thread.logicalId == logicalId) {
                thread.lastItem = item;
                thread.state = ThreadRunState::Running;
            }
        }
        events_.publish(EventType::ThreadProduced, "ProducerConsumer",
                        "Produced item " + std::to_string(item), -1, logicalId,
                        EventLevel::Info, "buffer=" + std::to_string(count_));
        pthread_cond_signal(&notEmpty_);
        pthread_mutex_unlock(&mutex_);
    }
}

void ProducerConsumer::consumerLoop(int logicalId) {
    events_.publish(EventType::ThreadStarted, "ProducerConsumer", "Consumer started", -1, logicalId);
    while (true) {
        pthread_mutex_lock(&mutex_);
        while (running_ && !stopRequested_ && count_ == 0 &&
               !(productionFinished_ && produced_ >= totalItems_)) {
            setState(logicalId, ThreadRunState::Waiting);
            events_.publish(EventType::ThreadWaiting, "ProducerConsumer", "Consumer waiting (buffer empty)",
                            -1, logicalId);
            pthread_cond_wait(&notEmpty_, &mutex_);
        }
        if (stopRequested_ || (count_ == 0 && (productionFinished_ || produced_ >= totalItems_))) {
            setState(logicalId, ThreadRunState::Finished);
            pthread_cond_broadcast(&done_);
            pthread_mutex_unlock(&mutex_);
            events_.publish(EventType::ThreadFinished, "ProducerConsumer", "Consumer finished", -1, logicalId);
            return;
        }
        const int item = buffer_[static_cast<std::size_t>(head_)];
        buffer_[static_cast<std::size_t>(head_)] = -1;
        head_ = (head_ + 1) % capacity_;
        --count_;
        ++consumed_;
        consumedIds_.push_back(item);
        for (auto& thread : threads_) {
            if (thread.logicalId == logicalId) {
                thread.lastItem = item;
                thread.state = ThreadRunState::Running;
            }
        }
        events_.publish(EventType::ThreadConsumed, "ProducerConsumer",
                        "Consumed item " + std::to_string(item), -1, logicalId,
                        EventLevel::Info, "buffer=" + std::to_string(count_));
        pthread_cond_signal(&notFull_);
        if (consumed_ >= totalItems_) {
            pthread_cond_broadcast(&done_);
        }
        pthread_mutex_unlock(&mutex_);
    }
}

OperationResult ProducerConsumer::start() {
    pthread_mutex_lock(&mutex_);
    if (running_) {
        pthread_mutex_unlock(&mutex_);
        return OperationResult::failure("Already running");
    }
    buffer_.assign(static_cast<std::size_t>(capacity_), -1);
    head_ = tail_ = count_ = 0;
    nextToProduce_ = produced_ = consumed_ = 0;
    stopRequested_ = false;
    productionFinished_ = totalItems_ == 0;
    consumedIds_.clear();
    threads_.clear();
    nativeThreads_.clear();
    args_.clear();
    running_ = true;

    int logical = 1;
    args_.reserve(static_cast<std::size_t>(producerCount_ + consumerCount_));
    for (int i = 0; i < producerCount_; ++i) {
        ThreadView view;
        view.logicalId = logical++;
        view.producer = true;
        view.state = ThreadRunState::Created;
        threads_.push_back(view);
        args_.push_back(WorkerArg{this, view.logicalId, true});
    }
    for (int i = 0; i < consumerCount_; ++i) {
        ThreadView view;
        view.logicalId = logical++;
        view.producer = false;
        view.state = ThreadRunState::Created;
        threads_.push_back(view);
        args_.push_back(WorkerArg{this, view.logicalId, false});
    }
    pthread_mutex_unlock(&mutex_);

    for (std::size_t i = 0; i < args_.size(); ++i) {
        pthread_t handle{};
        auto entry = args_[i].producer ? &ProducerConsumer::producerEntry
                                       : &ProducerConsumer::consumerEntry;
        if (pthread_create(&handle, nullptr, entry, &args_[i]) != 0) {
            stop();
            return OperationResult::failure("pthread_create failed");
        }
        nativeThreads_.push_back(handle);
        pthread_mutex_lock(&mutex_);
        threads_[i].nativeId = handle;
        threads_[i].state = ThreadRunState::Running;
        pthread_mutex_unlock(&mutex_);
    }
    if (totalItems_ == 0) {
        pthread_mutex_lock(&mutex_);
        productionFinished_ = true;
        pthread_cond_broadcast(&notEmpty_);
        pthread_mutex_unlock(&mutex_);
    }
    return OperationResult::success();
}

OperationResult ProducerConsumer::stop() {
    pthread_mutex_lock(&mutex_);
    if (!running_ && nativeThreads_.empty()) {
        pthread_mutex_unlock(&mutex_);
        return OperationResult::success();
    }
    stopRequested_ = true;
    productionFinished_ = true;
    pthread_cond_broadcast(&notEmpty_);
    pthread_cond_broadcast(&notFull_);
    pthread_cond_broadcast(&done_);
    auto joining = nativeThreads_;
    pthread_mutex_unlock(&mutex_);

    for (auto handle : joining) {
        pthread_join(handle, nullptr);
    }

    pthread_mutex_lock(&mutex_);
    nativeThreads_.clear();
    running_ = false;
    pthread_mutex_unlock(&mutex_);
    return OperationResult::success();
}

bool ProducerConsumer::waitUntilFinished(int timeoutMs) {
    pthread_mutex_lock(&mutex_);
    if (totalItems_ == 0) {
        const bool done = consumed_ == produced_;
        pthread_mutex_unlock(&mutex_);
        return done;
    }
    timespec deadline{};
    clock_gettime(CLOCK_REALTIME, &deadline);
    deadline.tv_sec += timeoutMs / 1000;
    deadline.tv_nsec += (timeoutMs % 1000) * 1000000L;
    if (deadline.tv_nsec >= 1000000000L) {
        deadline.tv_sec += 1;
        deadline.tv_nsec -= 1000000000L;
    }
    while (consumed_ < totalItems_ && !stopRequested_) {
        const int rc = pthread_cond_timedwait(&done_, &mutex_, &deadline);
        if (rc == ETIMEDOUT) {
            pthread_mutex_unlock(&mutex_);
            return false;
        }
    }
    const bool ok = consumed_ >= totalItems_;
    pthread_mutex_unlock(&mutex_);
    if (ok) {
        stop();
    }
    return ok;
}

ProducerConsumerStats ProducerConsumer::stats() const {
    pthread_mutex_lock(&mutex_);
    ProducerConsumerStats snapshot;
    snapshot.capacity = capacity_;
    snapshot.produced = produced_;
    snapshot.consumed = consumed_;
    snapshot.inBuffer = count_;
    snapshot.running = running_;
    snapshot.productionFinished = productionFinished_;
    snapshot.threads = threads_;
    for (int i = 0; i < count_; ++i) {
        snapshot.buffer.push_back(buffer_[static_cast<std::size_t>((head_ + i) % capacity_)]);
    }
    pthread_mutex_unlock(&mutex_);
    return snapshot;
}

bool ProducerConsumer::valid() const {
    pthread_mutex_lock(&mutex_);
    std::set<int> ids(consumedIds_.begin(), consumedIds_.end());
    const bool ok = produced_ == consumed_ &&
                    static_cast<int>(ids.size()) == consumed_ &&
                    count_ == 0;
    pthread_mutex_unlock(&mutex_);
    return ok;
}

}  // namespace miniso
