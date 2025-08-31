cmake_minimum_required(VERSION 3.22)
project(kaleidoscope LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Sources
set(SOURCES
    src/compiler.cpp
    src/ast.cpp
    src/debug.cpp
    src/lexer.cpp
    src/llvm.cpp
    src/parser.cpp
    src/runtime.cpp
)

add_executable(kaleidoscope ${SOURCES})

# Get flags from llvm-config
execute_process(COMMAND llvm-config --cxxflags
                OUTPUT_VARIABLE LLVM_CXXFLAGS
                OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND llvm-config --ldflags
                OUTPUT_VARIABLE LLVM_LDFLAGS
                OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND llvm-config --libs core orcjit native
                OUTPUT_VARIABLE LLVM_LIBS
                OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND llvm-config --system-libs
                OUTPUT_VARIABLE LLVM_SYSTEM_LIBS
                OUTPUT_STRIP_TRAILING_WHITESPACE)

# Apply compiler flags
target_compile_options(kaleidoscope PRIVATE ${LLVM_CXXFLAGS})

# Apply linker flags
target_link_libraries(kaleidoscope PRIVATE -rdynamic ${LLVM_LDFLAGS} ${LLVM_LIBS} ${LLVM_SYSTEM_LIBS})
