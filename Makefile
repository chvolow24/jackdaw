# JACKDAW_VERSION used in runtime and macos_bundle defined here
JACKDAW_VERSION := 0.8.1
EXEC := jackdaw

# Default target
all: $(EXEC)

# Printed at the end of a successful build
BUILD_SUMMARY := "\nBuild complete. Summary:"

# PKGCONF is used to locate system packages
PKGCONF := $(shell command -v pkg-config 2>/dev/null || command -v pkgconf 2>/dev/null)
ifeq ($(PKGCONF),)
$(error pkg-config is required. Please install it first.)
endif

# Canonical pkg-config package names
SDL2_PKG_NAME := sdl2
SDL2_TTF_PKG_NAME := SDL2_ttf
PORTMIDI_PKG_NAME := portmidi
LIBAVCODEC_PKG_NAME := libavcodec
LIBAVFORMAT_PKG_NAME := libavformat
LIBAVUTIL_PKG_NAME := libavutil
LIBSWRESAMPLE_PKG_NAME := libswresample
FFMPEG_PKG_NAME := $(LIBAVCODEC_PKG_NAME) $(LIBAVFORMAT_PKG_NAME) $(LIBAVUTIL_PKG_NAME) $(LIBSWRESAMPLE_PKG_NAME)

# Check for system packages, or hide them if BUNDLED option set
ifndef BUNDLED
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

# Check if all FFMPEG components available
ifeq ($(HAVE_SYSTEM_LIBAVCODEC)$(HAVE_SYSTEM_LIBAVFORMAT)$(HAVE_SYSTEM_LIBAVUTIL)$(HAVE_SYSTEM_LIBSWRESAMPLE),1111)
HAVE_SYSTEM_FFMPEG := 1
else
HAVE_SYSTEM_FFMPEG := 0
endif

# BUNDLED_ flags: hide system packages

ifdef BUNDLED_SDL
HAVE_SYSTEM_SDL2 := 0
HAVE_SYSTEM_SDL2_TTF := 0
endif

ifdef BUNDLED_SDL_TTF
HAVE_SYSTEM_SDL2_TTF := 0
endif

ifdef BUNDLED_PORTMIDI
HAVE_SYSTEM_PORTMIDI := 0
endif

ifdef BUNDLED_FFMPEG
HAVE_SYSTEM_FFMPEG := 0
HAVE_SYSTEM_LIBAVCODEC := 0
HAVE_SYSTEM_LIBAVFORMAT := 0
HAVE_SYSTEM_LIBAVUTIL := 0
HAVE_SYSTEM_LIBSWRESAMPLE := 0
endif


# Define paths for bundled dependencies
SDL2_BUNDLED_PATH := $(CURDIR)/SDL
SDL2_TTF_BUNDLED_PATH := $(CURDIR)/SDL_ttf
PORTMIDI_BUNDLED_PATH := $(CURDIR)/portmidi
FFMPEG_BUNDLED_PATH := $(CURDIR)/FFmpeg

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

ifeq ($(HAVE_SYSTEM_FFMPEG),0)
PKG_CONFIG_PATH := $(if $(PKG_CONFIG_PATH),$(PKG_CONFIG_PATH):)$(FFMPEG_BUNDLED_PATH)/installation/lib/pkgconfig/
LIBAVCODEC_BUILD_TARGET := $(FFMPEG_BUNDLED_PATH)/installation/lib/libavcodec.a
LIBAVFORMAT_BUILD_TARGET := $(FFMPEG_BUNDLED_PATH)/installation/lib/libavformat.a
LIBSWRESAMPLE_BUILD_TARGET := $(FFMPEG_BUNDLED_PATH)/installation/lib/libswresample.a
LIBAVUTIL_BUILD_TARGET := $(FFMPEG_BUNDLED_PATH)/installation/lib/libavutil.a
FFMPEG_BUILD_TARGET := $(LIBAVCODEC_BUILD_TARGET) $(LIBAVFORMAT_BUILD_TARGET) $(LIBAVUTIL_BUILD_TARGET) $(LIBSWRESAMPLE_BUILD_TARGET)
else
FFMPEG_BUILD_TARGET := 
endif
###############################################################


