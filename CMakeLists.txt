cmake_minimum_required(VERSION 3.20)

project(MiniSOLab
    VERSION 1.0.0
    DESCRIPTION "Mini Operating System Laboratory"
    LANGUAGES C CXX
)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

enable_testing()

option(ENABLE_SANITIZERS "Enable sanitizers" ON)

if(ENABLE_SANITIZERS)
    add_compile_options(
        -fsanitize=address
        -fsanitize=undefined
        -fno-omit-frame-pointer
    )

    add_link_options(
        -fsanitize=address
        -fsanitize=undefined
    )
endif()