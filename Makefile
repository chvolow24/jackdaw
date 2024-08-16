CC := gcc
SRC_DIR := src
BUILD_DIR := build
GUI_SRC_DIR := gui/src
GUI_BUILD_DIR := gui/build
CFLAGS := -Wall -Wno-unused-command-line-argument -I$(SRC_DIR) -I$(GUI_SRC_DIR) -I/usr/include/SDL2 `sdl2-config --libs --cflags` -lSDL2 -lSDL2_ttf -lpthread -lm -DINSTALL_DIR=\"`pwd`\"
CFLAGS_JDAW_ONLY := -DLT_DEV_MODE=0
CFLAGS_LT_ONLY := -DLT_DEV_MODE=1 -DLAYOUT_BUILD=1
CFLAGS_PROD := -O3
CFLAGS_DEBUG := -g -O0 -fsanitize=address
CFLAGS_ADDTL =

LAYOUT_PROGRAM_SRCS := gui/src/openfile.c gui/src/lt_params.c gui/src/draw.c gui/src/main.c gui/src/test.c
JACKDAW_ONLY_SRCS :=  src/main.c  gui/src/test.c gui/src/menu.c gui/src/modal.c gui/src/dir.c gui/src/components.c
SRCS := $(wildcard $(SRC_DIR)/*.c)
GUI_SRCS_ALL := $(wildcard $(GUI_SRC_DIR)/*.c)
GUI_SRCS := $(filter-out $(LAYOUT_PROGRAM_SRCS), $(GUI_SRCS_ALL))
LT_SRCS_ALL := $(filter-out $(JACKDAW_ONLY_SRCS), $(GUI_SRCS_ALL))

OBJS := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))
GUI_OBJS := $(patsubst $(GUI_SRC_DIR)/%.c, $(GUI_BUILD_DIR)/%.o, $(GUI_SRCS))

LT_OBJS := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(LT_SRCS_ALL))

EXEC := jackdaw
LT_EXEC := layout

all: $(EXEC)
.PHONY: debug

# Default to production flags
CFLAGS_ADDTL := $(CFLAGS_PROD)

# Override CFLAGS_ADDTL if the target is "debug"
ifeq ($(MAKECMDGOALS),debug)
    CFLAGS_ADDTL := $(CFLAGS_DEBUG)
endif


$(EXEC): $(OBJS) $(GUI_OBJS)
	$(CC) -o $@ $^ $(CFLAGS) $(CFLAGS_ADDTL) $(CFLAGS_JDAW_ONLY)

debug: $(OBJS) $(GUI_OBJS)
	$(CC) -o $(EXEC) $^ $(CFLAGS) $(CFLAGS_ADDTL)

$(LT_EXEC): CFLAGS_ADDTL := $(CFLAGS_LT_ONLY)
$(LT_EXEC): $(LT_OBJS)
	$(CC) -o $@ $^ $(CFLAGS) $(CFLAGS_ADDTL)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) $(CFLAGS_ADDTL) -c $< -o $@

$(GUI_BUILD_DIR)/%.o: $(GUI_SRC_DIR)/%.c 
	$(CC) $(CFLAGS) $(CFLAGS_ADDTL) -c $< -o $@

# .PHONY: $(BUILD_DIR)
# $(BUILD_DIR):
# 	@test -d $(BUILD_DIR) || mkdir -p $(BUILD_DIR)
# .PHONY: $(GUI_BUILD_DIR)
# $(GUI_BUILD_DIR):
# 	@test -d $(GUI_BUILD_DIR) || mkdir -p $(GUI_BUILD_DIR)
clean:
	@[ -n "${BUILD_DIR}" ] || { echo "BUILD_DIR unset or null"; exit 127; }
	@[ -n "${GUI_BUILD_DIR}" ] || { echo "GUI_BUILD_DIR unset or null"; exit 127; }
	rm -rf $(BUILD_DIR)/* $(GUI_BUILD_DIR)/*
