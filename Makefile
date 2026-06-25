JACKDAW_VERSION := 0.8.0
EXEC := jackdaw
all: jackdaw

# PKGCONF is used to locate system packages
PKGCONF := $(shell command -v pkg-config 2>/dev/null || command -v pkgconf 2>/dev/null)
ifeq ($(PKGCONF),)
    HAVE_PKGCONF := 0
else
    HAVE_PKGCONF := 1
endif

# Canonical pkg-config package names
SDL2_PKG_NAME := sdl2
SDL2_TTF_PKG_NAME := SDL2_ttf
PORTMIDI_PKG_NAME := portmidi

# Append CFLAGS for each package
PKG_FLAGS := 

# Check for system packages
ifeq ($(HAVE_PKGCONF),1)
HAVE_SYSTEM_SDL2 := $(shell $(PKGCONF) --exists $(SDL2_PKG_NAME) 2>/dev/null && echo 1 || echo 0)
HAVE_SYSTEM_SDL2_TTF := $(shell $(PKGCONF) --exists $(SDL2_TTF_PKG_NAME) 2>/dev/null && echo 1 || echo 0)
HAVE_SYSTEM_PORTMIDI := $(shell $(PKGCONF) --exists $(PORTMIDI_PKG_NAME) 2>/dev/null && echo 1 || echo 0)
HAVE_SYSTEM_LIBAVCODEC := $(shell $(PKGCONF) --exists libavcodec 2>/dev/null && echo 1 || echo 0)
HAVE_SYSTEM_LIBAVFORMAT := $(shell $(PKGCONF) --exists libavformat 2>/dev/null && echo 1 || echo 0)
HAVE_SYSTEM_LIBAVUTIL := $(shell $(PKGCONF) --exists libavutil 2>/dev/null && echo 1 || echo 0)
HAVE_SYSTEM_LIBSWRESAMPLE := $(shell $(PKGCONF) --exists libswresample 2>/dev/null && echo 1 || echo 0)
else
HAVE_SYSTEM_SDL2 := 0
HAVE_SYSTEM_SDL2_TTF := 0
HAVE_SYSTEM_PORTMIDI := 0
HAVE_SYSTEM_LIBAVCODEC := 0
HAVE_SYSTEM_LIBAVFORMAT := 0
HAVE_SYSTEM_LIBAVUTIL := 0
HAVE_SYSTEM_LIBSWRESAMPLE := 0
endif

# Define paths for bundled dependencies
SDL2_BUNDLED_PATH := $(PWD)/SDL
SDL2_TTF_BUNDLED_PATH := $(PWD)/SDL_ttf
PORTMIDI_BUNDLED_PATH := $(PWD)/portmidi

PKG_CONFIG_PATH := $(shell echo $(PKG_CONFIG_PATH))
###############################################################
# For each dep
#	- define build target IFF bundled
#	- append to PKG_CONFIG_PATH IFF bundled

ifeq ($(HAVE_SYSTEM_SDL2),0)
SDL2_BUILD_TARGET := $(SDL2_BUNDLED_PATH)/installation/lib/libSDL2.a
PKG_CONFIG_PATH := $(if $(PKG_CONFIG_PATH),$(PKG_CONFIG_PATH):)$(SDL2_BUNDLED_PATH)/installation/lib/pkgconfig/
else
SDL2_BUILD_TARGET :=
endif

ifeq ($(HAVE_SYSTEM_SDL2_TTF),0)
SDL2_TTF_BUILD_TARGET := $(SDL2_TTF_BUNDLED_PATH)/installation/lib/libSDL2_ttf.a
PKG_CONFIG_PATH := $(if $(PKG_CONFIG_PATH),$(PKG_CONFIG_PATH):)$(SDL2_TTF_BUNDLED_PATH)
else
SDL2_TTF_BUILD_TARGET :=
endif

