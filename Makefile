# Ak nie je nastavené predom inak používa sa prekladac gcc
CC ?= gcc

# Ak nieje predom nastavené inak používa sa štandart c11
CSTD ?= -std=c11

# Flagy pri preklade
CFLAGS ?= $(CSTD) -Wall -Werror -Wextra

# Priečinky
SRC_DIR := src
BUILD_DIR := obj
TEST_DIR = tests

# Meno .exe súboru
TARGET := prekladac

# Všetky .c súbory v src/
SRC_FILES := $(wildcard $(SRC_DIR)/*.c)

# Zoznam .o súborov v obj/ z .c v src/
OBJS := $(SRC_FILES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

# Prevencia pred fake calls
.PHONY: build check build-check clean

build: $(TARGET)

# Pravidlo že najpr musia byť hotové všetky .o súbory
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

# Pravidlo ak nie je obj/ tak ho vytvorí
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Pravidlo že musí existovať obj/ a doňho sa preložia .c súbory na .o
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Spustí testy
#TODO urobiť špúšťanie a výpis testov
check:

# Spustí 
build-check: check build

# Vymaže obj/ a .exe súbor
clean:
	rm -rf $(BUILD_DIR) $(TARGET)
