/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2026 Charlie Volow
  
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
#include "clipref.h"
#include "consts.h"
#include "dsp_utils.h"
#include "function_lookup.h"
#include "init_panels.h"
#include "input.h"
#include "io.h"
#include "log.h"
#include "midi_io.h"
#include "midi_qwerty.h"
#include "project.h"
#include "pure_data.h"
#include "session.h"
#include "symbol.h"
#include "tempo.h"
#include "text.h"
#include "transport.h"
#include "window.h"

#include "jdaw_ffmpeg.h"

#define LT_DEV_MODE 0

#define WINDOW_DEFAULT_W 1093
#define WINDOW_DEFAULT_H 650

#define DEFAULT_PROJ_AUDIO_SETTINGS 2, DEFAULT_SAMPLE_RATE, DEFAULT_SAMPLE_FORMAT, DEFAULT_AUDIO_CHUNK_LEN_SFRAMES, DEFAULT_FOURIER_LEN_SFRAMES

#ifndef JACKDAW_VERSION
#define JACKDAW_VERSION "unknown"
#endif

const char *jackdaw_version = JACKDAW_VERSION;
/* char DIRPATH_SAVED_PROJ[MAX_PATHLEN]; */
/* char DIRPATH_OPEN_FILE[MAX_PATHLEN]; */
/* char DIRPATH_EXPORT[MAX_PATHLEN]; */

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

static void input_init()
{
    input_init_hash_table();
    input_init_mode_load_all();
    input_load_keybinding_config(DEFAULT_KEYBIND_CFG_PATH);

}

static void init()
{
    set_thread_id(JDAW_THREAD_MAIN);
    log_init();
    init_SDL();
    get_native_byte_order();
    input_init();
    mqwert_init();
    pd_jackdaw_shm_init();
    midi_io_init();
    init_dsp();
}

static void quit()
{
    Session *session = session_get();
    if (main_win->txt_editing) txt_stop_editing(main_win->txt_editing);
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
    log_quit();
}

void loop_project_main();

extern bool connection_open;


static const char *license_text = "Copyright (C) 2023-2026 Charlie Volow\nThis program is free software: you can redistribute it and/or modify \nit under the terms of the GNU General Public License as published by \nthe Free Software Foundation, either version 3 of the License, or \n(at your option) any later version. \n\nThis program is distributed in the hope that it will be useful,\nbut WITHOUT ANY WARRANTY; without even the implied warranty of\nMERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\nGNU General Public License for more details.";
static const char *usagestr = "\nusage: jackdaw [ <file> | <directory> | log | --version | --help | --usage ]\n";
static const char *helpstr = "\
\nUsage: \
\n\tjackdaw [<file>|<directory>] \
\n\tjackdaw log \
\n\tjackdaw --usage \
\n\tjackdaw --version \
\n\tjackdaw --help \
\n\n<file>:\
\n\t- an audio file (.wav, .mp3, etc.)\
\n\t- a project file (.jdaw)\
\n\t- a synth preset file (.jsynth)\
\n\t- a midi file (.mid, .midi)\
\n\n<directory>:\
\n\tAll audio files in <directory> ('stems') will be opened\n\tin a new project after user confirmation. \
\n\nlog:\
\n\tLogs from the last session will be printed, if available.\
\n\n";

#ifndef HAVE_SYSTEM_SDL2
#define HAVE_SYSTEM_SDL2 "NONE"
#endif

