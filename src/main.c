/*****************************************************************************************************************
  Jackdaw | a stripped-down, keyboard-focused Digital Audio Workstation | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023 Charlie Volow
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:
  
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
  
  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.

*****************************************************************************************************************/

/*****************************************************************************************************************
    main.c

    * initialize resources
    * call project_loop
 *****************************************************************************************************************/

/* #include <sys/param.h> */
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "SDL.h"
#include "SDL_ttf.h"
#include "dir.h"
#include "dsp.h"
#include "input.h"
#include "layout.h"
#include "layout_xml.h"
#include "project.h"
#include "pure_data.h"
#include "text.h"
#include "transport.h"
#include "wav.h"
#include "waveform.h"
#include "window.h"

#include "audio_connection.h"

/* #define JACKDAW_VERSION "0.2.0" */

#define LT_DEV_MODE 0

#ifndef INSTALL_DIR
#define INSTALL_DIR ""
#endif

#define WINDOW_DEFAULT_W 900
#define WINDOW_DEFAULT_H 800
/* #define OPEN_SANS_PATH INSTALL_DIR "/assets/ttf/OpenSans-Regular.ttf" */
#define DEFAULT_KEYBIND_CFG_PATH INSTALL_DIR "/assets/key_bindings/default.yaml"

#define DEFAULT_PROJ_AUDIO_SETTINGS 2, 48000, AUDIO_S16SYS, 512, 4096

const char *JACKDAW_VERSION = "0.2.0";
char DIRPATH_SAVED_PROJ[MAX_PATHLEN];
char DIRPATH_OPEN_FILE[MAX_PATHLEN];
char DIRPATH_EXPORT[MAX_PATHLEN];

bool sys_byteorder_le = false;

Window *main_win;
Project *proj = NULL;
/* char *saved_proj_path = NULL; */

static void get_native_byte_order()
{
    #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    sys_byteorder_le = true;
    #else
    sys_byteorder_le = false;
    #endif
}

/* Initialize SDL Video and TTF */
static void init_SDL()
{
    /* if (SDL_Init(SDL_INIT_VIDEO) != 0) { */
    /*     fprintf(stderr, "Error initializing SDL: %s\n", SDL_GetError()); */
    /*     exit(1); */
    /* } */
    if (SDL_Init(SDL_INIT_AUDIO) != 0) {
        fprintf(stderr, "Error initializing audio: %s\n", SDL_GetError());
        exit(1);
    }
    if (TTF_Init() != 0) {
        fprintf(stderr, "Error initializing SDL_ttf: %s\n", SDL_GetError());
        exit(1);
    }

    /* int numDrivers = SDL_GetNumAudioDrivers(); */
    /* fprintf(stdout, "Number of drivers found: %d", numDrivers); */
    /* for(int i = 0 ; i < numDrivers; i ++) */
    /* { */
    /* 	SDL_Log(" Index value: %d = %s", i, SDL_GetAudioDriver(i)); */
    /* } */
    /* SDL_AudioInit("dummy"); */
    /* SDL_SetHint(SDL_HINT_MOUSE_TOUCH_EVENTS, "1"); */
    /* SDL_StopTextInput(); */
}

static void init()
{
    init_SDL();
    /* get_native_byte_order(); */
    /* input_init_hash_table(); */
    /* input_init_mode_load_all(); */
    /* input_load_keybinding_config(DEFAULT_KEYBIND_CFG_PATH); */
    /* pd_jackdaw_shm_init(); */
    /* char *realpath_ret; */
    /* if (!(realpath_ret = realpath(".", NULL))) { */
    /* 	perror("Error in realpath"); */
    /* } else { */
    /* 	strncpy(DIRPATH_SAVED_PROJ, realpath_ret, MAX_PATHLEN); */
    /* 	free(realpath_ret); */
    /* } */
    /* strcpy(DIRPATH_OPEN_FILE, DIRPATH_SAVED_PROJ); */
    /* strcpy(DIRPATH_EXPORT, DIRPATH_SAVED_PROJ); */
    /* /\* fprintf(stdout, "Initializing dsp...\n"); *\/ */
    /* init_dsp(); */
}
static void take_time()
{
    for (int i=0; i<10000000; i++) {
	printf("i: %d\n", i);
    }
    
}
static void quit()
{
    pd_signal_termination_of_jackdaw();
    if (proj->recording) {
	transport_stop_recording();
    }
    transport_stop_playback();
    /* Sleep to allow DSP thread to exit */
    SDL_Delay(100);
    project_destroy(proj);
    if (main_win) {
	window_destroy(main_win);
    }
    input_quit();
    take_time();
    SDL_Quit();
}

void loop_project_main();
Project *jdaw_read_file(const char *path);

extern bool connection_open;

void dir_tests();
int main(int argc, char **argv)
{

    fprintf(stdout, "\n\nJACKDAW (version %s)\nby Charlie Volow\n\n", JACKDAW_VERSION);
    
    init();

    /* Create a window, assign a std_font, and set a main layout */
    /* main_win = window_create(WINDOW_DEFAULT_W, WINDOW_DEFAULT_H, "Jackdaw"); */
    /* window_assign_font(main_win, OPEN_SANS_PATH, REG);  */

    proj = project_create("project.jdaw", DEFAULT_PROJ_AUDIO_SETTINGS);

    loop_project_main();
    
    quit();
    
}
