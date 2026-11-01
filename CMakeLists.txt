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