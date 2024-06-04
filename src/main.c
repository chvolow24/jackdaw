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

#include <stdbool.h>
#include <stdio.h>
#include <time.h>
#include "SDL.h"
#include "SDL_ttf.h"
#include "dir.h"
#include "input.h"
#include "layout.h"
#include "layout_xml.h"
#include "project.h"
#include "pure_data.h"
#include "text.h"
#include "transport.h"
#include "wav.h"
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

#define DEFAULT_PROJ_AUDIO_SETTINGS 2, 48000, AUDIO_S16SYS, 64

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
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "Error initializing SDL: %s\n", SDL_GetError());
        exit(1);
    }
    if (SDL_Init(SDL_INIT_AUDIO) != 0) {
        fprintf(stderr, "Error initializing audio: %s\n", SDL_GetError());
        exit(1);
    }
    if (TTF_Init() != 0) {
        fprintf(stderr, "Error initializing SDL_ttf: %s\n", SDL_GetError());
        exit(1);
    }
    SDL_SetHint(SDL_HINT_MOUSE_TOUCH_EVENTS, "1");
    SDL_StopTextInput();
}

static void init()
{
    init_SDL();
    get_native_byte_order();
    input_init_hash_table();
    input_init_mode_load_all();
    input_load_keybinding_config(DEFAULT_KEYBIND_CFG_PATH);
    pd_jackdaw_shm_init();
    realpath(".", DIRPATH_SAVED_PROJ);
    strcpy(DIRPATH_OPEN_FILE, DIRPATH_SAVED_PROJ);
    strcpy(DIRPATH_EXPORT, DIRPATH_SAVED_PROJ);
}

static void quit()
{
    pd_signal_termination_of_jackdaw();
    if (proj->recording) {
	transport_stop_recording();
    }
    transport_stop_playback();

    project_destroy(proj);
    if (main_win) {
	window_destroy(main_win);
    }
    input_quit();
    SDL_Quit();
}

void loop_project_main();
Project *jdaw_read_file(const char *path);

extern bool connection_open;

void dir_tests();
int main(int argc, char **argv)
{
    /* dir_tests(); */
    /* exit(0); */
    fprintf(stdout, "\n\nJACKDAW (version %s)\nby Charlie Volow\n\n", JACKDAW_VERSION);
    
    init();

    /* pd_open_shared_memory(); */

    char *file_to_open = NULL;
    bool invoke_open_wav_file = false;
    bool invoke_open_jdaw_file = false;
    if (argc > 2) {
        exit(1);
    } else if (argc == 2) {
	if (strcmp(argv[1], "pd_test") == 0) {
	    /* while (!connection_open) { */
	    /* 	fprintf(stdout, "connection not open\n"); */
	    /* 	SDL_Delay(500); */
	    /* } */
	    /* fprintf(stdout, "REC GET BLOCK\n"); */
	    /* /\* pd_jackdaw_record_get_block(); *\/ */
	    exit(0);
	}
	file_to_open = argv[1];
	char *dotpos = strrchr(file_to_open, '.');
	char *ext = dotpos + 1;
	if (strcmp("wav", ext) * strcmp("WAV", ext) == 0) {
	    fprintf(stderr, "Passed WAV file.\n");
	    invoke_open_wav_file = true;
	} else if (strcmp("jdaw", ext) * strcmp("JDAW", ext) == 0) {
	    fprintf(stderr, "Passed JDAW file.\n");
	    invoke_open_jdaw_file = true;
	} else {
	    fprintf(stderr, "Error: argument \"%s\" not recognized. Pass a .jdaw or .wav file to open that file.\n", argv[1]);
	    exit(1);
	}
    }

    /* Create a window, assign a std_font, and set a main layout */
    main_win = window_create(WINDOW_DEFAULT_W, WINDOW_DEFAULT_H, "Jackdaw");
    window_assign_font(main_win, OPEN_SANS_PATH, REG);
    window_assign_font(main_win, OPEN_SANS_BOLD_PATH, BOLD);
    window_assign_font(main_win, LTSUPERIOR_PATH, MONO);
    window_assign_font(main_win, LTSUPERIOR_BOLD_PATH, MONO_BOLD);


    
    
    /* Create project here */

    if (invoke_open_jdaw_file) {
	proj = jdaw_read_file(file_to_open);
	if (proj) {
	    realpath(file_to_open, DIRPATH_SAVED_PROJ);
	    char *last_slash_pos = strrchr(DIRPATH_SAVED_PROJ, '/');
	    if (last_slash_pos) {
		*last_slash_pos = '\0';
		fprintf(stdout, "SAVED PROJ DIRPATH: %s\n", DIRPATH_SAVED_PROJ);
	    } else {
	        realpath(".", DIRPATH_SAVED_PROJ);
		fprintf(stderr, "Error: no slash in saved proj dir");
	    }
	    for (int i=0; i<proj->num_timelines; i++) {
		timeline_reset_full(proj->timelines[i]);
	    }
	}
    } else {
	proj = project_create("project.jdaw", DEFAULT_PROJ_AUDIO_SETTINGS);
    }

    
    if (!proj && invoke_open_jdaw_file) {
	fprintf(stderr, "Error: unable to open project \"%s\".\n", file_to_open);
	exit(1);
    } else if (!proj) {
	fprintf(stderr, "Critical error: unable to create project\n");
    }

    if (invoke_open_wav_file) {
	Track *track = timeline_add_track(proj->timelines[0]);
	wav_load_to_track(track, file_to_open, 0);

    }
    /* timeline_add_track(proj->timelines[0]); */
    /* timeline_add_track(proj->timelines[0]); */
    /* timeline_add_track(proj->timelines[0]); */


    loop_project_main();
    
    quit();
    
}
