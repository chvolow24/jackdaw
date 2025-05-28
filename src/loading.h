
#ifndef JDAW_LOADING_H
#define JDAW_LOADING_H

#include "endpoint.h"

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

void session_loading_screen_deinit(Session *session);

#endif


