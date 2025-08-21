BUILD_DIR := build
C_COMPILER := clang
CXX_COMPILER := clang++

.PHONY: all build debug clean

all: build

build:
	mkdir -p $(BUILD_DIR)
	cmake \
		-D CMAKE_CXX_COMPILER=$(CXX_COMPILER) \
		-D CMAKE_C_COMPILER=$(C_COMPILER) \
		-S . -B $(BUILD_DIR)
	cmake --build $(BUILD_DIR)

debug:
	mkdir -p $(BUILD_DIR)
	cmake \
		-D CMAKE_BUILD_TYPE=Debug \
		-D CMAKE_CXX_COMPILER=$(CXX_COMPILER) \
		-D CMAKE_C_COMPILER=$(C_COMPILER) \
		-S . -B $(BUILD_DIR)
	cmake --build $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)
	rm kaleidoscope.bc
	rm kaleidoscope.ll
