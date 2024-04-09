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

#include <stdbool.h>
#include <stdio.h>
#include "SDL.h"
#include "SDL_ttf.h"
#include "input.h"
#include "layout.h"
#include "layout_xml.h"
#include "project.h"
#include "window.h"

#define JACKDAW_VERSION "0.2.0"

#ifndef INSTALL_DIR
#define INSTALL_DIR ""
#endif

#define WINDOW_DEFAULT_W 1200
#define WINDOW_DEFAULT_H 800
#define OPEN_SANS_PATH INSTALL_DIR "/assets/ttf/OpenSans-Regular.ttf"
#define MAIN_LT_PATH INSTALL_DIR "/gui/jackdaw_main_layout.xml"
#define DEFAULT_KEYBIND_CFG_PATH INSTALL_DIR "/assets/key_bindings/default.yaml"

bool sys_byteorder_le = false;

Window *main_win;
Project *proj;

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
}

static void quit()
{
    if (main_win) {
	window_destroy(main_win);
    }
    SDL_Quit();
}

void loop_project_main();

int main(int argc, char **argv)
{
    fprintf(stdout, "\n\nJACKDAW (version %s)\nby Charlie Volow\n\n", JACKDAW_VERSION);
    
    init();

    char *file_to_open = NULL;
    bool invoke_open_wav_file = false;
    bool invoke_open_jdaw_file = false;
    if (argc > 2) {
        exit(1);
    } else if (argc == 2) {
        file_to_open = argv[1];
        int dotpos = strcspn(file_to_open, ".");
        char *ext = file_to_open + dotpos + 1;
        if (strcmp("wav", ext) * strcmp("WAV", ext) == 0) {
            fprintf(stderr, "Passed WAV file.\n");
            invoke_open_wav_file = true;
        } else if (strcmp("jdaw", ext) * strcmp("JDAW", ext) == 0) {
            fprintf(stderr, "Passed JDAW file.\n");
            invoke_open_jdaw_file = true;
        } else {
	    fprintf(stderr, "Error: argument \"%s\" not recognized. Pass a .jdaw or .wav file to open that file.\n", argv[1]);
	}
    }

    /* Create a window, assign a std_font, and set a main layout */
    main_win = window_create(WINDOW_DEFAULT_W, WINDOW_DEFAULT_H, "Jackdaw");
    window_assign_std_font(main_win, OPEN_SANS_PATH);
    window_set_layout(main_win, layout_create_from_window(main_win));

    layout_read_xml_to_lt(main_win->layout, MAIN_LT_PATH);
    
    loop_project_main();
    
    quit();
    
}
