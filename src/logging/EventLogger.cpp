#include "logging/EventLogger.h"

#include <sstream>

namespace miniso {

EventLogger::EventLogger(EventSystem& events) : events_(events) {}

std::string EventLogger::recent(std::size_t count) const {
    auto all = events_.snapshot();
    if (all.size() > count) {
        all.erase(all.begin(), all.end() - static_cast<std::ptrdiff_t>(count));
    }
    std::ostringstream out;
    for (const auto& event : all) {
        out << '[' << eventLevelName(event.level) << "] " << eventTypeName(event.type) << " "
            << event.module << ": " << event.message << " (pid=" << event.pid << ")\n";
    }
    return out.str();
}

std::string EventLogger::exportAll() const {
    return events_.exportText();
}

std::string EventLogger::filter(EventType type) const {
    std::ostringstream out;
    for (const auto& event : events_.snapshot()) {
        if (event.type == type) {
            out << eventTypeName(event.type) << ": " << event.message << " pid=" << event.pid << '\n';
        }
    }
    return out.str();
}

}  // namespace miniso