##############################################################
# Bundled dependency build steps.
#
# Make is executed recursively so PKGCONF can be used to set flags

$(SDL2_BUILD_TARGET):
	@echo "\nChecking SDL2 submodule..."
	@if [ ! -e SDL/.git ]; then \
		git submodule update --init --recursive SDL; \
	fi
	@echo "Done.\n\nConfiguring and building SDL2. This may take several minutes. (Logs in sdl_build.log)\n\n\tNote: if you prefer to install SDL2 on your system: \n\t\t- cancel this (CTRL-c)\n\t\t- install it (e.g. 'sudo apt install libsdl2-dev', 'brew install sdl2')\n\t\t- re-run make\n"
	@cd SDL && \
	mkdir -p installation && \
	./configure --enable-static --disable-shared --prefix=$(SDL2_BUNDLED_PATH)/installation >>../sdl_build.log 2>&1 && \
	make >>../sdl_build.log 2>&1 && \
	make install >>../sdl_build.log 2>&1
	@echo "...SDL build complete"

SDL2_TTF_CONFIG_OPTIONS := --disable-shared --enable-static --prefix=$(SDL2_TTF_BUNDLED_PATH)/installation --disable-sdltest
ifeq ($(HAVE_SYSTEM_SDL2),0)
SDL2_TTF_CONFIG_OPTIONS += --with-sdl-prefix=$(SDL2_BUNDLED_PATH)/installation
endif

$(SDL2_TTF_BUILD_TARGET): $(SDL2_BUILD_TARGET)
	@echo "\nChecking SDL2_ttf submodule..."
	@if [ ! -e SDL_ttf/.git ]; then \
		git submodule update --init --recursive SDL_ttf; \
	fi
	@echo "Done.\n\nConfiguring and building SDL2_ttf. This may take several minutes. (Logs in sdl_ttf_build.log)\n\n\tNote: if you prefer to install SDL2_ttf on your system: \n\t\t- cancel this (CTRL-c)\n\t\t- install it (e.g. 'sudo apt install libsdl2-ttf-dev', 'brew install sdl2_ttf')\n\t\t- re-run make\n"	
	@cd SDL_ttf && \
	touch aclocal.m4 && \
	touch configure && \
	touch config.h.in && \
	find . -name 'Makefile.in' -exec touch {} \; && \
	mkdir -p installation && \
	./configure $(SDL2_TTF_CONFIG_OPTIONS) \
		PKG_CONFIG_PATH=$(SDL2_BUNDLED_PATH)/installation/lib/pkgconfig \
		>>../sdl_ttf_build.log 2>&1 && \
	sed -i.bak 's|^SDL_LIBS = -L[^ ]* [^ ]*\.a|SDL_LIBS = -L$(SDL2_BUNDLED_PATH)/installation/lib -lSDL2|' $(SDL2_TTF_BUNDLED_PATH)/Makefile && \
	sed -i.bak 's|^LIBS =  -L[^ ]* [^ ]*\.a|LIBS = -L$(SDL2_BUNDLED_PATH)/installation/lib -lSDL2|' $(SDL2_TTF_BUNDLED_PATH)/Makefile && \
	make >>../sdl_ttf_build.log 2>&1 && \
	make install >>../sdl_ttf_build.log 2>&1
	@echo "...SDL_ttf build complete."

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