ifeq ($(HAVE_SYSTEM_PORTMIDI),0)
PORTMIDI_BUILD_TARGET := $(PORTMIDI_BUNDLED_PATH)/build/libportmidi.a
PKG_CONFIG_PATH := $(if $(PKG_CONFIG_PATH),$(PKG_CONFIG_PATH):)$(PORTMIDI_BUNDLED_PATH)/build/packaging
else
PORTMIDI_BUILD_TARGET :=
endif
###############################################################

SDL_INCLUDE_PATH := $(SDL_PATH)/include
PORTMIDI_PATH := $(PWD)/portmidi
PORTMIDI_LIB := $(PORTMIDI_PATH)/build/libportmidi.a


ifdef USE_EXTERNAL_SDLS
SDL2_TTF_LIB :=
else
SDL2_TTF_LIB := $(SDL2_TTF_BUNDLED_PATH)/build/lib/libSDL2_ttf.a
endif

##############################################################
# Bundled dependency build steps.
#
# Make is executed recursively so PKGCONF can be used to set flags

$(SDL2_BUILD_TARGET):
	@echo "\nChecking SDL2 submodule..."
	@if [ ! -e SDL/.git ]; then \
		git submodule update --init --recursive SDL; \
	fi
	@echo "Done.\nConfiguring and building SDL2. This may take several minutes. (Logs in sdl_build.log)..."
	@cd SDL && \
	./configure --enable-static --disable-shared --prefix=$(PWD)/SDL/installation >>../sdl_build.log 2>&1 && \
	make >>../sdl_build.log 2>&1 && \
	make install >>../sdl_build.log 2>&1
	@echo "...SDL build complete"
	$(MAKE) $(MAKE_CMD_GOALS)

$(SDL2_TTF_BUILD_TARGET): $(SDL2_BUILD_TARGET)
	@echo "\nChecking SDL2_ttf submodule..."
	@if [ ! -e SDL_ttf/.git ]; then \
		git submodule update --init --recursive SDL_ttf; \
	fi
	@echo "Done.\nConfiguring and building SDL2_ttf. This may take several minutes. (Logs in sdl_ttf_build.log)..."
	@cd SDL_ttf && \
	mkdir -p build && \
	touch aclocal.m4 && \
	touch configure && \
	touch config.h.in && \
	find . -name 'Makefile.in' -exec touch {} \; && \
	./configure --disable-shared --enable-static --with-sdl-prefix=$(PWD)/SDL/installation --prefix=$(PWD)/sdl_ttf/build --disable-sdltest >>../sdl_ttf_build.log 2>&1 && \
	make >>../sdl_ttf_build.log 2>&1 && \
	make install >>../sdl_ttf_build.log 2>&1 && \
	@echo "...SDL_ttf build complete."
	$(MAKE) $(MAKE_CMD_GOALS)

$(PORTMIDI_BUILD_TARGET):
	@command -v cmake >/dev/null 2>&1 || { \
		echo "\nError: unable to build portmidi from source: cmake is required.\nUse a package manager to install portmidi on your system, or install cmake, and rerun make."; \
		exit 1; \
	}
	@echo "\nChecking SDL2_ttf submodule..."
	@if [ ! -e portmidi/.git ]; then \
		git submodule update --init --recursive portmidi; \
	fi
	@echo "Done.\nConfiguring and building portmidi..."
	(cd portmidi && mkdir -p build && chmod 755 build && cd build && cmake .. -DBUILD_SHARED_LIBS=OFF -DCMAKE_BUILD_TYPE=Release && make)
	@echo "...portmidi build complete."
	$(MAKE) $(MAKE_CMD_GOALS)

##############################################################

##############################################################
# For each dep, assemble compiler flags and linker flags

PKG_CFLAGS :=
PKG_LINK_FLAGS := 

# SDL2
ifeq ($(HAVE_SYSTEM_SDL2),1)
PKG_CFLAGS += $(shell $(PKGCONF) $(SDL2_PKG_NAME) --cflags)
PKG_LINK_FLAGS += $(shell $(PKGCONF) $(SDL2_PKG_NAME) --libs)
else
PKG_CFLAGS += $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) $(PKGCONF) --static $(SDL2_PKG_NAME) --cflags)
PKG_LINK_FLAGS += $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) $(PKGCONF) --static $(SDL2_PKG_NAME) --libs)
endif

