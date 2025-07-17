CC := gcc
SRC_DIR := src
BUILD_DIR := build
GUI_SRC_DIR := gui/src
GUI_BUILD_DIR := gui/build

# Dependencies

SDL_PATH := $(PWD)/SDL
SDL_LIB_PATH := $(SDL_PATH)/build/.libs
SDL_INCLUDE_PATH := $(SDL_PATH)/include


SDL_FLAGS_MACOS_ONLY := -framework AudioToolBox \
	-framework Cocoa \
	-framework CoreAudio \
	-framework CoreFoundation \
	-framework CoreHaptics \
	-framework CoreVideo \
	-framework IOKit \
	-framework AVFoundation \
	-framework Metal \
	-framework ForceFeedback \
	-framework GameController \
	-framework Carbon \
	-framework Quartz \
	-framework CoreMedia

UNAME_S := $(shell uname -s)

SDL_FLAGS_ALL := $(SDL_LIB_PATH)/libSDL2.a -I$(SDL_INCLUDE_PATH)
ifeq ($(UNAME_S),Darwin)
SDL_FLAGS := $(SDL_FLAGS_MACOS_ONLY) $(SDL_FLAGS_ALL)
else
SDL_FLAGS := $(SDL_FLAGS_ALL)
endif

CFLAGS := -Wall -Wno-unused-command-line-argument -I$(SRC_DIR) -I$(GUI_SRC_DIR) \
	-lSDL2_ttf \
	-lpthread -lm -DINSTALL_DIR=\"`pwd`\" \
	-Iportmidi/porttime \
	-Iportmidi/pm_common \
	-Lportmidi/build -lportmidi \
	-Wl,-rpath,./portmidi/build \
	-I/opt/homebrew/include/SDL2 \
	-L/opt/homebrew/lib \
	$(SDL_FLAGS)

CFLAGS_JDAW_ONLY := -DLT_DEV_MODE=0
CFLAGS_LT_ONLY := -DLT_DEV_MODE=1 -DLAYOUT_BUILD=1
CFLAGS_PROD := -O3
CFLAGS_DEBUG := -DTESTBUILD=1 -g -O0 -fsanitize=address
CFLAGS_ADDTL =



LAYOUT_PROGRAM_SRCS := gui/src/openfile.c gui/src/lt_params.c gui/src/draw.c gui/src/main.c gui/src/test.c
JACKDAW_ONLY_SRCS :=  src/main.c  gui/src/test.c gui/src/menu.c gui/src/modal.c gui/src/dir.c gui/src/components.c gui/src/label.c gui/src/symbols.c gui/src/autocompletion.c gui/src/symbol.c
SRCS := $(wildcard $(SRC_DIR)/*.c)
GUI_SRCS_ALL := $(wildcard $(GUI_SRC_DIR)/*.c)
GUI_SRCS := $(filter-out $(LAYOUT_PROGRAM_SRCS), $(GUI_SRCS_ALL))
LT_SRCS_ALL := $(filter-out $(JACKDAW_ONLY_SRCS), $(GUI_SRCS_ALL))

OBJS := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))
GUI_OBJS := $(patsubst $(GUI_SRC_DIR)/%.c, $(GUI_BUILD_DIR)/%.o, $(GUI_SRCS))

LT_OBJS := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(LT_SRCS_ALL))

EXEC := jackdaw
LT_EXEC := layout

# PORTMIDI_LIB = portmidi/build/libportmidi

all: $(EXEC)

.PHONY: portmidi_target
portmidi_target:
	cd portmidi && \
	mkdir -p build && \
	cd build && \
	cmake .. -DCMAKE_BUILD_TYPE=Release && \
	make

.PHONY: sdl_target
sdl_target:
	cd sdl && \
	make

.PHONY: debug

# Default to production flags
CFLAGS_ADDTL := $(CFLAGS_PROD)

# Override CFLAGS_ADDTL if the target is "debug"
ifeq ($(MAKECMDGOALS),debug)
    CFLAGS_ADDTL := $(CFLAGS_DEBUG)
endif

ifeq ($(MAKECMDGOALS),layout)
    CFLAGS_ADDTL := $(CFLAGS_DEBUG)
endif


$(EXEC): portmidi_target sdl_target $(OBJS) $(GUI_OBJS)
	$(CC) -o $@  $(filter-out %_target,$^) $(CFLAGS) $(CFLAGS_ADDTL) $(CFLAGS_JDAW_ONLY) $(SDL_FLAGS)

debug: $(OBJS) $(GUI_OBJS)
	$(CC) -o $(EXEC) $^ $(CFLAGS) $(CFLAGS_ADDTL) $(SDL_FLAGS)

# $(LT_EXEC): CFLAGS_ADDTL := $(CFLAGS_LT_ONLY)
$(LT_EXEC): $(LT_OBJS)
	$(CC) -o $@ $^ $(CFLAGS) $(CFLAGS_ADDTL) $(CFLAGS_LT_ONLY)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) $(CFLAGS_ADDTL) $(SDL_FLAGS) -c $< -o $@ 

$(GUI_BUILD_DIR)/%.o: $(GUI_SRC_DIR)/%.c 
	$(CC) $(CFLAGS) $(CFLAGS_ADDTL) $(SDL_FLAGS) -c $< -o $@

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
