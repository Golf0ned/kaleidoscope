cmake_minimum_required(VERSION 3.11)

project(kaleidoscope)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(kaleidoscope
    compiler.cpp
    ast/ast.cpp
    lexer/lexer.cpp
    parser/parser.cpp
)

target_include_directories(kaleidoscope PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
