CC := gcc
SRC_DIR := src
BUILD_DIR := build
GUI_SRC_DIR := gui/src
GUI_BUILD_DIR := gui/build
CFLAGS := -Wall -g -I$(SRC_DIR) -I$(GUI_SRC_DIR)  -I/usr/include/SDL2 `sdl2-config --libs --cflags` -lSDL2 -lSDL2_ttf -lpthread -lm -DINSTALL_DIR=\"`pwd`\" -fsanitize=address #-O3 # -O2 #-DLT_DEV_MODE=0
CFLAGS_JDAW_ONLY := -DLT_DEV_MODE=0
CFLAGS_LT_ONLY := -DLT_DEV_MODE=1 -DLAYOUT_BUILD=1
CFLAGS_ADDTL =

LAYOUT_PROGRAM_SRCS := gui/src/openfile.c gui/src/lt_params.c gui/src/draw.c gui/src/main.c gui/src/test.c
JACKDAW_ONLY_SRCS :=  src/main.c gui/src/userfn.c gui/src/input.c gui/src/test.c gui/src/modal.c gui/src/dir.c
SRCS := $(wildcard $(SRC_DIR)/*.c)
GUI_SRCS_ALL := $(wildcard $(GUI_SRC_DIR)/*.c)
GUI_SRCS := $(filter-out $(LAYOUT_PROGRAM_SRCS), $(GUI_SRCS_ALL))
LT_SRCS_ALL := $(filter-out $(JACKDAW_ONLY_SRCS), $(GUI_SRCS_ALL))

OBJS := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))
GUI_OBJS := $(patsubst $(GUI_SRC_DIR)/%.c, $(GUI_BUILD_DIR)/%.o, $(GUI_SRCS))

LT_OBJS := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(LT_SRCS_ALL))

EXEC := jackdaw
LT_EXEC := layout


$(EXEC): CFLAGS_ADDTL := $(CFLAGS_JDAW_ONLY)
$(EXEC): $(OBJS) $(GUI_OBJS)
	$(CC) -o $@ $^ $(CFLAGS) $(CFLAGS_JDAW_ONLY)

$(LT_EXEC): CFLAGS_ADDTL := $(CFLAGS_LT_ONLY)
$(LT_EXEC): $(LT_OBJS)
	$(CC) -o $@ $^ $(CFLAGS) $(CFLAGS_ADDTL)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) $(CFLAGS_ADDTL) -c $< -o $@

$(GUI_BUILD_DIR)/%.o: $(GUI_SRC_DIR)/%.c
	$(CC) $(CFLAGS) $(CFLAGS_ADDTL) -c $< -o $@


# .SECONDEXPANSION:
# $(EXEC): CFLAGS_ADDTL := $(CFLAGS_JDAW_ONLY)
# $(LT_EXEC): CFLAGS_ADDTL := $(CFLAGS_LT_ONLY)


clean:
	rm -rf $(EXEC) $(LT_EXEC) $(BUILD_DIR)/* $(GUI_BUILD_DIR)/*
