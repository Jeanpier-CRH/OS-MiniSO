#include "core/SystemConfig.h"

#include <charconv>
#include <fstream>
#include <sstream>
#include <string_view>

namespace miniso {
namespace {

bool findValue(const std::string& json, const std::string& key, std::string& value) {
    const std::string token = "\"" + key + "\"";
    const auto tokenPos = json.find(token);
    if (tokenPos == std::string::npos) {
        return false;
    }
    const auto colonPos = json.find(':', tokenPos + token.size());
    if (colonPos == std::string::npos) {
        return false;
    }

    std::size_t start = colonPos + 1;
    while (start < json.size() && (json[start] == ' ' || json[start] == '\t' || json[start] == '\n' || json[start] == '\r')) {
        ++start;
    }
    std::size_t end = start;
    while (end < json.size() && json[end] != ',' && json[end] != '}' && json[end] != '\n') {
        ++end;
    }
    while (end > start && (json[end - 1] == ' ' || json[end - 1] == '\t' || json[end - 1] == '\r')) {
        --end;
    }
    value = json.substr(start, end - start);
    return true;
}

bool parseUnsigned(const std::string& json, const std::string& key, std::size_t& output) {
    std::string value;
    if (!findValue(json, key, value)) {
        return false;
    }
    unsigned long long parsed = 0;
    const char* begin = value.data();
    const char* finish = value.data() + value.size();
    const auto result = std::from_chars(begin, finish, parsed);
    if (result.ec != std::errc{} || result.ptr != finish) {
        return false;
    }
    output = static_cast<std::size_t>(parsed);
    return true;
}

bool parseInt(const std::string& json, const std::string& key, int& output) {
    std::string value;
    if (!findValue(json, key, value)) {
        return false;
    }
    int parsed = 0;
    const char* begin = value.data();
    const char* finish = value.data() + value.size();
    const auto result = std::from_chars(begin, finish, parsed);
    if (result.ec != std::errc{} || result.ptr != finish) {
        return false;
    }
    output = parsed;
    return true;
}

bool parseDouble(const std::string& json, const std::string& key, double& output) {
    std::string value;
    if (!findValue(json, key, value)) {
        return false;
    }
    try {
        std::size_t consumed = 0;
        output = std::stod(value, &consumed);
        return consumed == value.size();
    } catch (...) {
        return false;
    }
}

bool parseBool(const std::string& json, const std::string& key, bool& output) {
    std::string value;
    if (!findValue(json, key, value)) {
        return false;
    }
    if (value == "true") {
        output = true;
        return true;
    }
    if (value == "false") {
        output = false;
        return true;
    }
    return false;
}

}  // namespace

bool loadSystemConfig(const std::string& path, SystemConfig& config, std::string* error) {
    std::ifstream input(path);
    if (!input.is_open()) {
        if (error) *error = "Unable to open configuration file: " + path;
        return false;
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();
    const std::string json = buffer.str();

    if (!parseUnsigned(json, "ramBytes", config.ramBytes) ||
        !parseUnsigned(json, "pageSizeBytes", config.pageSizeBytes) ||
        !parseUnsigned(json, "swapPages", config.swapPages) ||
        !parseInt(json, "quantum", config.quantum) ||
        !parseInt(json, "firstUserPid", config.firstUserPid) ||
        !parseUnsigned(json, "eventCapacity", config.eventCapacity) ||
        !parseUnsigned(json, "systemProcessMemoryBytes", config.systemProcessMemoryBytes) ||
        !parseDouble(json, "weightMemory", config.oom.weightMemory) ||
        !parseDouble(json, "weightPriority", config.oom.weightPriority) ||
        !parseBool(json, "protectSystemProcesses", config.oom.protectSystemProcesses)) {
        if (error) *error = "Invalid or incomplete JSON configuration fields in: " + path;
        return false;
    }

    if (config.ramBytes == 0 || config.pageSizeBytes == 0 ||
        config.ramBytes < config.pageSizeBytes || config.quantum < 1 ||
        config.firstUserPid < 100 || config.eventCapacity == 0 ||
        config.systemProcessMemoryBytes == 0 || config.systemProcessMemoryBytes > config.ramBytes ||
        config.oom.weightMemory < 0 || config.oom.weightPriority < 0) {
        if (error) *error = "Configuration values are outside the allowed ranges in: " + path;
        return false;
    }

    return true;
}

}  // namespace miniso
