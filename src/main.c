/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

/*****************************************************************************************************************
    main.c

    * (no header file)
    * initialize resources
    * call project_loop
    * exit
 *****************************************************************************************************************/

#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "SDL.h"
#include "SDL_ttf.h"
#include "assets.h"
#include "consts.h"
#include "dir.h"
#include "dot_jdaw.h"
#include "dsp_utils.h"
#include "function_lookup.h"
#include "init_panels.h"
#include "input.h"
#include "midi_io.h"
#include "midi_file.h"
#include "midi_qwerty.h"
#include "project.h"
#include "pure_data.h"
#include "session.h"
#include "symbol.h"
#include "tempo.h"
#include "text.h"
#include "transport.h"
#include "wav.h"
#include "window.h"

#include "audio_connection.h"

#define LT_DEV_MODE 0

#define WINDOW_DEFAULT_W 1093
#define WINDOW_DEFAULT_H 650

#define DEFAULT_PROJ_AUDIO_SETTINGS 2, DEFAULT_SAMPLE_RATE, DEFAULT_SAMPLE_FORMAT, DEFAULT_AUDIO_CHUNK_LEN_SFRAMES, DEFAULT_FOURIER_LEN_SFRAMES

const char *JACKDAW_VERSION = "0.6.0";
char DIRPATH_SAVED_PROJ[MAX_PATHLEN];
char DIRPATH_OPEN_FILE[MAX_PATHLEN];
char DIRPATH_EXPORT[MAX_PATHLEN];

bool SYS_BYTEORDER_LE = false;
volatile bool CANCEL_THREADS = false;

Window *main_win = NULL;

/* extern pthread_t MAIN_THREAD_ID; */
/* extern pthread_t DSP_THREAD_ID; */
/* extern pthread_t CURRENT_THREAD_ID; */

static void get_native_byte_order()
{
    #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    SYS_BYTEORDER_LE = true;
    #else
    SYS_BYTEORDER_LE = false;
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
    /* FILE *test = open_asset("layouts/jackdaw_main_layout.xml", "r"); */
    /* exit(0); */
    fprintf(stderr, "Initializing SDL and subsystems...\n");
    set_thread_id(JDAW_THREAD_MAIN);
    /* MAIN_THREAD_ID = pthread_self(); */
    /* CURRENT_THREAD_ID = MAIN_THREAD_ID; */
    init_SDL();
    get_native_byte_order();
    input_init_hash_table();
    input_init_mode_load_all();
    input_load_keybinding_config(DEFAULT_KEYBIND_CFG_PATH);
    mqwert_init();
    pd_jackdaw_shm_init();
    char *realpath_ret;
    if (!(realpath_ret = realpath(".", NULL))) {
	perror("Error in realpath");
    } else {
	strncpy(DIRPATH_SAVED_PROJ, realpath_ret, MAX_PATHLEN);
	free(realpath_ret);
    }
    strcpy(DIRPATH_OPEN_FILE, DIRPATH_SAVED_PROJ);
    strcpy(DIRPATH_EXPORT, DIRPATH_SAVED_PROJ);
    midi_io_init();
    init_dsp();
    fprintf(stderr, "\t...done.\n");
}

static void quit()
{
    Session *session = session_get();
    api_quit();
    CANCEL_THREADS = true;
    pd_signal_termination_of_jackdaw();
    if (session->playback.recording) {
	transport_stop_recording();
    }
    transport_stop_playback();
    /* Sleep to allow DSP thread to exit */
    SDL_Delay(100);
    session_destroy();
    symbol_quit(main_win);
    if (main_win) {
	window_destroy(main_win);
    }
    function_lookup_deinit();
    input_quit();
    midi_io_deinit();
    SDL_Quit();
}

void loop_project_main();

extern bool connection_open;


static const char *license_text = "Copyright (C) 2023-2025 Charlie Volow\nThis program is free software: you can redistribute it and/or modify \nit under the terms of the GNU General Public License as published by \nthe Free Software Foundation, either version 3 of the License, or \n(at your option) any later version. \n\nThis program is distributed in the hope that it will be useful,\nbut WITHOUT ANY WARRANTY; without even the implied warranty of\nMERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\nGNU General Public License for more details.";


