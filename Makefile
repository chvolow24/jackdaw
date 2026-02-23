ifdef USE_EXTERNAL_SDLS
# Check for pkg-config or pkgconf
PKGCONF := $(shell command -v pkg-config 2>/dev/null || command -v pkgconf 2>/dev/null)
# Check if we found a pkg-config implementation
ifeq ($(PKGCONF),)
$(error "pkg-config required when USE_EXTERNAL_SDLS is set")
endif
HAVE_EXTERNAL_SDL := $(shell $(PKG_CONF) --exists sdl2 2>/dev/null && echo yes || echo no)
HAVE_EXTERNAL_SDL_TTF := $(shell $(PKG_CONF) --exists sdl2_ttf 2>/dev/null && echo yes || echo no)

ifeq ($(HAVE_EXTERNAL_SDL),no)
$(error "SDL2 was not found on your system.")
endif

ifeq ($(HAVE_EXTERNAL_SDL_TTF),no)
$(error "SDL_ttf was not found on your system.")
endif

endif

CC := gcc
SRC_DIR := src
BUILD_DIR := build
GUI_SRC_DIR := gui/src
GUI_BUILD_DIR := gui/build

DEPFLAGS = -MMD -MP -MF $(@:.o=.d)

# Dependencies


SDL_PATH := $(PWD)/SDL
SDL_LIB := $(SDL_PATH)/build/.libs/libSDL2.a
SDL_INCLUDE_PATH := $(SDL_PATH)/include

PORTMIDI_PATH := $(PWD)/portmidi
PORTMIDI_LIB := $(PORTMIDI_PATH)/build/libportmidi.a

SDL_TTF_PATH := $(PWD)/SDL_ttf


ifdef USE_EXTERNAL_SDLS
SDL_TTF_LIB :=
else
SDL_TTF_LIB := $(SDL_TTF_PATH)/.libs/libSDL2_ttf.a
endif

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

ifdef USE_EXTERNAL_SDLS
SDL_FLAGS_ALL := 
else
SDL_FLAGS_ALL := -I$(SDL_INCLUDE_PATH)
endif
LINK_ASOUND :=

# Operation system checks
ifeq ($(UNAME_S),Darwin)
SDL_FLAGS := $(SDL_FLAGS_MACOS_ONLY) $(SDL_FLAGS_ALL)
LDFLAGS := -lpthread -lm
else
SDL_FLAGS := $(SDL_FLAGS_ALL)
LINK_ASOUND := -lasound
LDFLAGS := -lpthread -lm -ldl -lrt
endif

ifdef USE_EXTERNAL_SDLS
	LIBS := $(PORTMIDI_LIB)
	LDFLAGS += $(shell $(PKGCONF) sdl2 --libs) $(shell $(PKGCONF) sdl2_ttf --libs)

else
	LIBS := $(SDL_LIB) $(SDL_TTF_LIB) $(PORTMIDI_LIB)
endif

CFLAGS := -Wall -Wno-unused-command-line-argument -Wno-format-truncation -I$(SRC_DIR) -I$(GUI_SRC_DIR) \
	-Iportmidi/porttime \
	-Iportmidi/pm_common \
	-ISDL_ttf \
	-DINSTALL_DIR="\"$(PWD)\"" \
	$(SDL_FLAGS)

ifdef USE_EXTERNAL_SDLS
CFLAGS += $(shell $(PKGCONF) sdl2 --cflags) $(shell $(PKGCONF) sdl2_ttf --cflags)
endif

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

DEPS := $(OBJS:.o=.d)
GUI_DEPS := $(GUI_OBJS:.o=.d)

EXEC := jackdaw
LT_EXEC := layout

all: $(EXEC)

$(SDL_LIB):
	@echo "\nConfiguring and building SDL2. This may take several minutes. (Logs in sdl_build.log)..."
	@cd SDL && \
	./configure --enable-static --prefix=$(PWD)/SDL/installation >>../sdl_build.log 2>&1 && \
	make >>../sdl_build.log 2>&1 && \
	make install >>../sdl_build.log 2>&1
	@echo "...SDL build complete"

$(SDL_TTF_LIB):
	@echo "\nConfiguring and building SDL2_ttf. This may take several minutes. (Logs in sdl_ttf_build.log)..."
	@cd SDL_ttf && \
	./configure --disable-shared --enable-static --with-sdl-prefix=$(PWD)/SDL/installation --disable-sdltest >>../sdl_ttf_build.log 2>&1 && \
	make >>../sdl_ttf_build.log 2>&1
	@echo "...SDL_ttf build complete."

$(PORTMIDI_LIB):
	@echo "\nConfiguring and building portmidi..."
	(cd portmidi && mkdir -p build && chmod 755 build && cd build && cmake .. -DBUILD_SHARED_LIBS=OFF -DCMAKE_BUILD_TYPE=Release && make)
	@echo "...portmidi build complete."

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
	$(CC) -o $@  $(filter-out %_target,$^) $(CFLAGS) $(CFLAGS_ADDTL) $(CFLAGS_JDAW_ONLY) $(SDL_FLAGS) $(LIBS) $(LINK_ASOUND) $(LDFLAGS)

.PHONY: debug
debug: $(EXEC)

$(LT_EXEC): $(LT_OBJS)
	$(CC) -o $@ $^ $(CFLAGS) $(CFLAGS_ADDTL) $(CFLAGS_LT_ONLY) $(LIBS) $(LFLAGS)

$(BUILD_DIR):
	mkdir $(BUILD_DIR)

$(GUI_BUILD_DIR):
	mkdir -p $(GUI_BUILD_DIR)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(CFLAGS_ADDTL) $(SDL_FLAGS) $(DEPFLAGS) -c $< -o $@

$(GUI_BUILD_DIR)/%.o: $(GUI_SRC_DIR)/%.c $(LIBS) | $(GUI_BUILD_DIR)
	$(CC) $(CFLAGS) $(CFLAGS_ADDTL) $(SDL_FLAGS) $(LIBS) $(DEPFLAGS) -c $< -o $@

-include ${DEPS}
-include ${GUI_DEPS}



clean:
	@[ -n "${BUILD_DIR}" ] || { echo "BUILD_DIR unset or null"; exit 127; }
	@[ -n "${GUI_BUILD_DIR}" ] || { echo "GUI_BUILD_DIR unset or null"; exit 127; }
	rm -rf $(BUILD_DIR)/* $(GUI_BUILD_DIR)/*

cleanall:
	@[ -n "${BUILD_DIR}" ] || { echo "BUILD_DIR unset or null"; exit 127; }
	@[ -n "${GUI_BUILD_DIR}" ] || { echo "GUI_BUILD_DIR unset or null"; exit 127; }
	rm -rf $(BUILD_DIR)/* $(GUI_BUILD_DIR)/*
	git submodule foreach --recursive git clean -fdx
