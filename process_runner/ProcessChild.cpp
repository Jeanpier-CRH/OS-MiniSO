#include <iostream>
#include <unistd.h>
#include <cstdlib>

int main(int argc, char** argv) {
    const int exitCode = argc > 1 ? std::atoi(argv[1]) : 0;
    std::cout << "{\"pid\":" << getpid() << ",\"ppid\":" << getppid()
              << ",\"message\":\"process_child running\",\"exitCode\":" << exitCode << "}\n";
    std::cout.flush();
    return exitCode;
}
