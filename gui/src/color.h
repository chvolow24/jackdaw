#ifndef JDAW_GUI_COLOR_H
#define JDAW_GUI_COLOR_H

#include "SDL.h"

#define sdl_color_expand(color) color.r, color.g, color.b, color.a
#define sdl_colorp_expand(colorp) colorp->r, colorp->g, colorp->b, colorp->a

/* /\* TODO: flesh this out *\/ */
/* typedef struct color_theme { */
/*     SDL_Color background; */
/*     SDL_Color track; */
/*     SDL_Color clip; */
/*     SDL_Color clone; */
/* } ColorTheme; */

struct colors {
    SDL_Color white;
    SDL_Color black;
    SDL_Color clear;
    SDL_Color grey;
    SDL_Color light_grey;
    SDL_Color red;
    SDL_Color green;
    SDL_Color blue;
    SDL_Color yellow;
    SDL_Color quickref_button_blue;
    SDL_Color quickref_button_pressed;
    SDL_Color play_green;
    SDL_Color undo_purple;
    SDL_Color cerulean;
    SDL_Color cerulean_pale;
    SDL_Color dark_brown;


    SDL_Color dropdown_green;
    SDL_Color x_red;
    SDL_Color min_yellow;
    SDL_Color click_track;

    SDL_Color freq_L;
    SDL_Color freq_R;

};


#endif
