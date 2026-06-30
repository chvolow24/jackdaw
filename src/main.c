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
#include "audio_clip.h"
#include "clipref.h"
#include "consts.h"
#include "dir.h"
#include "dot_jdaw.h"
#include "dsp_utils.h"
#include "function_lookup.h"
#include "init_panels.h"
#include "input.h"
#include "io.h"
#include "log.h"
#include "midi_io.h"
#include "midi_file.h"
#include "midi_qwerty.h"
#include "project.h"
#include "pure_data.h"
#include "session.h"
#include "symbol.h"
#include "synth.h"
#include "tempo.h"
#include "text.h"
#include "transport.h"
#include "userfn.h"
#include "wav.h"
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

static void input_init()
{
    input_init_hash_table();
    input_init_mode_load_all();
    input_load_keybinding_config(DEFAULT_KEYBIND_CFG_PATH);

}

static void init()
{
    /* FILE *test = open_asset("layouts/jackdaw_main_layout.xml", "r"); */
    /* exit(0); */
    set_thread_id(JDAW_THREAD_MAIN);
    /* MAIN_THREAD_ID = pthread_self(); */
    /* CURRENT_THREAD_ID = MAIN_THREAD_ID; */
    /* error_register_signal_handlers(); */
    log_init();
    init_SDL();
    get_native_byte_order();
    input_init();
    mqwert_init();
    pd_jackdaw_shm_init();
    char *realpath_ret;
    if (!(realpath_ret = realpath(".", NULL))) {
	perror("Error in realpath");
    } else {
	snprintf(DIRPATH_SAVED_PROJ, MAX_PATHLEN, "%s", realpath_ret); 
	/* strlcpy(DIRPATH_SAVED_PROJ, realpath_ret, MAX_PATHLEN); */
	free(realpath_ret);
    }
    strcpy(DIRPATH_OPEN_FILE, DIRPATH_SAVED_PROJ);
    strcpy(DIRPATH_EXPORT, DIRPATH_SAVED_PROJ);
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
    /* fprintf(stderr, "%s\n", usagestr); */
    /* exit(0); */
    /* char *file_to_open = NULL; */
    /* bool invoke_open_wav_file = false; */
    /* bool invoke_open_jdaw_file = false; */
    /* bool invoke_open_midi_file = false; */
    /* bool invoke_open_jsynth_file = false; */
    /* bool invoke_open_audio_file = false; */
    /* char **stems_paths = NULL; */
    /* int num_stems = 0; */
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
	/* file_to_open = argv[1]; */
	/* const char *audio_file_extensions[] = {AUDIO_FILE_EXTENSIONS}; */
	/* int num_extensions = sizeof(audio_file_extensions) / sizeof(float *); */
	/* char *dotpos = strrchr(file_to_open, '.'); */
	/* if (!dotpos) goto unrecognized_arg; */
	/* char *ext = dotpos + 1; */
	/* if (strncmp("wav", ext, 3) * strcmp("WAV", ext) == 0) { */
	/*     fprintf(stderr, "Passed WAV file.\n"); */
	/*     invoke_open_wav_file = true; */
	/* } else if (strncmp("jdaw", ext, 4) * strcmp("JDAW", ext) * strcmp("bak", ext)  == 0) { */
	/*     fprintf(stderr, "Passed JDAW file.\n"); */
	/*     invoke_open_jdaw_file = true; */
	/* } else if ( */
	/*     strncmp("mid", ext, 3) * strncmp("MID", ext, 3) == 0) { */
	/*     invoke_open_midi_file = true; */
	/* } else if ( */
	/*     strncmp("jsynth", ext, 6) * strncmp("JSYNTH", ext, 6) == 0) { */
	/*     invoke_open_jsynth_file = true; */
	/* } else if (file_extension_in_list(file_to_open, audio_file_extensions, num_extensions)) { */
	/*     invoke_open_audio_file = true; */
	/* } else { */
	/* unrecognized_arg: */
	/*     num_stems = load_stems_dir(file_to_open, &stems_paths); */
	/*     if (num_stems <= 0) { */
	/* 	fprintf(stderr, "Error: argument \"%s\" not recognized. Pass a .jdaw, .wav, .mid, or .jsynth file to open that file, or a directory to open stems.\n", argv[1]); */
	/* 	exit(1); */
	/*     } */
	/* } */
    }
    fprintf(stdout, "\n\nJACKDAW (version %s)\nby Charlie Volow\n\nhttps://jackdaw-audio.net/\n\n%s\n\n", jackdaw_version, license_text);

    init();

    /* Create a window, assign a std_font, and set a main layout */
    main_win = window_create(WINDOW_DEFAULT_W, WINDOW_DEFAULT_H, "Jackdaw");
    window_assign_fonts(main_win);
    init_symbol_table(main_win);


    /* Create session before doing anything project-related */
    Session *session = session_create();

    
    /* int openfile_ret = open_file(command_line_arg, NULL, NULL); */
    
    /* if (invoke_open_jdaw_file) { */
    /* 	fprintf(stderr, "Opening \"%s\"...\n", file_to_open); */
    /* 	Project new_proj; */
    /* 	memset(&new_proj, '\0', sizeof(new_proj)); */
    /* 	session->proj_reading = &new_proj; */
    /* 	int ret = jdaw_read_file(&new_proj, file_to_open); */
    /* 	/\* int ret = jdaw_read_file(&session->proj, file_to_open); *\/ */
    /* 	if (ret == 0) { */
    /* 	    session_set_proj(session, &new_proj); */
    /* 	    session->proj_initialized = true; */
    /* 	    /\* TODO: handle audio format disagreements more elegantly *\/ */
    /* 	    AudioConn *output = session->audio_io.playback_conn; */
    /* 	    if (output->open) { */
    /* 		audioconn_close(output); */
    /* 		audioconn_open(session, output); */
    /* 	    } */
    /* 	} else { */
    /* 	    fprintf(stderr, "Unable to open project file at \"%s\": %s\n", file_to_open, ret == -2 ? "File does not exist" : "Unable to parse file"); */
    /* 	    return 1; */
    /* 	    /\* session->proj_initialized = false; *\/ */
    /* 	    /\* memset(&session->proj, '\0', sizeof(Project)); *\/ */
    /* 	} */
    /* 	session->proj_reading = NULL; */
    /* } */
    /* if (session->proj_initialized) { */
    /* 	char *realpath_ret; */
    /* 	if (!(realpath_ret = realpath(file_to_open, NULL))) { */
    /* 	    perror("Error in realpath"); */
    /* 	} else { */
    /* 	    char *last_slash_pos = strrchr(realpath_ret, '/'); */
    /* 	    if (last_slash_pos) { */
    /* 		*last_slash_pos = '\0'; */
    /* 		snprintf(DIRPATH_SAVED_PROJ, MAX_PATHLEN, "%s", realpath_ret); */
    /* 		/\* strlcpy(DIRPATH_SAVED_PROJ, realpath_ret, MAX_PATHLEN); *\/ */
    /* 	    } */
    /* 	    free(realpath_ret); */
    /* 	} */
    /* 	for (int i=0; i<session->proj.num_timelines; i++) { */
    /* 	    timeline_reset_full(session->proj.timelines[i]); */
    /* 	} */
    /* } else { */
	fprintf(stderr, "Creating new project...\n");
	int ret = project_init(
	    &session->proj,
	    "project.jdaw",
	    DEFAULT_PROJ_AUDIO_SETTINGS,
	    true);
	session->proj_initialized = true;
	session_init_panels(session);
	timeline_add_track(session->proj.timelines[0], -1);
    /* } */

    window_push_mode(main_win, MODE_TIMELINE);
    if (command_line_arg) {
	open_file(command_line_arg, session->proj.timelines[0]->tracks[0], 0);
    }

    /* if (invoke_open_wav_file) { */
    /* 	/\* Track *track = timeline_add_track(session->proj.timelines[0], -1); *\/ */
    /* 	wav_load_to_track(session->proj.timelines[0]->tracks[0], file_to_open, 0); */
    /* 	char *filepath = realpath(file_to_open, NULL); */
    /* 	if (!filepath) { */
    /* 	    fprintf(stderr, "Could not find file at \"%s\"\n", file_to_open); */
    /* 	    return 1; */
    /* 	} else { */
    /* 	    char *last_slash_pos = strrchr(filepath, '/'); */
    /* 	    if (last_slash_pos) { */
    /* 		*last_slash_pos = '\0'; */
    /* 		snprintf(DIRPATH_OPEN_FILE, MAX_PATHLEN, "%s", filepath); */
    /* 		/\* strlcpy(DIRPATH_OPEN_FILE, filepath, MAX_PATHLEN); *\/ */
    /* 	    } else { */
    /* 		fprintf(stderr, "Error: no slash in real path of opened file\n"); */
    /* 	    } */
    /* 	    free(filepath); */
    /* 	} */
    /* } else if (invoke_open_audio_file) { */
    /* 	float *L, *R; */
    /* 	int32_t length_sframes = av_open_file(file_to_open, &L, &R); */
    /* 	int32_t length_seconds = length_sframes / session_get_sample_rate(); */
    /* 	fprintf(stderr, "DECODED %d:%d of audio (%d)\n", length_seconds / 60, length_seconds - (length_seconds / 60), length_sframes); */
    /* 	if (length_sframes == 0) { */
    /* 	    fprintf(stderr, "Unable to decode file at %s. run './jackdaw log' for details\n", file_to_open); */
    /* 	    exit(1); */
    /* 	} */
    /* 	Track *dst_track = session->proj.timelines[0]->tracks[0]; */
    /* 	Clip *clip = clip_create(NULL, dst_track); */
    /* 	clip->L = L; */
    /* 	clip->R = R; */
    /* 	clip->channels = 2; */
    /* 	clip->len_sframes = length_sframes; */
    /* 	clip_init_or_update_waveform(clip); */
    /* 	ClipRef *cr = clipref_create(dst_track, 0, CLIP_AUDIO, clip); */
    /* 	timeline_reset(cr->track->tl, true); */
    /* } else if (invoke_open_midi_file) { */
    /* 	midi_file_open(file_to_open, true); */
    /* 	char *filepath = realpath(file_to_open, NULL); */
    /* 	if (!filepath) { */
    /* 	    perror("Error in realpath"); */
    /* 	} else { */
    /* 	    char *last_slash_pos = strrchr(filepath, '/'); */
    /* 	    if (last_slash_pos) { */
    /* 		*last_slash_pos = '\0'; */
    /* 		snprintf(DIRPATH_OPEN_FILE, MAX_PATHLEN, "%s", filepath); */
    /* 		/\* strlcpy(DIRPATH_OPEN_FILE, filepath, MAX_PATHLEN); *\/ */
    /* 	    } else { */
    /* 		fprintf(stderr, "Error: no slash in real path of opened file\n"); */
    /* 	    } */
    /* 	    free(filepath); */
    /* 	} */
    /* } else if (invoke_open_jsynth_file) { */
    /* 	char *filepath = realpath(file_to_open, NULL); */
    /* 	if (!filepath) { */
    /* 	    perror("Error in realpath"); */
    /* 	} else { */
    /* 	    Track *track = session->proj.timelines[0]->tracks[0]; */
    /* 	    track->synth = synth_create(track); */
    /* 	    track->midi_out = track->synth; */
    /* 	    synth_read_preset_file(filepath, track->synth); */
    /* 	    log_tmp(LOG_DEBUG, "Opening synth in main.c\n"); */
    /* 	    user_tl_track_open_synth(NULL); */
    /* 	} */
    /* } else if (num_stems > 0) { */
    /* 	Timeline *tl = session->proj.timelines[0]; */
    /* 	for (int i=0; i<num_stems; i++) { */
    /* 	    Track *track; */
    /* 	    if (i != 0) { */
    /* 		track = timeline_add_track(tl, i); */
    /* 	    } else { */
    /* 		track = tl->tracks[0]; */
    /* 	    } */
    /* 	    wav_load_to_track(track, stems_paths[i], 0); */
    /* 	} */
    /* } */
    loop_project_main();
    
    quit();
    fprintf(stderr, "See ya later!\n");
    
}
