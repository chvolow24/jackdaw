CC := gcc
SRC_DIR := src
BUILD_DIR := build
INCLUDE_DIR := include
GUI_SRC_DIR := gui/src
GUI_INCLUDE_DIR := gui/include
GUI_BUILD_DIR := gui/build
CFLAGS := -Wall -g -I$(INCLUDE_DIR) -I$(GUI_INCLUDE_DIR) -I/usr/include/SDL2 `sdl2-config --libs --cflags` -lSDL2 -lSDL2_ttf -lpthread -lm -DINSTALL_DIR=\"`pwd`\" -fsanitize=address # -O2 #-DLT_DEV_MODE=0


LAYOUT_PROGRAM_SRCS := gui/src/openfile.c gui/src/lt_params.c gui/src/draw.c gui/src/main.c gui/src/test.c
JACKDAW_ONLY_SRCS := src/main.c
SRCS := $(wildcard $(SRC_DIR)/*.c)
GUI_SRCS_ALL := $(wildcard $(GUI_SRC_DIR)/*.c)
GUI_SRCS := $(filter-out $(LAYOUT_PROGRAM_SRCS), $(GUI_SRCS_ALL))
LT_SRCS_ALL := $(filter-out $(JACKDAW_ONLY_SRCS), $(SRCS) $(GUI_SRCS_ALL))

OBJS := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))
GUI_OBJS := $(patsubst $(GUI_SRC_DIR)/%.c, $(GUI_BUILD_DIR)/%.o, $(GUI_SRCS))

LT_OBJS := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(LT_SRCS_ALL))

EXEC := jackdaw
LT_EXEC := layout


$(EXEC): $(OBJS) $(GUI_OBJS)
	$(CC) -o $@ $^ $(CFLAGS)

$(LT_EXEC): $(LT_OBJS)
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
	rm -rf $(EXEC) $(BUILD_DIR)/* $(GUI_BUILD_DIR)/*
