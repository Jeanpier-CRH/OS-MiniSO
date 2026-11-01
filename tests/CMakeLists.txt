include(FetchContent)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(googletest)

function(miniso_test name source)
    add_executable(${name} ${source})
    target_link_libraries(${name} PRIVATE miniso_core GTest::gtest_main)
    add_test(NAME ${name} COMMAND ${name})
endfunction()

miniso_test(test_processes test_processes.cpp)
miniso_test(test_scheduler test_scheduler.cpp)
miniso_test(test_partition test_partition.cpp)
miniso_test(test_paging test_paging.cpp)
miniso_test(test_swap test_swap.cpp)
miniso_test(test_oom test_oom.cpp)
miniso_test(test_concurrency test_concurrency.cpp)
miniso_test(test_integration test_integration.cpp)
miniso_test(test_config test_config.cpp)
target_compile_definitions(test_config PRIVATE MINISO_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
