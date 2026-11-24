# If not set otherwise, the compiler used is gcc
CC ?= gcc

# If not set otherwise, the C standard used is c11
CSTD ?= -std=c11

# Default flags for the compiler
CFLAGS ?= $(CSTD) -Wall -Wextra

# Directories
SRC_DIR := src
BUILD_DIR := obj
TEST_DIR := tests

# Name of the final executable
TARGET := prekladac

# List of .c files in src directory
SRC_FILES := $(wildcard $(SRC_DIR)/*.c)

# List of .o files in obj directory corresponding to .c files in src directory
OBJS := $(SRC_FILES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

# Phony targets to avoid conflicts with files of the same name
.PHONY: build check build-check clean

build: $(TARGET)

# Rule that the target depends on all .o files
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

# Rule to create the obj directory if it doesn't exist
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Rule that requires obj directory to exist and compiles .c files into .o files in it
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Runs tests
# TODO: Implement tests
check:

# Runs build and tests
build-check: build check

# Deletes the build directory and the final executable
clean:
	rm -rf $(BUILD_DIR) $(TARGET)
