/**************************************************************************************************
 * Jackdaw | a stripped-down, keyboard-focused Digital Audio Workstation
 **************************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>
#include <pthread.h>
#include "SDL2/SDL.h"
#include "SDL2/SDL_ttf.h"
#include "audio.h"
#include "theme.h"
#include "text.h"
#include "project.h"
#include "gui.h"

#define abs_min(a,b) (abs(a) < abs(b) ? a : b)
#define lesser_of(a,b) (a < b ? a : b)
#define greater_of(a,b) (a > b ? a : b)

bool dark_mode = true;

/* Temporary */
bool record = 0;
/* -Temporary */

Project *proj;

TTF_Font *open_sans_12;
TTF_Font *open_sans_14;
TTF_Font *open_sans_16;
TTF_Font *open_sans_var_12;
TTF_Font *open_sans_var_14;
TTF_Font *open_sans_var_16;
TTF_Font *courier_new_12;

extern JDAW_Color bckgrnd_color;
extern JDAW_Color tl_bckgrnd;
extern JDAW_Color txt_soft;
extern JDAW_Color txt_main;

void init_graphics()
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "\nError initializing SDL: %s\n", SDL_GetError());
        exit(1);
    }

    if (TTF_Init() != 0) {
        fprintf(stderr, "\nError initializing SDL_ttf: %s", SDL_GetError());
        exit(1);
    }
}

void init_fonts()
{
    open_sans_12 = open_font(OPEN_SANS, 12);
    open_sans_14 = open_font(OPEN_SANS, 14);
    open_sans_16 = open_font(OPEN_SANS, 16);
    open_sans_var_12 = open_font(OPEN_SANS_VAR, 12);
    open_sans_var_14 = open_font(OPEN_SANS_VAR, 14);
    open_sans_var_16 = open_font(OPEN_SANS_VAR, 16);
    courier_new_12 = open_font(COURIER_NEW, 12);
}

SDL_Rect mark_I;
SDL_Rect mark_O;
void mark_in(SDL_Point play_head_start)
{
    mark_I = (SDL_Rect) {play_head_start.x, play_head_start.y - 20, 50, 50};
}

void mark_out(SDL_Point play_head_start)
{
    mark_O = (SDL_Rect) {play_head_start.x, play_head_start.y - 20, 50, 50};

}


void playback()
{
    if (proj->play_speed == 1 && !record) {
        start_playback();
    } else if (proj->play_speed != 0 && !record) {
        proj->tl->play_position += proj->play_speed * 441;
    }

}

int main()
{

    proj = create_project("Untitled", dark_mode);
    init_graphics();
    init_fonts();

    // SDL_GetWindowSize(main_win, &main_win_w, &main_win_h);
    
    init_audio();
    init_SDL_ttf();
    init_fonts();

    // txt_soft_c = get_color(txt_soft);
    // txt_main_c = get_color(txt_main);

    /* TEMP */
    int wavebox_width = 500;
    SDL_Rect wavebox = {200, 200, 500, 100};
    SDL_Point play_head_start = {200, 200};
    SDL_Point play_head_end = {200, 300};
    /* -TEMP */
    // Clip *active_clip;
    int active_track = 0;
    bool quit = false;
    while (!quit) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                quit = true;
            } else if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                // SDL_GetWindowSize(main_win, &main_win_w, &main_win_h);
            } else if (e.type == SDL_KEYUP) {
                switch (e.key.keysym.scancode) {        
                    case SDL_SCANCODE_R:
                        if (!record) {
                            printf("Record!\n");
                            record = true;
                            proj->active_clip = create_clip((proj->tl->tracks)[active_track], 0, proj->tl->play_position);
                            start_recording();
                            proj->play_speed = 1;
                        } else {
                            record = false;
                            stop_recording(proj->active_clip);
                            proj->play_speed = 0;
                        }
                        break;
                    case SDL_SCANCODE_L:
                        printf("Play!\n");
                        if (record) {
                            record = false;
                        }
                        if (proj->play_speed <= 0) {
                            proj->play_speed = 1;
                        } else if (proj->play_speed < 32) {
                            proj->play_speed *= 2;
                        }
                        break;
                    case SDL_SCANCODE_K:
                        printf("Pause\n");
                        if (record) {
                            record = false;
                        }
                        proj->play_speed = 0;
                        stop_playback();
                        break;
                    case SDL_SCANCODE_J:
                        printf("Rewind!\n");
                        if (record) {
                            record = false;
                        }
                        if (proj->play_speed >= 0) {
                            proj->play_speed = -1;
                        } else if (proj->play_speed > -32) {
                            proj->play_speed *= 2;
                        }
                        break;
                    case SDL_SCANCODE_I:
                        mark_in(play_head_start);
                        break;
                    case SDL_SCANCODE_O:
                        mark_out(play_head_start);
                        break;
                    case SDL_SCANCODE_T:
                        create_track(proj->tl, false);
                        break;
                    case SDL_SCANCODE_1:
                        active_track = 0;
                        break;
                    case SDL_SCANCODE_2:
                        active_track = 1;
                        break;
                    case SDL_SCANCODE_3:
                        active_track = 2;
                        break;
                    case SDL_SCANCODE_4:
                        active_track = 3;
                        break;
                    case SDL_SCANCODE_5:
                        active_track = 4;
                        break;
                    case SDL_SCANCODE_6:
                        active_track = 5;
                        break;
                    case SDL_SCANCODE_D:
                        destroy_track((proj->tl->tracks)[proj->tl->num_tracks - 1]);
                        break;
                    default:
                        break;
                }
            }
        }
        playback(proj->play_speed);
        // proj->tl->play_position += proj->play_speed * 441;

        draw_project(proj);
        SDL_Delay(1);
    }

    SDL_Quit();
}