int main(int argc, char **argv)
{
    fprintf(stdout, "\n\nJACKDAW (version %s)\nby Charlie Volow\n\nhttps://jackdaw-audio.net/\n\n%s\n\n", JACKDAW_VERSION, license_text);
    
    init();

    char *file_to_open = NULL;
    bool invoke_open_wav_file = false;
    bool invoke_open_jdaw_file = false;
    bool invoke_open_midi_file = false;
    if (argc > 2) {
	fprintf(stderr, "Usage: jackdaw [file_to_open]");
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
	} else if (strcmp(argv[1], "fn_ref") == 0) {
	    input_create_function_reference();
	    exit(0);
	}
	file_to_open = argv[1];
	char *dotpos = strrchr(file_to_open, '.');
	if (!dotpos) goto unrecognized_arg;
	char *ext = dotpos + 1;
	if (strncmp("wav", ext, 3) * strcmp("WAV", ext) == 0) {
	    fprintf(stderr, "Passed WAV file.\n");
	    invoke_open_wav_file = true;
	} else if (strncmp("jdaw", ext, 4) * strcmp("JDAW", ext) * strcmp("bak", ext)  == 0) {
	    fprintf(stderr, "Passed JDAW file.\n");
	    invoke_open_jdaw_file = true;
	} else if (
	    strncmp("mid", ext, 3) * strncmp("MID", ext, 3) == 0) {
	    invoke_open_midi_file = true;
	} else {
	unrecognized_arg:
	    fprintf(stderr, "Error: argument \"%s\" not recognized. Pass a .jdaw or .wav file to open that file.\n", argv[1]);
	    exit(1);
	}
    }

    /* Create a window, assign a std_font, and set a main layout */
    main_win = window_create(WINDOW_DEFAULT_W, WINDOW_DEFAULT_H, "Jackdaw");
    window_assign_font(main_win, OPEN_SANS_PATH, FONT_REG);
    window_assign_font(main_win, OPEN_SANS_BOLD_PATH, FONT_BOLD);
    window_assign_font(main_win, LTSUPERIOR_PATH, FONT_MONO);
    window_assign_font(main_win, LTSUPERIOR_BOLD_PATH, FONT_MONO_BOLD);
    window_assign_font(main_win, NOTO_SANS_SYMBOLS2_PATH, FONT_SYMBOLIC);
    window_assign_font(main_win, NOTO_SANS_MATH_PATH, FONT_MATHEMATICAL);
    window_assign_font(main_win, NOTO_MUSIC_PATH, FONT_MUSIC);
    init_symbol_table(main_win);


    
    
    /* Create project here */

    Session *session = session_create();
    if (invoke_open_jdaw_file) {
	fprintf(stderr, "Opening \"%s\"...\n", file_to_open);
	Project new_proj;
	memset(&new_proj, '\0', sizeof(new_proj));
	session->proj_reading = &new_proj;
	int ret = jdaw_read_file(&new_proj, file_to_open);
	/* int ret = jdaw_read_file(&session->proj, file_to_open); */
	if (ret == 0) {
	    session_set_proj(session, &new_proj);
	    session->proj_initialized = true;
	    /* TODO: handle audio format disagreements more elegantly */
	    AudioConn *output = session->audio_io.playback_conn;
	    if (output->open) {
		audioconn_close(output);
		audioconn_open(session, output);
	    }
	} else {
	    session->proj_initialized = false;
	    memset(&session->proj, '\0', sizeof(Project));
	}
	session->proj_reading = NULL;
    }
    if (session->proj_initialized) {
	char *realpath_ret;
	if (!(realpath_ret = realpath(file_to_open, NULL))) {
	    perror("Error in realpath");
	} else {
	    char *last_slash_pos = strrchr(realpath_ret, '/');
	    if (last_slash_pos) {
		*last_slash_pos = '\0';
		strncpy(DIRPATH_SAVED_PROJ, realpath_ret, MAX_PATHLEN);
	    }
	    free(realpath_ret);
	}
	for (int i=0; i<session->proj.num_timelines; i++) {
	    timeline_reset_full(session->proj.timelines[i]);
	}
    } else {
	fprintf(stderr, "Creating new project...\n");
	int ret = project_init(
	    &session->proj,
	    "project.jdaw",
	    DEFAULT_PROJ_AUDIO_SETTINGS,
	    true);
	session->proj_initialized = true;
	session_init_panels(session);
	if (ret != 0) {
	    fprintf(stderr, "Error: unable to open project \"%s\".\n", file_to_open);
	    exit(1);
	}
    }
    fprintf(stderr, "\t...done\n");


    if (invoke_open_wav_file) {
	Track *track = timeline_add_track(session->proj.timelines[0]);
	wav_load_to_track(track, file_to_open, 0);
	char *filepath = realpath(file_to_open, NULL);
	if (!filepath) {
	    perror("Error in realpath");
	} else {
	    char *last_slash_pos = strrchr(filepath, '/');
	    if (last_slash_pos) {
		*last_slash_pos = '\0';
		strncpy(DIRPATH_OPEN_FILE, filepath, MAX_PATHLEN);
	    } else {
		fprintf(stderr, "Error: no slash in real path of opened file\n");
	    }
	    free(filepath);
	}
    } else if (invoke_open_midi_file) {
	midi_file_open(file_to_open, true);
	char *filepath = realpath(file_to_open, NULL);
	if (!filepath) {
	    perror("Error in realpath");
	} else {
	    char *last_slash_pos = strrchr(filepath, '/');
	    if (last_slash_pos) {
		*last_slash_pos = '\0';
		strncpy(DIRPATH_OPEN_FILE, filepath, MAX_PATHLEN);
	    } else {
		fprintf(stderr, "Error: no slash in real path of opened file\n");
	    }
	    free(filepath);
	}
    }

    loop_project_main();
    
    quit();
    fprintf(stderr, "See ya later!\n");
    
}
