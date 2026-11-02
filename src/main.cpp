#include "console/ConsoleInterface.h"
#include "core/MiniSO.h"

#include <iostream>
#include <string>

int main(int argc, char** argv) {
    miniso::SystemConfig config;
    if (argc > 1) {
        const std::string arg = argv[1];
        if (arg == "--help") {
            std::cout << "MiniSO Lab Phase 1\n"
                      << "  ./miniso              interactive console\n"
                      << "  ./miniso --demo       run built-in scenarios and exit\n";
            return 0;
        }
        if (arg == "--demo") {
            miniso::MiniSO system(config);
            system.boot();
            std::cout << system.runScenarioA() << '\n';
            std::cout << system.runScenarioB() << '\n';
            std::cout << system.runScenarioC() << '\n';
            std::cout << system.runScenarioD() << '\n';
            auto linuxDemo = system.runLinuxDemo();
            std::cout << (linuxDemo.ok ? linuxDemo.value.rawOutput : linuxDemo.message) << '\n';
            std::cout << system.processTree();
            system.shutdown();
            return 0;
        }
    }
    miniso::MiniSO system(config);
    system.boot();
    miniso::ConsoleInterface console(system);
    return console.run(std::cin, std::cout);
}
