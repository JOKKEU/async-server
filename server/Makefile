TARGET = server


SRC_DIR = src
OBJ_DIR = obj
INC_DIR = hd


CC = gcc
CFLAGS = -Wall -Wextra -g -I$(INC_DIR) -MMD -MP
LDFLAGS =

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))
DEPS = $(OBJS:.o=.d)


.PHONY: all clean re

all: $(TARGET)


$(TARGET): $(OBJS)
	@echo "Linking..."
	$(CC) $(LDFLAGS) -o $@ $^

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	@echo "Cleaning up..."
	rm -f $(TARGET)
	rm -rf $(OBJ_DIR)


re: clean all

-include $(DEPS)
