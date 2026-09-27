CC=gcc
CFLAGS=-std=16 -Wall -Werror -Wextra

.PHONY: all test clean

all:
  mkdir -p build && cd build && cmake .. && cmake --build .

test:
  mkdir -p build && cd build && cmake .. && cmake --build . && ./run_tests

clean:
  rm -rf build
