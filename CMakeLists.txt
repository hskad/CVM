cmake_minimum_required(VERSION 3.15)
project(CVM++ VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler warnings
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# ── Main executable ──────────────────────────────────────────────────────────
add_executable(cvm
    src/main.cpp
    src/Lexer.cpp
)

target_include_directories(cvm PRIVATE include)

# ── Test executable ──────────────────────────────────────────────────────────
add_executable(test_lexer
    tests/test_lexer.cpp
    src/Lexer.cpp
)

target_include_directories(test_lexer PRIVATE include)

# ── Install ──────────────────────────────────────────────────────────────────
install(TARGETS cvm DESTINATION bin)