# SDL2_ttf
ifeq ($(HAVE_SYSTEM_SDL2_TTF),1)
PKG_CFLAGS += $(shell $(PKGCONF) $(SDL2_TTF_PKG_NAME) --cflags)
PKG_LINK_FLAGS += $(shell $(PKGCONF) $(SDL2_TTF_PKG_NAME) --libs)
else
PKG_CFLAGS += $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) $(PKGCONF) --static $(SDL2_TTF_PKG_NAME) --cflags)
PKG_LINK_FLAGS += $(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) $(PKGCONF) --static $(SDL2_TTF_PKG_NAME) --libs)
endif

# Portmidi
ifeq ($(HAVE_SYSTEM_PORTMIDI),1)
PKG_CFLAGS += $(shell $(PKGCONF) $(PORTMIDI_PKG_NAME) --cflags)
PKG_LINK_FLAGS += $(shell $(PKGCONF) $(PORTMIDI_PKG_NAME) --libs)
else
PKG_CFLAGS += -I$(PORTMIDI_BUNDLED_PATH)/pm_common -I$(PORTMIDI_BUNDLED_PATH)/porttime
endif

# USE_EXTERNAL_SDLS forces the use of system packages; error if unavailable
ifdef USE_EXTERNAL_SDLS
ifeq ($(HAVE_PKGCONF),0)
$(error "pkg-config required when USE_EXTERNAL_SDLS is set")
endif

ifeq ($(HAVE_SYSTEM_SDL2),0)
$(error "SDL2 was not found on your system.")
endif

ifeq ($(HAVE_SYSTEM_SDL2_TTF),0)
$(error "SDL_ttf was not found on your system.")
endif
endif

CC := gcc
SRC_DIR := src
BUILD_DIR := build
GUI_SRC_DIR := gui/src
GUI_BUILD_DIR := gui/build

DEPFLAGS = -MMD -MP -MF $(@:.o=.d)

MACOS_FRAMEWORK_FLAGS := -framework AudioToolBox \
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

# ifdef USE_EXTERNAL_SDLS
# SDL_FLAGS_ALL := 
# else
# SDL_FLAGS_ALL := -I$(SDL_INCLUDE_PATH)
# endif
# LINK_ASOUND :=

# Operation system checks
ifeq ($(UNAME_S),Darwin)
# SDL_FLAGS := $(MACOS_FRAMEWORK_FLAGS) $(SDL_FLAGS_ALL)
LDFLAGS := -lpthread -lm
else
SDL_FLAGS := $(SDL_FLAGS_ALL)
# LINK_ASOUND := -lasound
LDFLAGS := -lpthread -lm -ldl -lrt
endif

LDFLAGS += $(PKG_LINK_FLAGS) $(MACOS_FRAMEWORK_FLAGS)

LIBS := $(SDL2_BUILD_TARGET) $(SDL2_TTF_BUILD_TARGET) $(PORTMIDI_BUILD_TARGET)
# ifdef USE_EXTERNAL_SDLS
# 	LIBS := $(PORTMIDI_LIB)
# 	LDFLAGS += $(shell $(PKGCONF) sdl2 --libs) $(shell $(PKGCONF) SDL2_ttf --libs)

# else
# 	LIBS := $(SDL2_LIB) $(SDL2_TTF_LIB) $(PORTMIDI_LIB)
# endif

CFLAGS := $(PKG_CFLAGS) -Wall -Wno-unused-command-line-argument -I$(SRC_DIR) -I$(GUI_SRC_DIR) \
	-DJACKDAW_VERSION=\"$(JACKDAW_VERSION)\" \
	-DINSTALL_DIR="\"$(PWD)\""

ifeq ($(UNAME_S),Darwin)
CFLAGS += -DJDAW_MACOS_BUILD
else
CFLAGS += -DJDAW_LINUX_BUILD -Wno-format-truncation
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

