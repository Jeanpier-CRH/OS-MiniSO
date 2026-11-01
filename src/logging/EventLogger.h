#pragma once

#include "core/EventSystem.h"

#include <string>

namespace miniso {

class EventLogger {
public:
    explicit EventLogger(EventSystem& events);

    std::string recent(std::size_t count = 50) const;
    std::string exportAll() const;
    std::string filter(EventType type) const;

private:
    EventSystem& events_;
};

}  // namespace miniso
