#include "linux/ProcessRunner.h"

#include <array>
#include <cstdlib>
#include <cstdio>
#include <sstream>

namespace miniso {

ProcessRunner::ProcessRunner(EventSystem& events, SystemConfig config)
    : events_(events), config_(std::move(config)) {}

static std::string extractJsonField(const std::string& text, const std::string& key) {
    const std::string pattern = "\"" + key + "\":";
    auto pos = text.find(pattern);
    if (pos == std::string::npos) {
        return {};
    }
    pos += pattern.size();
    while (pos < text.size() && (text[pos] == ' ')) {
        ++pos;
    }
    if (pos < text.size() && text[pos] == '"') {
        const auto end = text.find('"', pos + 1);
        if (end == std::string::npos) {
            return {};
        }
        return text.substr(pos + 1, end - pos - 1);
    }
    const auto end = text.find_first_of(",}\n", pos);
    return text.substr(pos, end == std::string::npos ? std::string::npos : end - pos);
}

ValueResult<LinuxRunReport> ProcessRunner::runDemo() {
    const std::string command = config_.processRunnerPath + " --demo --child " + config_.processChildPath;
    std::array<char, 512> buffer{};
    std::string output;
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        return ValueResult<LinuxRunReport>::failure("Unable to start process_runner");
    }
    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr) {
        output += buffer.data();
    }
    const int status = pclose(pipe);
    LinuxRunReport report;
    report.rawOutput = output;
    report.runnerPid = std::atoi(extractJsonField(output, "runnerPid").c_str());

    std::istringstream stream(output);
    std::string line;
    while (std::getline(stream, line)) {
        if (line.find("\"pid\"") == std::string::npos) {
            continue;
        }
        LinuxChildResult child;
        child.pid = std::atoi(extractJsonField(line, "pid").c_str());
        child.ppid = std::atoi(extractJsonField(line, "ppid").c_str());
        child.exitCode = std::atoi(extractJsonField(line, "exitCode").c_str());
        child.execFailed = extractJsonField(line, "execFailed").find("true") != std::string::npos;
        child.output = line;
        if (child.pid > 0 || child.execFailed) {
            report.children.push_back(child);
        }
    }

    events_.publish(EventType::LinuxProcess, "ProcessRunner",
                    "Finished Linux demo with " + std::to_string(report.children.size()) + " children",
                    -1, -1, EventLevel::Info, output);
    if (status == -1) {
        return ValueResult<LinuxRunReport>::failure("process_runner failed");
    }
    return ValueResult<LinuxRunReport>::success(report, output);
}

}  // namespace miniso
