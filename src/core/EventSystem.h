#pragma once

#include <chrono>
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

namespace miniso {

enum class EventType {
    ProcessCreated,
    ProcessStateChanged,
    ProcessTerminated,
    MemoryAllocated,
    MemoryReleased,
    PageMapped,
    PageFault,
    PageLoaded,
    PageEvicted,
    QuantumExpired,
    ThreadStarted,
    ThreadWaiting,
    ThreadProduced,
    ThreadConsumed,
    ThreadFinished,
    OOMTriggered,
    SchedulerTick,
    AddressTranslated,
    ScenarioStarted,
    ScenarioFinished,
    LinuxProcess,
    Error,
    Info
};

enum class EventLevel { Info, Warning, Error, Critical };

struct Event {
    std::uint64_t id = 0;
    EventType type = EventType::Info;
    EventLevel level = EventLevel::Info;
    std::chrono::system_clock::time_point timestamp{};
    int pid = -1;
    int tid = -1;
    std::string module;
    std::string message;
    std::string extra;
};

std::string eventTypeName(EventType type);
std::string eventLevelName(EventLevel level);

class EventSystem {
public:
    explicit EventSystem(std::size_t capacity = 10000);

    std::uint64_t publish(Event event);
    std::uint64_t publish(EventType type,
                          std::string module,
                          std::string message,
                          int pid = -1,
                          int tid = -1,
                          EventLevel level = EventLevel::Info,
                          std::string extra = {});

    std::vector<Event> snapshot() const;
    std::vector<Event> since(std::uint64_t lastId) const;
    std::string exportText() const;
    void clear();
    std::uint64_t lastId() const;

private:
    mutable std::mutex mutex_;
    std::size_t capacity_;
    std::uint64_t nextId_ = 1;
    std::vector<Event> events_;
};

}  // namespace miniso