int main(int argc, char **argv)
{
    /* float buf[] = {-3.5, 1.2, -2.3, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0, 2.0}; */
    /* srand(time(NULL)); */
    /* const int print_samples = 32; */
    /* enum ProjectAudioBitDepth bd = PROJ_AUDIO_32; */
    /* int BUFLEN = 96000 * 60 * 2; */
    /* float *buf = malloc(BUFLEN * sizeof(float)); */
    /* fprintf(stderr, "INPUT: \n"); */
    /* for (int i=0; i<BUFLEN; i++) { */
    /*     buf[i] = (float)(rand() % 100) / 100 - 0.5; */
    /*     if (i < print_samples) { */
    /*         fprintf(stderr, "%f, ", buf[i]); */
    /*     } */
    /* } */
    /* fprintf(stderr, "\n"); */
    /* uint8_t *encoded_data = NULL; */
    /* size_t encoded_size = 0; */
    /* encode_flac(buf, BUFLEN, bd, &encoded_data, &encoded_size); */
    /* int32_t samples_recd = 0; */
    /* float *buf2 = malloc(BUFLEN * sizeof(float)); */
    /* decode_flac(encoded_data, encoded_size, buf2, &samples_recd, bd); */
    /* fprintf(stderr, "OUTPUT: \n"); */
    /* for (int i=0; i<print_samples; i++) { */
    /*     fprintf(stderr, "%f, ", buf2[i]); */
    /* } */
    /* fprintf(stderr, "\n"); */
    /* double cum_error = 0.0; */
    /* for (int i=0; i<BUFLEN; i++) { */
    /*     cum_error += fabs(buf2[i] - buf[i]); */
    /* } */
    /* fprintf(stderr, "CUM ERROR: %f\n", cum_error); */
    /* fprintf(stderr, "Space v. 16 PCM: %f\n", (double)encoded_size / (double)(BUFLEN * 2)); */
    /* fprintf(stderr, "Space v. 32 PCM: %f\n", (double)encoded_size / (double)(BUFLEN * 4)); */
    
    /* return 0; */
    const char *command_line_arg = NULL;
    if (argc > 2) {
	fprintf(stderr, "%s\n", usagestr);
        exit(1);
    } else if (argc == 2) {
	if (strncmp(argv[1], "--", 2) == 0) {
	    char *option = argv[1] + 2;
	    if (strncmp(option, "version", 7) == 0) {
		fprintf(stderr, "%s\n", JACKDAW_VERSION);
		return 0;
	    } else if (strncmp(option, "usage", 5) == 0) {
		fprintf(stderr, "%s\n", usagestr);
		return 0;
	    } else if (strncmp(option, "help", 4) == 0) {
		fprintf(stderr, "%s\n", helpstr);
		return 0;
	    } else {
		fprintf(stderr, "%s\nUnknown option: \"%s\"\n", usagestr, argv[1]);
		return 1;
	    }       
	} else if (strncmp(argv[1], "fn_ref", 6) == 0) {
	    log_disable();
	    input_init();
	    input_create_function_reference();
	    input_quit();
	    exit(0);
	} else if (strncmp(argv[1], "log", 3) == 0) {
	    log_print_last_session();
	    return 0;
	} else {
	    command_line_arg = argv[1];
	}
    }
    char rp[PATH_MAX] = {0};
    IOFileType in_file_type = IO_FILE_TYPE_UNDETERMINED;
    if (command_line_arg) {
	in_file_type = io_file_type_from_path(command_line_arg, rp);
	if (!IO_FILE_TYPE_OK(in_file_type)) {
	    fprintf(stderr, "Unable to open \"%s\": %s\n", command_line_arg, io_file_get_error(in_file_type));
	    return 1;
	}
    }

    fprintf(stdout, "\n\nJACKDAW (version %s)\nby Charlie Volow\n\nhttps://jackdaw-audio.net/\n\n%s\n\n", jackdaw_version, license_text);

    init();

    /* Create a window, assign a std_font, and set a main layout */
    main_win = window_create(WINDOW_DEFAULT_W, WINDOW_DEFAULT_H, "Jackdaw");
    window_assign_fonts(main_win);
    init_symbol_table(main_win);


    /* Create session before doing anything project-related */
    Session *session = session_create();

    /* Check for opening JDAW file */

    if (in_file_type == IO_FILE_PROJ) {
	if (open_jdaw_file_starttime(rp) < 0) {
	    fprintf(stderr, "Error opening file %s. Exiting.\n", rp);
	    return 1;
	}
	command_line_arg = NULL;
    }	    

    if (!session->proj_initialized) {
	project_init(
	    &session->proj,
	    "project.jdaw",
	    DEFAULT_PROJ_AUDIO_SETTINGS,
	    true);
	session->proj_initialized = true;
	session_init_panels(session);
	timeline_add_track(session->proj.timelines[0], -1);
        session_set_proj_save_point();
    }

    window_push_mode(main_win, MODE_TIMELINE);
    if (command_line_arg) {
	io_open_file(rp, in_file_type, session->proj.timelines[0]->tracks[0], 0);
    }
    session_check_reset_window_title();
    session->init_complete = true;
    loop_project_main();
    quit();    
}