$(FFMPEG_BUILD_TARGET):
	@echo "\nChecking FFmpeg submodule..."
	@if [ ! -e FFmpeg/.git ]; then \
		git submodule update --init --recursive FFmpeg; \
	fi
	@echo "Done.\n\nConfiguring and building FFmpeg. This may take several minutes. (Logs in ffmpeg_build.log)\n\n\tNote: if you prefer to install FFmpeg on your system: \n\t\t- cancel this (CTRL-c)\n\t\t- install it (e.g. 'sudo apt install ffmpeg', 'brew install ffmpeg')\n\t\t- re-run make\n"
	@cd FFmpeg && \
	mkdir -p installation && \
	./configure \
		--prefix="$(FFMPEG_BUNDLED_PATH)/installation" \
		--disable-everything \
		--enable-static \
		--disable-shared \
		--enable-avcodec \
		--enable-avformat \
		--enable-avutil \
		--enable-swresample \
		--enable-protocol=file,pipe \
		--enable-demuxer=aac,aiff,ape,asf,au,caf,flac,matroska,mov,mp3,mpc,ogg,tta,wav,wv,ac3,eac3,dts,amr,dsf \
		--enable-muxer=adts,aiff,caf,flac,ipod,matroska,mp3,mov,null,ogg,wav,opus \
		--enable-decoder=ac3,eac3,alac,ape,atrac1,atrac3,atrac3p,flac,gsm,mp1,mp2,mp3,aac,opus,vorbis,wavpack,wmav1,wmav2,wmavoice,pcm_alaw,pcm_mulaw,pcm_f32be,pcm_f32le,pcm_f64be,pcm_f64le,pcm_s8,pcm_s16be,pcm_s16le,pcm_s24be,pcm_s24le,pcm_s32be,pcm_s32le,pcm_u8 \
		--enable-encoder=ac3,eac3,alac,ape,atrac1,atrac3,atrac3p,flac,gsm,mp1,mp2,mp3,aac,opus,vorbis,wavpack,wmav1,wmav2,wmavoice,pcm_alaw,pcm_mulaw,pcm_f32be,pcm_f32le,pcm_f64be,pcm_f64le,pcm_s8,pcm_s16be,pcm_s16le,pcm_s24be,pcm_s24le,pcm_s32be,pcm_s32le,pcm_u8 \
		--enable-parser=ac3,dca,eac3,flac,mpegaudio,aac,opus,vorbis \
		--enable-bsf=aac_adtstoasc \
		>>../ffmpeg_build.log 2>&1 && \
	make >>../ffmpeg_build.log 2>&1 && \
	make install >>../ffmpeg_build.log 2>&1
	@echo "...FFmpeg build complete"


##############################################################

# USE_EXTERNAL_SDLS forces the use of system packages; error if unavailable
ifdef USE_EXTERNAL_SDLS
ifeq ($(HAVE_SYSTEM_SDL2),0)
$(error "SDL2 was not found on your system.")
endif

ifeq ($(HAVE_SYSTEM_SDL2_TTF),0)
$(error "SDL_ttf was not found on your system.")
endif
endif

DEP_BUILD_TARGETS := $(SDL2_BUILD_TARGET) $(SDL2_TTF_BUILD_TARGET) $(PORTMIDI_BUILD_TARGET) $(FFMPEG_BUILD_TARGET)

# 'deps-ready' adds to compiler directives using module .pc files
.PHONY: deps-ready
deps-ready: $(DEP_BUILD_TARGETS)
	$(eval PKG_CFLAGS := $(PKG_CFLAGS))
	$(eval PKG_LINK_FLAGS := $(PKG_LINK_FLAGS))

# SDL2
	$(eval PKG_CFLAGS += $(if $(filter 1,$(HAVE_SYSTEM_SDL2)),\
		$(shell $(PKGCONF) $(SDL2_PKG_NAME) --cflags),\
		$(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) $(PKGCONF) --static $(SDL2_PKG_NAME) --cflags)))
	$(eval PKG_LINK_FLAGS += $(if $(filter 1,$(HAVE_SYSTEM_SDL2)),\
		$(shell $(PKGCONF) $(SDL2_PKG_NAME) --libs),\
		$(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) $(PKGCONF) --static $(SDL2_PKG_NAME) --libs)))

# SDL2_TTF
	$(eval PKG_CFLAGS += $(if $(filter 1,$(HAVE_SYSTEM_SDL2_TTF)),\
		$(shell $(PKGCONF) $(SDL2_TTF_PKG_NAME) --cflags),\
		$(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) $(PKGCONF) --static $(SDL2_TTF_PKG_NAME) --cflags)))
	$(eval PKG_LINK_FLAGS += $(if $(filter 1,$(HAVE_SYSTEM_SDL2_TTF)),\
		$(shell $(PKGCONF) $(SDL2_TTF_PKG_NAME) --libs),\
		$(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) $(PKGCONF) --static $(SDL2_TTF_PKG_NAME) --libs)))

