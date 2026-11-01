#include "core/EventSystem.h"

#include <sstream>

namespace miniso {

std::string eventTypeName(EventType type) {
    switch (type) {
        case EventType::ProcessCreated: return "ProcessCreated";
        case EventType::ProcessStateChanged: return "ProcessStateChanged";
        case EventType::ProcessTerminated: return "ProcessTerminated";
        case EventType::MemoryAllocated: return "MemoryAllocated";
        case EventType::MemoryReleased: return "MemoryReleased";
        case EventType::PageMapped: return "PageMapped";
        case EventType::PageFault: return "PageFault";
        case EventType::PageLoaded: return "PageLoaded";
        case EventType::PageEvicted: return "PageEvicted";
        case EventType::QuantumExpired: return "QuantumExpired";
        case EventType::ThreadStarted: return "ThreadStarted";
        case EventType::ThreadWaiting: return "ThreadWaiting";
        case EventType::ThreadProduced: return "ThreadProduced";
        case EventType::ThreadConsumed: return "ThreadConsumed";
        case EventType::ThreadFinished: return "ThreadFinished";
        case EventType::OOMTriggered: return "OOMTriggered";
        case EventType::SchedulerTick: return "SchedulerTick";
        case EventType::AddressTranslated: return "AddressTranslated";
        case EventType::ScenarioStarted: return "ScenarioStarted";
        case EventType::ScenarioFinished: return "ScenarioFinished";
        case EventType::LinuxProcess: return "LinuxProcess";
        case EventType::Error: return "Error";
        case EventType::Info: return "Info";
    }
    return "Unknown";
}

std::string eventLevelName(EventLevel level) {
    switch (level) {
        case EventLevel::Info: return "INFO";
        case EventLevel::Warning: return "WARN";
        case EventLevel::Error: return "ERROR";
        case EventLevel::Critical: return "CRIT";
    }
    return "INFO";
}

EventSystem::EventSystem(std::size_t capacity) : capacity_(capacity == 0 ? 1000 : capacity) {}

std::uint64_t EventSystem::publish(Event event) {
    std::lock_guard<std::mutex> lock(mutex_);
    event.id = nextId_++;
    event.timestamp = std::chrono::system_clock::now();
    if (events_.size() >= capacity_) {
        events_.erase(events_.begin());
    }
    events_.push_back(event);
    return event.id;
}

std::uint64_t EventSystem::publish(EventType type,
                                   std::string module,
                                   std::string message,
                                   int pid,
                                   int tid,
                                   EventLevel level,
                                   std::string extra) {
    Event event;
    event.type = type;
    event.level = level;
    event.pid = pid;
    event.tid = tid;
    event.module = std::move(module);
    event.message = std::move(message);
    event.extra = std::move(extra);
    return publish(std::move(event));
}

std::vector<Event> EventSystem::snapshot() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return events_;
}

std::vector<Event> EventSystem::since(std::uint64_t lastId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Event> result;
    for (const auto& event : events_) {
        if (event.id > lastId) {
            result.push_back(event);
        }
    }
    return result;
}

std::string EventSystem::exportText() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ostringstream out;
    for (const auto& event : events_) {
        const auto time = std::chrono::system_clock::to_time_t(event.timestamp);
        out << event.id << '\t' << eventLevelName(event.level) << '\t' << eventTypeName(event.type)
            << "\tpid=" << event.pid << "\ttid=" << event.tid << '\t' << event.module << '\t'
            << event.message;
        if (!event.extra.empty()) {
            out << '\t' << event.extra;
        }
        out << "\ttime=" << time << '\n';
    }
    return out.str();
}

void EventSystem::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    events_.clear();
}

std::uint64_t EventSystem::lastId() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return nextId_ == 0 ? 0 : nextId_ - 1;
}

}  // namespace miniso
