#include <iostream>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

namespace {

void printChild(int pid, int ppid, int exitCode, bool execFailed) {
    std::cout << "{\"pid\":" << pid << ",\"ppid\":" << ppid << ",\"exitCode\":" << exitCode
              << ",\"execFailed\":" << (execFailed ? "true" : "false") << "}\n";
}

int spawn(const char* path, const char* arg, bool expectFail) {
    const pid_t pid = fork();
    if (pid < 0) {
        std::cerr << "{\"error\":\"fork failed\"}\n";
        return -1;
    }
    if (pid == 0) {
        char* args[] = {const_cast<char*>(path), const_cast<char*>(arg), nullptr};
        execv(path, args);
        _exit(expectFail ? 42 : 127);
    }
    return pid;
}

}  // namespace

int main(int argc, char** argv) {
    std::string childPath = "process_child";
    bool demo = false;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--demo") {
            demo = true;
        } else if (arg == "--child" && i + 1 < argc) {
            childPath = argv[++i];
        }
    }
    if (!demo) {
        std::cerr << "usage: process_runner --demo --child <path>\n";
        return 1;
    }

    std::cout << "{\"runnerPid\":" << getpid() << ",\"runnerPpid\":" << getppid() << "}\n";
    std::cout.flush();

    const int first = spawn(childPath.c_str(), "0", false);
    const int second = spawn(childPath.c_str(), "3", false);
    const int failed = spawn("/this/path/does/not/exist/miniso_missing", "1", true);

    auto waitOne = [](int pid, bool expectExecFail) {
        int status = 0;
        const pid_t waited = waitpid(pid, &status, 0);
        int exitCode = -1;
        if (waited > 0 && WIFEXITED(status)) {
            exitCode = WEXITSTATUS(status);
        }
        printChild(pid, getpid(), exitCode, expectExecFail || exitCode == 42 || exitCode == 127);
    };

    if (first > 0) {
        waitOne(first, false);
    }
    if (second > 0) {
        waitOne(second, false);
    }
    if (failed > 0) {
        waitOne(failed, true);
    }
    return 0;
}
