/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
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
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "SDL.h"
#include "SDL_ttf.h"
#include "assets.h"
#include "dir.h"
#include "dsp.h"
#include "input.h"
#include "layout.h"
#include "layout_xml.h"
#include "project.h"
#include "pure_data.h"
#include "symbol.h"
#include "tempo.h"
#include "text.h"
#include "transport.h"
#include "wav.h"
#include "waveform.h"
#include "window.h"

#include "audio_connection.h"

#define LT_DEV_MODE 0

/* #define WINDOW_DEFAULT_W 900 */
#define WINDOW_DEFAULT_W 1093
#define WINDOW_DEFAULT_H 650
/* #define OPEN_SANS_PATH INSTALL_DIR "/assets/ttf/OpenSans-Regular.ttf" */


#define DEFAULT_PROJ_AUDIO_SETTINGS 2, 96000, AUDIO_S16SYS, 1024, 2048

const char *JACKDAW_VERSION = "0.4.3";
char DIRPATH_SAVED_PROJ[MAX_PATHLEN];
char DIRPATH_OPEN_FILE[MAX_PATHLEN];
char DIRPATH_EXPORT[MAX_PATHLEN];

bool SYS_BYTEORDER_LE = false;
volatile bool CANCEL_THREADS = false;

Window *main_win;
Project *proj = NULL;
pthread_t MAIN_THREAD_ID;
pthread_t DSP_THREAD_ID;
/* char *saved_proj_path = NULL; */

static void get_native_byte_order()
{
    #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    SYS_BYTEORDER_LE = true;
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
    MAIN_THREAD_ID = pthread_self();
    init_SDL();
    get_native_byte_order();
    input_init_hash_table();
    input_init_mode_load_all();
    input_load_keybinding_config(DEFAULT_KEYBIND_CFG_PATH);
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
    /* fprintf(stdout, "Initializing dsp...\n"); */
    init_dsp();
    api_start_server(5080);
}

static void quit()
{
    CANCEL_THREADS = true;
    pd_signal_termination_of_jackdaw();
    if (proj->recording) {
	transport_stop_recording();
    }
    transport_stop_playback();
    /* Sleep to allow DSP thread to exit */
    SDL_Delay(100);
    project_destroy(proj);
    symbol_quit(main_win);
    if (main_win) {
	window_destroy(main_win);
    }
    input_quit();
    api_quit();
    SDL_Quit();
}

void loop_project_main();
Project *jdaw_read_file(const char *path);

extern bool connection_open;


/* void value_serialize_test() */
/* { */
/*     char dst[17]; */
/*     double test = 1.2345678987654321; */
    
/*     snprintf(dst, 16, "%.16g", test); */
/*     fprintf(stderr, "Dst: %s\n", dst); */

/*     FILE *f = fopen("hello.txt", "w"); */
/*     ValType t = JDAW_FLOAT; */
/*     Value a; */
/*     a.float_v = 3.141593456789098765456789; */
/*     jdaw_val_serialize(a, t, f, 16); */
/*     a.float_v = -3.141593456789098765456789; */
/*     jdaw_val_serialize(a, t, f, 16); */
/*     t = JDAW_BOOL; */
/*     a.bool_v = true; */
/*     jdaw_val_serialize(a, t, f, 16); */
/*     a.double_v = 3.141593456789098765456789; */
/*     t = JDAW_DOUBLE; */
/*     jdaw_val_serialize(a, t, f, 16); */
/*     fclose(f); */
    
    
/*     f = fopen("hello.txt", "r"); */
/*     a = jdaw_val_deserialize(f, 16, JDAW_FLOAT); */
/*     fprintf(stderr, "A: %.16g\n", a.float_v); */
/*     a = jdaw_val_deserialize(f, 16, JDAW_FLOAT); */
/*     fprintf(stderr, "A: %.16g\n", a.float_v); */
/*     a = jdaw_val_deserialize(f, 16, JDAW_BOOL); */
/*     fprintf(stderr, "A: %d\n", a.bool_v); */
/*     a = jdaw_val_deserialize(f, 16, JDAW_DOUBLE); */
/*     fprintf(stderr, "A: %f\n", a.double_v); */
/*     fclose(f); */
/* } */


void tempo_track_get_next_pos(TempoTrack *t, bool start, int32_t start_from, int32_t *pos, enum beat_prominence *bp);
void tempo_track_fprint(FILE *f, TempoTrack *tt);
void tempo_track_draw(TempoTrack *tt);

int main(int argc, char **argv)
{

    /* value_serialize_test(); */
    /* return 1; */
    /* SDL_Rect test = {0, 0, 1000, 1000}; */
    /* struct logscale_array *la = waveform_create_logscale(NULL, 512, &test); */
    /* fprintf(stdout, "Interval : %f\n", la->interval); */
    /* /\* dir_tests(); *\/ */
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
	} else if (strncmp("jdaw", ext, 4) * strcmp("JDAW", ext) == 0) {
	    fprintf(stderr, "Passed JDAW file.\n");
	    invoke_open_jdaw_file = true;
	} else {
	unrecognized_arg:
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
    window_assign_font(main_win, NOTO_SANS_SYMBOLS2_PATH, SYMBOLIC);
    window_assign_font(main_win, NOTO_SANS_MATH_PATH, MATHEMATICAL);
    init_symbol_table(main_win);


    
    
    /* Create project here */

    if (invoke_open_jdaw_file) {
	proj = jdaw_read_file(file_to_open);
	if (proj) {
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
    
}
