CC := gcc
CFLAGS := -Wall -Wextra -Wpedantic -std=c11 -g -Iinclude
LDFLAGS := -pthread -lm

SRC_DIR := src
OBJ_DIR := build
BIN_DIR := bin

SIM_SRCS := $(SRC_DIR)/main.c $(SRC_DIR)/control_tower.c
SIM_OBJS := $(SIM_SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
SIM_BIN := $(BIN_DIR)/airport_simulator

SENDER_SRC := $(SRC_DIR)/sendInfo.c
SENDER_OBJ := $(SENDER_SRC:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
SENDER_BIN := $(BIN_DIR)/send_info

.PHONY: all clean

all: $(SIM_BIN) $(SENDER_BIN)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c include/airport.h | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(SIM_BIN): $(SIM_OBJS) | $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(SENDER_BIN): $(SENDER_OBJ) | $(BIN_DIR)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(OBJ_DIR) $(BIN_DIR):
	mkdir -p $@

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)
