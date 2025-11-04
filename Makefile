SRC_DIR = src
OBJ_DIR = build
INCLUDE_DIR = include

CC = gcc
CFLAGS = -Wall -I$(INCLUDE_DIR)

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

TARGET = $(OBJ_DIR)/modbus-server

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR) $(TARGET)

run: $(TARGET)
	@echo "Running $(TARGET)..."
	@$(TARGET)