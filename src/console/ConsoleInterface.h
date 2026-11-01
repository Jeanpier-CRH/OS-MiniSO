#pragma once

#include "core/MiniSO.h"

#include <iosfwd>
#include <string>

namespace miniso {

class ConsoleInterface {
public:
    explicit ConsoleInterface(MiniSO& system);
    int run(std::istream& in, std::ostream& out);

private:
    void printBanner(std::ostream& out) const;
    void printMenu(std::ostream& out) const;
    void handle(int option, std::istream& in, std::ostream& out);
    std::string readLine(std::istream& in, std::ostream& out, const std::string& prompt) const;
    int readInt(std::istream& in, std::ostream& out, const std::string& prompt, int fallback) const;

    MiniSO& system_;
};

}  // namespace miniso
