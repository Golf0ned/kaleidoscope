BUILD_DIR := build

.PHONY: all build clean

all: build

build:
	mkdir -p $(BUILD_DIR)
	cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -S . -B $(BUILD_DIR)
	cmake --build $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)
