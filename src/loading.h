/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

/*****************************************************************************************************************
    loading.h

    * loading screens for long-running operations
    * optional progress bar
    * session_loading_screen_update() polls for available SDL_Events,
      preventing the program from appearing unresponsive
    * BUT session_loading_screen_update() MUST BE USED JUDICIOUSLY lest
      it significantly slow the already-long-running operations under its auspices
 *****************************************************************************************************************/

#ifndef JDAW_LOADING_H
#define JDAW_LOADING_H

#include "textbox.h"

#define MAX_LOADSTR_LEN 255

typedef struct loading_screen {
    Layout *layout;
    bool loading;
    char title[MAX_LOADSTR_LEN];
    char subtitle[MAX_LOADSTR_LEN];
    Textbox *title_tb;
    Textbox *subtitle_tb;
    float progress; /* Range 0-1 */
    bool draw_progress_bar;
    SDL_Rect *progress_bar_rect;
} LoadingScreen;


void session_set_loading_screen(
    const char *title,
    const char *subtitle,
    bool draw_progress_bar);

int session_loading_screen_update(
    const char *subtitle,
    float progress);

void session_loading_screen_deinit();

#endif


