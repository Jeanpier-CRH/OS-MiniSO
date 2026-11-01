#include "console/ConsoleInterface.h"

#include <iostream>
#include <sstream>

namespace miniso {
namespace {

void printPcb(std::ostream& out, const ProcessControlBlock& pcb) {
    out << "PID=" << pcb.pid << " PPID=" << pcb.parentPid << " name=" << pcb.name
        << " state=" << processStateName(pcb.state) << " prio=" << pcb.priority
        << " mem=" << pcb.logicalMemorySize << " resident=" << pcb.residentMemorySize
        << " pages=" << pcb.requiredPages << " frames=" << pcb.assignedFrames
        << " cpuLeft=" << pcb.remainingCpuTime << " threads=" << pcb.logicalThreadCount;
    if (pcb.systemProcess) {
        out << " [SYSTEM]";
    }
    out << '\n';
}

}  // namespace

ConsoleInterface::ConsoleInterface(MiniSO& system) : system_(system) {}

void ConsoleInterface::printBanner(std::ostream& out) const {
    const auto status = system_.status();
    out << "================================================\n";
    out << "                  MINISO LAB\n";
    out << "          OPERATING SYSTEM SIMULATOR\n";
    out << "================================================\n\n";
    out << "System Status: " << status.state << '\n';
    out << "Simulated RAM: " << bytesToMb(status.memory.paging.ramBytes) << " MB\n";
    out << "Used Frames: " << status.memory.paging.usedFrames << '/' << status.memory.paging.frameCount << '\n';
    out << "Free Frames: " << status.memory.paging.freeFrames << '\n';
    out << "Active Processes: " << status.activeProcesses << "\n\n";
}

void ConsoleInterface::printMenu(std::ostream& out) const {
    out << "1. Process Manager\n";
    out << "2. CPU Scheduler\n";
    out << "3. Memory Manager\n";
    out << "4. Thread Manager\n";
    out << "5. Desktop Applications\n";
    out << "6. Scenario Laboratory\n";
    out << "7. Linux Real Processes\n";
    out << "8. System Event Log\n";
    out << "9. System Status\n";
    out << "0. Shutdown\n";
    out << "\nSelect option: ";
}

std::string ConsoleInterface::readLine(std::istream& in, std::ostream& out, const std::string& prompt) const {
    out << prompt;
    std::string line;
    std::getline(in, line);
    return line;
}

int ConsoleInterface::readInt(std::istream& in, std::ostream& out, const std::string& prompt, int fallback) const {
    const auto line = readLine(in, out, prompt);
    if (line.empty()) {
        return fallback;
    }
    try {
        return std::stoi(line);
    } catch (...) {
        return fallback;
    }
}

void ConsoleInterface::handle(int option, std::istream& in, std::ostream& out) {
    if (option == 1) {
        out << "a) List  b) Create  c) BCP  d) Tree  e) Terminate\n";
        const auto sub = readLine(in, out, "choice: ");
        if (sub == "a") {
            for (const auto& pcb : system_.processes(true)) {
                printPcb(out, pcb);
            }
        } else if (sub == "b") {
            CreateProcessRequest request;
            request.name = readLine(in, out, "name: ");
            request.logicalMemorySize = static_cast<std::size_t>(
                readInt(in, out, "memory bytes [4194304]: ", 4194304));
            request.priority = readInt(in, out, "priority [0]: ", 0);
            request.cpuBurst = static_cast<std::uint64_t>(readInt(in, out, "cpu burst [10]: ", 10));
            request.logicalThreadCount = readInt(in, out, "logical threads [1]: ", 1);
            request.parentPid = readInt(in, out, "parent pid [1]: ", 1);
            auto created = system_.createProcess(request);
            out << (created.ok ? "Created PID " + std::to_string(created.value.pid) : created.message) << '\n';
        } else if (sub == "c") {
            const int pid = readInt(in, out, "pid: ", -1);
            auto pcb = system_.getProcess(pid);
            if (!pcb) {
                out << "Unknown PID\n";
            } else {
                printPcb(out, *pcb);
            }
        } else if (sub == "d") {
            out << system_.processTree();
        } else if (sub == "e") {
            const int pid = readInt(in, out, "pid: ", -1);
            auto result = system_.terminateProcess(pid, false, "console");
            out << result.message << (result.ok ? " terminated\n" : "\n");
        }
    } else if (option == 2) {
        out << "a) Tick  b) Run 20 ticks  c) Quantum  d) Snapshot\n";
        const auto sub = readLine(in, out, "choice: ");
        if (sub == "a") {
            out << system_.schedulerTick().message << '\n';
        } else if (sub == "b") {
            for (int i = 0; i < 20; ++i) {
                system_.schedulerTick();
            }
            auto snap = system_.schedulerSnapshot();
            out << "clock=" << snap.clock << " running=" << snap.runningPid << '\n';
        } else if (sub == "c") {
            system_.setQuantum(readInt(in, out, "quantum: ", 3));
        } else if (sub == "d") {
            auto snap = system_.schedulerSnapshot();
            out << "clock=" << snap.clock << " running=" << snap.runningPid << " ready=";
            for (int pid : snap.readyQueue) {
                out << pid << ' ';
            }
            out << '\n';
        }
    } else if (option == 3) {
        out << "a) Status  b) Partitions  c) Allocate  d) Release  e) Frames  f) Page table  g) Translate  h) Swap\n";
        const auto sub = readLine(in, out, "choice: ");
        if (sub == "a") {
            auto mem = system_.memoryStatus();
            out << "partition used=" << mem.partition.used << " free=" << mem.partition.free
                << " largestFree=" << mem.partition.largestFree << '\n';
            out << "frames used=" << mem.paging.usedFrames << " free=" << mem.paging.freeFrames
                << " swap=" << mem.paging.swapUsed << '/' << mem.paging.swapCapacity << '\n';
        } else if (sub == "b") {
            out << system_.dumpPartitions();
        } else if (sub == "c") {
            const auto size = static_cast<std::size_t>(readInt(in, out, "size: ", 1024));
            const int pid = readInt(in, out, "pid: ", 100);
            auto result = system_.allocatePartition(size, pid);
            out << (result.ok ? "allocated at " + std::to_string(result.value.startAddress) : result.message)
                << '\n';
        } else if (sub == "d") {
            const auto start = static_cast<std::size_t>(readInt(in, out, "start: ", 0));
            out << system_.releasePartition(start).message << '\n';
        } else if (sub == "e") {
            out << system_.dumpFrames();
        } else if (sub == "f") {
            const int pid = readInt(in, out, "pid: ", 1);
            auto table = system_.pageTable(pid);
            if (!table) {
                out << "No page table\n";
            } else {
                out << "internal fragmentation=" << table->internalFragmentation() << '\n';
                for (const auto& entry : table->entries) {
                    out << "page " << entry.pageNumber << " present=" << entry.present
                        << " frame=" << entry.frameNumber << " swap=" << entry.swapSlot << '\n';
                }
            }
        } else if (sub == "g") {
            const int pid = readInt(in, out, "pid: ", 1);
            const auto addr = static_cast<std::size_t>(readInt(in, out, "logical address: ", 0));
            auto tr = system_.translateAddress(pid, addr);
            out << tr.explanation << '\n';
        } else if (sub == "h") {
            out << system_.dumpSwap();
        }
    } else if (option == 4) {
        out << "a) Start PC  b) Status  c) Wait  d) Stop  e) MiniRenderer\n";
        const auto sub = readLine(in, out, "choice: ");
        if (sub == "a") {
            const int cap = readInt(in, out, "capacity [4]: ", 4);
            const int items = readInt(in, out, "items [20]: ", 20);
            out << system_.startProducerConsumer(cap, items).message << " started\n";
        } else if (sub == "b") {
            auto stats = system_.producerConsumerStats();
            out << "produced=" << stats.produced << " consumed=" << stats.consumed
                << " buffer=" << stats.inBuffer << '/' << stats.capacity << '\n';
        } else if (sub == "c") {
            out << (system_.waitProducerConsumer() ? "finished\n" : "timeout\n");
        } else if (sub == "d") {
            system_.stopProducerConsumer();
        } else if (sub == "e") {
            auto result = system_.runMiniRenderer(8, 8, true);
            out << (result.ok ? "checksum=" + std::to_string(result.value.checksum) : result.message) << '\n';
        }
    } else if (option == 5) {
        out << "a) MiniBrowser  b) MiniEditor  c) MiniPlayer  d) MiniCalculator  e) MiniFileManager  f) Tab  g) Close\n";
        const auto sub = readLine(in, out, "choice: ");
        const char* apps[] = {"MiniBrowser", "MiniEditor", "MiniPlayer", "MiniCalculator", "MiniFileManager"};
        if (sub.size() == 1 && sub[0] >= 'a' && sub[0] <= 'e') {
            auto opened = system_.openApplication(apps[sub[0] - 'a']);
            out << (opened.ok ? "pid=" + std::to_string(opened.value.pid) : opened.message) << '\n';
        } else if (sub == "f") {
            const int pid = readInt(in, out, "browser pid: ", -1);
            const auto title = readLine(in, out, "title: ");
            out << system_.addBrowserTab(pid, title).message << '\n';
        } else if (sub == "g") {
            const int pid = readInt(in, out, "pid: ", -1);
            out << system_.closeApplication(pid).message << '\n';
        }
    } else if (option == 6) {
        out << "a) Lifecycle  b) Renderer/PageFault  c) OOM  d) Producer-Consumer\n";
        const auto sub = readLine(in, out, "choice: ");
        if (sub == "a") {
            out << system_.runScenarioA();
        } else if (sub == "b") {
            out << system_.runScenarioB();
        } else if (sub == "c") {
            out << system_.runScenarioC();
        } else if (sub == "d") {
            out << system_.runScenarioD();
        }
    } else if (option == 7) {
        auto report = system_.runLinuxDemo();
        out << (report.ok ? report.value.rawOutput : report.message) << '\n';
    } else if (option == 8) {
        out << system_.logger().recent(40);
    } else if (option == 9) {
        printBanner(out);
        out << system_.processTree();
    }
}

int ConsoleInterface::run(std::istream& in, std::ostream& out) {
    while (true) {
        printBanner(out);
        printMenu(out);
        std::string line;
        if (!std::getline(in, line)) {
            break;
        }
        int option = -1;
        try {
            option = std::stoi(line);
        } catch (...) {
            continue;
        }
        if (option == 0) {
            system_.shutdown();
            out << "MiniSO Lab stopped.\n";
            return 0;
        }
        handle(option, in, out);
        out << '\n';
    }
    system_.shutdown();
    return 0;
}

}  // namespace miniso
