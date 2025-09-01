CC := gcc
SRC_DIR := src
BUILD_DIR := build
GUI_SRC_DIR := gui/src
GUI_BUILD_DIR := gui/build

# Dependencies

SDL_PATH := $(PWD)/SDL
SDL_LIB := $(SDL_PATH)/build/.libs/libSDL2.a
SDL_INCLUDE_PATH := $(SDL_PATH)/include

PORTMIDI_PATH := $(PWD)/portmidi
PORTMIDI_LIB := $(PORTMIDI_PATH)/build/libportmidi.a

SDL_TTF_PATH := $(PWD)/SDL_ttf
SDL_TTF_LIB := $(SDL_TTF_PATH)/.libs/libSDL2_ttf.a


SDL_FLAGS_MACOS_ONLY := -framework AudioToolBox \
	-framework Cocoa \
	-framework CoreAudio \
	-framework CoreFoundation \
	-framework CoreMIDI \
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

SDL_FLAGS_ALL := -I$(SDL_INCLUDE_PATH)
ifeq ($(UNAME_S),Darwin)
SDL_FLAGS := $(SDL_FLAGS_MACOS_ONLY) $(SDL_FLAGS_ALL)
else
SDL_FLAGS := $(SDL_FLAGS_ALL)
endif

SDL_TTF_FLAGS := $(PWD)/sdl_ttf/.libs/libSDL2_ttf.a
PORTMIDI_FLAGS := $(PWD)/portmidi/build/libportmidi.a



LIBS := $(SDL_LIB) $(SDL_TTF_LIB) $(PORTMIDI_LIB)

CFLAGS := -Wall -Wno-unused-command-line-argument -I$(SRC_DIR) -I$(GUI_SRC_DIR) \
	-lpthread -lm \
	-Iportmidi/porttime \
	-Iportmidi/pm_common \
	-ISDL_ttf \
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

$(SDL_LIB):
	cd sdl && \
	make

$(SDL_TTF_LIB):
	cd sdl_ttf && \
	export PKG_CONFIG_PATH=../freetype:../harfbuzz/src:$PKG_CONFIG_PATH && \
	./configure --disable-shared --enable-static --prefix=$(pwd)/build && \
	make

$(PORTMIDI_LIB):
	cd portmidi && \
	mkdir -p build && \
	cd build && \
	cmake .. -DBUILD_SHARED_LIBS=OFF -DCMAKE_BUILD_TYPE=Release && \
	make


# .PHONY: sdl2_ttf_target
# sdl2_ttf_target:
# 	cd sdl_ttf && \
# 	export PKG_CONFIG_PATH=../freetype:../harfbuzz/src:$PKG_CONFIG_PATH && \
# 	./configure --disable-shared --enable-static --prefix=$(pwd)/build && \
# 	make

# .PHONY: portmidi_target
# portmidi_target:
# 	cd portmidi && \
# 	mkdir -p build && \
# 	cd build && \
# 	cmake .. -DBUILD_SHARED_LIBS=OFF -DCMAKE_BUILD_TYPE=Release && \
# 	make

# .PHONY: sdl_target
# sdl_target:
# 	cd sdl && \
# 	make

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


$(EXEC): $(OBJS) $(GUI_OBJS)
	$(CC) -o $@  $(filter-out %_target,$^) $(CFLAGS) $(CFLAGS_ADDTL) $(CFLAGS_JDAW_ONLY) $(SDL_FLAGS) $(LIBS)

debug: $(OBJS) $(GUI_OBJS)
	$(CC) -o $(EXEC) $^ $(CFLAGS) $(CFLAGS_ADDTL) $(SDL_FLAGS) $(LIBS)

# $(LT_EXEC): CFLAGS_ADDTL := $(CFLAGS_LT_ONLY)
$(LT_EXEC): $(LT_OBJS)
	$(CC) -o $@ $^ $(CFLAGS) $(CFLAGS_ADDTL) $(CFLAGS_LT_ONLY) $(LIBS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c $(LIBS)
	$(CC) $(CFLAGS) $(CFLAGS_ADDTL) $(SDL_FLAGS) $(LIBS) -c $< -o $@ 

$(GUI_BUILD_DIR)/%.o: $(GUI_SRC_DIR)/%.c $(LIBS)
	$(CC) $(CFLAGS) $(CFLAGS_ADDTL) $(SDL_FLAGS) $(LIBS) -c $< -o $@

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

cleanall:
	@[ -n "${BUILD_DIR}" ] || { echo "BUILD_DIR unset or null"; exit 127; }
	@[ -n "${GUI_BUILD_DIR}" ] || { echo "GUI_BUILD_DIR unset or null"; exit 127; }
	rm -rf $(BUILD_DIR)/* $(GUI_BUILD_DIR)/*
	cd $(SDL_PATH) && make clean
	cd $(PORTMIDI_PATH) && rm -rf build/*
	cd $(SDL_TTF_PATH) && make clean
