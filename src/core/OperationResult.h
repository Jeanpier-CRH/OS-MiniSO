#pragma once

#include <string>
#include <utility>

namespace miniso {

struct OperationResult {
    bool ok = false;
    std::string message;

    static OperationResult success(std::string message = {}) {
        return OperationResult{true, std::move(message)};
    }

    static OperationResult failure(std::string message) {
        return OperationResult{false, std::move(message)};
    }
};

template <typename T>
struct ValueResult {
    bool ok = false;
    T value{};
    std::string message;

    static ValueResult success(T value, std::string message = {}) {
        ValueResult result;
        result.ok = true;
        result.value = std::move(value);
        result.message = std::move(message);
        return result;
    }

    static ValueResult failure(std::string message) {
        ValueResult result;
        result.ok = false;
        result.message = std::move(message);
        return result;
    }
};

}  // namespace miniso
