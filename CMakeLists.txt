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

# ── Shared sources ────────────────────────────────────────────────────────────
set(CVM_SOURCES
    src/Lexer.cpp
    src/Parser.cpp
    src/ASTPrinter.cpp
    src/Chunk.cpp
    src/Compiler.cpp
    src/VM.cpp
)

# ── Main executable ──────────────────────────────────────────────────────────
add_executable(cvm src/main.cpp ${CVM_SOURCES})
target_include_directories(cvm PRIVATE include)

# ── Lexer test executable ─────────────────────────────────────────────────────
add_executable(test_lexer tests/test_lexer.cpp src/Lexer.cpp)
target_include_directories(test_lexer PRIVATE include)

# ── Parser test executable ────────────────────────────────────────────────────
add_executable(test_parser
    tests/test_parser.cpp
    src/Lexer.cpp
    src/Parser.cpp
    src/ASTPrinter.cpp
)
target_include_directories(test_parser PRIVATE include)

# ── Compiler test executable ──────────────────────────────────────────────────
add_executable(test_compiler tests/test_compiler.cpp ${CVM_SOURCES})
target_include_directories(test_compiler PRIVATE include)

# ── VM test executable ────────────────────────────────────────────────────────
add_executable(test_vm tests/test_vm.cpp ${CVM_SOURCES})
target_include_directories(test_vm PRIVATE include)

# ── Install ──────────────────────────────────────────────────────────────────
install(TARGETS cvm DESTINATION bin)