LT_EXEC := layout

all: $(EXEC)

# $(SDL2_LIB):
# 	@echo "\nConfiguring and building SDL2. This may take several minutes. (Logs in sdl_build.log)..."
# 	@cd SDL && \
# 	./configure --enable-static --prefix=$(PWD)/SDL/installation >>../sdl_build.log 2>&1 && \
# 	make >>../sdl_build.log 2>&1 && \
# 	make install >>../sdl_build.log 2>&1
# 	@echo "...SDL build complete"

# $(SDL2_TTF_LIB):
# 	@echo "\nConfiguring and building SDL2_ttf. This may take several minutes. (Logs in sdl_ttf_build.log)..."
# 	@cd SDL_ttf && \
# 	touch aclocal.m4 && \
# 	touch configure && \
# 	touch config.h.in && \
# 	find . -name 'Makefile.in' -exec touch {} \; && \
# 	./configure --disable-shared --enable-static --with-sdl-prefix=$(PWD)/SDL/installation --prefix=$(PWD)/sdl_ttf/build --disable-sdltest >>../sdl_ttf_build.log 2>&1 && \
# 	make >>../sdl_ttf_build.log 2>&1
# 	@echo "...SDL_ttf build complete."

# $(PORTMIDI_LIB):
# 	@echo "\nConfiguring and building portmidi..."
# 	(cd portmidi && mkdir -p build && chmod 755 build && cd build && cmake .. -DBUILD_SHARED_LIBS=OFF -DCMAKE_BUILD_TYPE=Release && make)
# 	@echo "...portmidi build complete."

# Default to production flags
CFLAGS_ADDTL := $(CFLAGS_PROD)

# Override CFLAGS_ADDTL if the target is "debug"
ifeq ($(MAKECMDGOALS),debug)
    CFLAGS_ADDTL := $(CFLAGS_DEBUG)
endif

ifeq ($(MAKECMDGOALS),layout)
    CFLAGS_ADDTL := $(CFLAGS_DEBUG)
endif

DEP_BUILD_TARGETS := $(SDL2_BUILD_TARGET) $(SDL2_TTF_BUILD_TARGET) $(PORTMIDI_BUILD_TARGET)

$(EXEC): $(DEP_BUILD_TARGETS) $(OBJS) $(GUI_OBJS)
	$(CC) -o $@  $(filter-out %_target,$^) $(CFLAGS) $(CFLAGS_ADDTL) $(CFLAGS_JDAW_ONLY) $(SDL_FLAGS) $(LIBS) $(LINK_ASOUND) $(LDFLAGS)
	@echo "\nBuild complete. Run jackdaw with:\n$ ./jackdaw\n"

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

.PHONY: macos_bundle
macos_bundle: $(EXEC)
	cp -r assets/* macos_bundle/Jackdaw.app/Contents/Resources/ \
	&& cp jackdaw macos_bundle/Jackdaw.app/Contents/MacOS/ \
	&& sed "s/@VERSION@/$(JACKDAW_VERSION)/g" macos_bundle/Info.plist.in > macos_bundle/Jackdaw.app/Contents/Info.plist

clean:
	@[ -n "${BUILD_DIR}" ] || { echo "BUILD_DIR unset or null"; exit 127; }
	@[ -n "${GUI_BUILD_DIR}" ] || { echo "GUI_BUILD_DIR unset or null"; exit 127; }
	rm -rf $(BUILD_DIR)/* $(GUI_BUILD_DIR)/*

cleanall:
	@[ -n "${BUILD_DIR}" ] || { echo "BUILD_DIR unset or null"; exit 127; }
	@[ -n "${GUI_BUILD_DIR}" ] || { echo "GUI_BUILD_DIR unset or null"; exit 127; }
	rm -rf $(BUILD_DIR)/* $(GUI_BUILD_DIR)/*
	git submodule foreach --recursive git clean -fdx

dump-vars:
	@$(foreach v,$(filter-out .VARIABLES,$(.VARIABLES)), \
	    $(info $(v) = $($(v))) \
	)
