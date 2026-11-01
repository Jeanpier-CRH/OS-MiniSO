cmake_minimum_required(VERSION 3.16)
project(MiniSO-Lab VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(MINISO_ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
option(MINISO_BUILD_TESTS "Build automated tests" ON)

set(MINISO_CORE_SOURCES
    src/core/EventSystem.cpp
    src/core/MiniSO.cpp
    src/processes/ProcessManager.cpp
    src/scheduler/RoundRobinScheduler.cpp
    src/memory/PartitionManager.cpp
    src/memory/PagingManager.cpp
    src/memory/SwapManager.cpp
    src/memory/OOMManager.cpp
    src/memory/MemoryManager.cpp
    src/concurrency/ProducerConsumer.cpp
    src/concurrency/MiniRenderer.cpp
    src/linux/ProcessRunner.cpp
    src/applications/ApplicationManager.cpp
    src/logging/EventLogger.cpp
    src/console/ConsoleInterface.cpp
)

add_library(miniso_core STATIC ${MINISO_CORE_SOURCES})
target_include_directories(miniso_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(miniso_core PUBLIC pthread)
target_compile_features(miniso_core PUBLIC cxx_std_17)
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(miniso_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(miniso src/main.cpp)
target_link_libraries(miniso PRIVATE miniso_core)

add_executable(process_child process_runner/ProcessChild.cpp)
add_executable(process_runner process_runner/main.cpp)

set(MINISO_PROCESS_RUNNER_PATH $<TARGET_FILE:process_runner>)
set(MINISO_PROCESS_CHILD_PATH $<TARGET_FILE:process_child>)

target_compile_definitions(miniso_core PUBLIC
    "MINISO_DEFAULT_RUNNER=\"$<TARGET_FILE:process_runner>\""
    "MINISO_DEFAULT_CHILD=\"$<TARGET_FILE:process_child>\""
)

if (MINISO_ENABLE_SANITIZERS AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(miniso_core PUBLIC -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(miniso_core PUBLIC -fsanitize=address,undefined)
    target_compile_options(miniso PUBLIC -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(miniso PUBLIC -fsanitize=address,undefined)
    target_compile_options(process_runner PUBLIC -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(process_runner PUBLIC -fsanitize=address,undefined)
    target_compile_options(process_child PUBLIC -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(process_child PUBLIC -fsanitize=address,undefined)
endif()

add_dependencies(miniso process_runner process_child)

if (MINISO_BUILD_TESTS)
    include(CTest)
    enable_testing()
    add_subdirectory(tests)
endif()
