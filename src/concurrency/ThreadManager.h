#pragma once

#include "concurrency/ProducerConsumer.h"
#include "concurrency/MiniRenderer.h"

namespace miniso {

class ThreadManager {
public:
    ThreadManager(EventSystem& events, MemoryManager& memory)
        : producerConsumer_(events), renderer_(events, memory) {}

    ProducerConsumer& producerConsumer() { return producerConsumer_; }
    MiniRenderer& renderer() { return renderer_; }

private:
    ProducerConsumer producerConsumer_;
    MiniRenderer renderer_;
};

}  // namespace miniso