# portmidi
	$(eval PKG_CFLAGS += $(if $(filter 1,$(HAVE_SYSTEM_PORTMIDI)),\
		$(shell $(PKGCONF) $(PORTMIDI_PKG_NAME) --cflags),\
		-I$(PORTMIDI_BUNDLED_PATH)/pm_common -I$(PORTMIDI_BUNDLED_PATH)/porttime))
	$(eval PKG_LINK_FLAGS += $(if $(filter 1,$(HAVE_SYSTEM_PORTMIDI)),\
		$(shell $(PKGCONF) $(PORTMIDI_PKG_NAME) --libs),))


# FFmpeg
	$(eval PKG_CFLAGS += $(if $(filter 1,$(HAVE_SYSTEM_FFMPEG)),\
		$(shell $(PKGCONF) $(FFMPEG_PKG_NAME) --cflags),\
		$(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) $(PKGCONF) --static $(FFMPEG_PKG_NAME) --cflags)))
	$(eval PKG_LINK_FLAGS += $(if $(filter 1,$(HAVE_SYSTEM_FFMPEG)),\
		$(shell $(PKGCONF) $(FFMPEG_PKG_NAME) --libs),\
		$(shell PKG_CONFIG_PATH=$(PKG_CONFIG_PATH) $(PKGCONF) --static $(FFMPEG_PKG_NAME) --libs)))


# Build summary
	$(eval BUILD_SUMMARY := $(BUILD_SUMMARY))
	$(eval BUILD_SUMMARY += "\n\t- SDL2:          ")
	$(eval BUILD_SUMMARY += $(if $(filter 0,$(HAVE_SYSTEM_SDL2)),"bundled ($(CURDIR)/SDL/)","v$(shell $(PKGCONF) $(SDL2_PKG_NAME) --modversion) found on your system"))
	$(eval BUILD_SUMMARY += "\n\t- SDL2_ttf:      ")
	$(eval BUILD_SUMMARY += $(if $(filter 0,$(HAVE_SYSTEM_SDL2_TTF)),"bundled ($(CURDIR)/SDL_ttf/)","v$(shell $(PKGCONF) $(SDL2_TTF_PKG_NAME) --modversion) found on your system"))
	$(eval BUILD_SUMMARY += "\n\t- Portmidi:      ")
	$(eval BUILD_SUMMARY += $(if $(filter 0,$(HAVE_SYSTEM_PORTMIDI)),"bundled ($(CURDIR)/portmidi/)","v$(shell $(PKGCONF) $(PORTMIDI_PKG_NAME) --modversion) found on your system"))
	$(eval BUILD_SUMMARY += "\n\t- libavcodec:    ")
	$(eval BUILD_SUMMARY += $(if $(filter 0,$(HAVE_SYSTEM_LIBAVCODEC)),"bundled ($(CURDIR)/FFmpeg/)","v$(shell $(PKGCONF) $(LIBAVCODEC_PKG_NAME) --modversion) found on your system"))
	$(eval BUILD_SUMMARY += "\n\t- libavformat:   ")
	$(eval BUILD_SUMMARY += $(if $(filter 0,$(HAVE_SYSTEM_LIBAVFORMAT)),"bundled ($(CURDIR)/FFmpeg/)","v$(shell $(PKGCONF) $(LIBAVFORMAT_PKG_NAME) --modversion) found on your system"))
	$(eval BUILD_SUMMARY += "\n\t- libavutil:     ")
	$(eval BUILD_SUMMARY += $(if $(filter 0,$(HAVE_SYSTEM_LIBAVUTIL)),"bundled ($(CURDIR)/FFmpeg/)","v$(shell $(PKGCONF) $(LIBAVUTIL_PKG_NAME) --modversion) found on your system"))
	$(eval BUILD_SUMMARY += "\n\t- libswresample: ")
	$(eval BUILD_SUMMARY += $(if $(filter 0,$(HAVE_SYSTEM_LIBSWRESAMPLE)),"bundled ($(CURDIR)/FFmpeg/)","v$(shell $(PKGCONF) $(LIBSWRESAMPLE_PKG_NAME) --modversion) found on your system"))
	$(eval BUILD_SUMMARY += "\n\nRun jackdaw with:\n./jackdaw\n")

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

