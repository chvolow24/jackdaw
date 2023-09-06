/**************************************************************************************************
 * Jackdaw | a stripped-down, keyboard-focused Digital Audio Workstation
 **************************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>
#include "SDL2/SDL.h"
#include "SDL2/SDL_ttf.h"
#include "audio.h"
#include "theme.h"
#include "text.h"

#define abs_min(a,b) (abs(a) < abs(b) ? a : b)
#define lesser_of(a,b) (a < b ? a : b)
#define greater_of(a,b) (a > b ? a : b)
#define get_color(color) (dark_mode ? color.dark : color.light)

bool dark_mode = false;
int play_direction = 0;
bool record = 0;

void init_graphics(SDL_Window **main_win, SDL_Renderer **rend)
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "\nError initializing SDL: %s\n", SDL_GetError());
        exit(1);
    }

    if (TTF_Init() != 0) {
        fprintf(stderr, "\nError initializing SDL_ttf: %s", SDL_GetError());
        exit(1);
    }

    *main_win = SDL_CreateWindow(
        "Jackdaw", 
        SDL_WINDOWPOS_CENTERED, 
        SDL_WINDOWPOS_CENTERED - 20, 
        900, 
        650, 
        SDL_WINDOW_RESIZABLE
    );

    if (!(*main_win)) {
        fprintf(stderr, "Error creating window: %s", SDL_GetError());
        exit(1);
    }

    printf("Window initialized successfully.\n");
    Uint32 render_flags = SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC;
    *rend = SDL_CreateRenderer(*main_win, -1, render_flags);
    if (!(*rend)) {
        fprintf(stderr, "Error creating renderer: %s\n", SDL_GetError());
        exit(1);
    }

}

void set_color(SDL_Renderer **rend, JDAW_Color* color_class) 
{
    SDL_Color color;
    if (dark_mode) {
        color = color_class->dark;
    } else {
        color = color_class->light;
    }
    SDL_SetRenderDrawColor(*rend, color.r, color.g, color.b, color.a);
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

int main()
{
    SDL_Window *main_win = NULL;
    int main_win_w, main_win_h;
    SDL_Renderer *rend = NULL;
    init_graphics(&main_win, &rend);
    if (main_win == NULL || rend == NULL) {
        fprintf(stderr, "\nError: unknown.");
    }
    SDL_GetWindowSize(main_win, &main_win_w, &main_win_h);
    // init_audio();
    init_SDL_ttf();
    TTF_Font *open_sans_12 = open_font(OPEN_SANS, 12);
    TTF_Font *open_sans_14 = open_font(OPEN_SANS, 14);
    TTF_Font *open_sans_16 = open_font(OPEN_SANS, 16);

    /* TEMP */
    int wavebox_width = 500;
    SDL_Rect wavebox = {200, 200, 500, 100};
    SDL_Point play_head_start = {200, 200};
    SDL_Point play_head_end = {200, 300};
    /* -TEMP */

    SDL_Color txt_soft_c = get_color(txt_soft);
    SDL_Color txt_main_c = get_color(txt_main);


    bool quit = false;
    while (!quit) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                quit = true;
            } else if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                SDL_GetWindowSize(main_win, &main_win_w, &main_win_h);
            } else if (e.type == SDL_KEYUP) {
                switch (e.key.keysym.scancode) {        
                    case SDL_SCANCODE_R:
                        if (!record) {
                            printf("Record!\n");
                            record = true;
                            play_direction = 1;
                        } else {
                            record = false;
                            play_direction = 0;
                        }
                        break;
                    case SDL_SCANCODE_L:
                        printf("Play!\n");
                        if (record) {
                            record = false;
                        }
                        if (play_direction <= 0) {
                            play_direction = 1;
                        } else if (play_direction < 32) {
                            play_direction *= 2;
                        }
                        break;
                    case SDL_SCANCODE_K:
                        printf("Pause\n");
                        if (record) {
                            record = false;
                        }
                        play_direction = 0;
                        break;
                    case SDL_SCANCODE_J:
                        printf("Rewind!\n");
                        if (record) {
                            record = false;
                        }
                        if (play_direction >= 0) {
                            play_direction = -1;
                        } else if (play_direction > -32) {
                            play_direction *= 2;
                        }
                        break;
                    case SDL_SCANCODE_I:
                        mark_in(play_head_start);
                        break;
                    case SDL_SCANCODE_O:
                        mark_out(play_head_start);
                        break;
                    default:
                        break;
                }
            }
        }

        set_color(&rend, &bckgrnd_color);
        SDL_RenderClear(rend);
        const char *title_text = "Jackdaw | by Charlie Volow";
        int title_w = 0;
        TTF_SizeText(open_sans_12, title_text, &title_w, NULL);
        SDL_Rect title = (SDL_Rect) {main_win_w / 2 - title_w / 2, main_win_h - 20, 500, 100};
        write_text(rend, &title, open_sans_12, &txt_soft_c, "Jackdaw | by Charlie Volow", true);
        write_text(rend, &mark_I, open_sans_16, &txt_main_c, "I", true);
        write_text(rend, &mark_O, open_sans_16, &txt_main_c, "O", true);
        SDL_SetRenderDrawColor(rend, 255, 150, 255, 255);
        SDL_RenderDrawRect(rend, &wavebox);

        SDL_Point play_head[2];
        play_head[0] = play_head_start;
        play_head[1] = play_head_end;
        if (record) {
            SDL_SetRenderDrawColor(rend, 255, 0, 0, 255);
        }
        SDL_RenderDrawLines(rend, play_head, 2);
        SDL_Rect r3 = (SDL_Rect) {200, 50, 200, 30};
        write_text(rend, &r3, open_sans_16, &txt_main_c, "Lorem ipsum, bullshit tell me I am not a piece of crap, please.", true);
        SDL_RenderPresent(rend);
        if (
            (play_head_start.x <= wavebox.x + wavebox.w && play_direction > 0) ||
            (play_head_start.x >= wavebox.x && play_direction < 0)
        ) {
            int new_pos = play_direction > 0 ? 
                lesser_of(wavebox.w + wavebox.x, play_head_start.x + play_direction) : 
                greater_of(wavebox.x, play_head_start.x + play_direction);
            play_head_start.x = new_pos;
            play_head_end.x = new_pos;
        }

        SDL_Delay(10);
    }

    SDL_DestroyWindow(main_win);
    SDL_Quit();
}