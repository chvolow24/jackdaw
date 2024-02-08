CC := gcc
SRC_DIR := src
BUILD_DIR := build
INCLUDE_DIR := include
CFLAGS := -Wall -Iinclude -I/usr/include/SDL2 `sdl2-config --libs --cflags` -lSDL2 -lSDL2_ttf -lpthread -lm -D INSTALL_DIR=\"`pwd`\"

SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))
VPATH = $(SRC_DIR)

EXEC := layout

$(EXEC): $(OBJS)
	$(CC) -o $@ $^ $(CFLAGS)

$(BUILD_DIR)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(EXEC) $(BUILD_DIR)/*