# Operation system checks
ifeq ($(UNAME_S),Darwin)
LDFLAGS := -lpthread -lm $(MACOS_FRAMEWORK_FLAGS)
else
LDFLAGS = -lpthread -lm -ldl -lrt -lasound
endif

CFLAGS = $(PKG_CFLAGS) -Wall -Wno-unused-command-line-argument -I$(SRC_DIR) -I$(GUI_SRC_DIR) \
	-DJACKDAW_VERSION=\"$(JACKDAW_VERSION)\" \
	-DINSTALL_DIR="\"$(PWD)\""

ifeq ($(UNAME_S),Darwin)
CFLAGS += -DJDAW_MACOS_BUILD
else
CFLAGS += -DJDAW_LINUX_BUILD -Wno-format-truncation
endif

CFLAGS_JDAW_ONLY := -DLT_DEV_MODE=0
CFLAGS_LT_ONLY := -DLT_DEV_MODE=1 -DLAYOUT_BUILD=1

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


# Add cflags for debug or prod
ifeq ($(MAKECMDGOALS),debug)
	CFLAGS += -DTESTBUILD=1 -g -O0 -fsanitize=address
else
	CFLAGS += -O3 
endif

# Main target
$(EXEC): $(OBJS) $(GUI_OBJS) | deps-ready
	$(CC) -o $@  $(filter-out deps-ready %_target,$^) $(CFLAGS) $(CFLAGS_JDAW_ONLY) $(DEP_BUILD_TARGETS) $(PKG_LINK_FLAGS) $(LDFLAGS)
	@echo $(BUILD_SUMMARY)

.PHONY: debug
debug: $(EXEC)

$(LT_EXEC): $(LT_OBJS)
	$(CC) -o $@ $^ $(CFLAGS) $(CFLAGS_LT_ONLY) $(LDFLAGS)

$(BUILD_DIR):
	mkdir $(BUILD_DIR)

$(GUI_BUILD_DIR):
	mkdir -p $(GUI_BUILD_DIR)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR) deps-ready
	$(CC) $(CFLAGS) $(PKG_CFLAGS) $(DEPFLAGS) -c $< -o $@

$(GUI_BUILD_DIR)/%.o: $(GUI_SRC_DIR)/%.c | $(GUI_BUILD_DIR) deps-ready
	$(CC) $(CFLAGS) $(PKG_CFLAGS) $(DEPFLAGS) -c $< -o $@

-include ${DEPS}
-include ${GUI_DEPS}

.PHONY: macos-bundle
macos-bundle: $(EXEC)
	mkdir -p macos_bundle/Jackdaw.app/Contents \
	&& mkdir -p macos_bundle/Jackdaw.app/Contents/Resources \
	&& mkdir -p macos_bundle/Jackdaw.app/Contents/MacOS \
	&& cp -r assets/* macos_bundle/Jackdaw.app/Contents/Resources/ \
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

.PHONY: options
.PHONY: help
options:
	@echo "\n\nJackdaw's Makefile supports the following options:\n\
\n\tmake                    Build Jackdaw with default options \
\n\tmake debug              Build a debug version of Jackdaw (runs much slower, better crash reports) \
\n\tmake clean              Clean Jackdaw's build artifacts, but leave dependencies in place \
\n\tmake cleanall           Clean Jackdaw's build artifacts and those of any vendored dependency \
\n\tmake layout             Build the layout program (undocumented, for use by the author) \
\n\tmake options            Prints this message \
\n\tmake help               Prints this message \
\n\tmake BUNDLED=1          Use vendored copies of each dependency and link them statically \
\n\tmake BUNDLED_SDL=1      Ignore system SDL2 and SDL2_ttf if exist -- use vendored \
\n\tmake BUNDLED_SDL_TTF=1  Ignore system SDL2_ttf if exists -- use vendored \
\n\tmake BUNDLED_PORTMIDI=1 Ignore system portmidi if exists -- use vendored \
\n\tmake BUNDLED_FFMPEG=1   Ignore system FFmpeg if exists -- use vendored\n" 

help: options
