CC := gcc
SRC_DIR := src
BUILD_DIR := build
INCLUDE_DIR := include
GUI_SRC_DIR := gui/src
GUI_INCLUDE_DIR := gui/include
GUI_BUILD_DIR := gui/build
CFLAGS := -Wall -I$(INCLUDE_DIR) -I$(GUI_INCLUDE_DIR) -I/usr/include/SDL2 `sdl2-config --libs --cflags` -lSDL2 -lSDL2_ttf -lpthread -lm -D INSTALL_DIR=\"`pwd`\"

SRCS := $(wildcard $(SRC_DIR)/*.c)
GUI_SRCS := $(wildcard $(GUI_SRC_DIR)/*.c)
OBJS := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))
GUI_OBJS := $(patsubst $(GUI_SRC_DIR)/%.c, $(GUI_BUILD_DIR)/%.o, $(GUI_SRCS))
VPATH = $(SRC_DIR)

EXEC := jackdaw



$(EXEC): $(OBJS) $(GUI_OBJS)
	$(CC) -o $@ $^ $(CFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(GUI_BUILD_DIR)/%.o: $(GUI_SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@



# $(EXEC): $(OBJS)
# 	$(CC) -o $@ $^ $(CFLAGS)

# $(BUILD_DIR)/%.o: %.c
# 	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(EXEC) $(BUILD_DIR)/*
