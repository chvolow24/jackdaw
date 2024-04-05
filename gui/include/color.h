#ifndef JDAW_GUI_COLOR_H
#define JDAW_GUI_COLOR_H

#include "SDL.h"

#define sdl_color_expand(color) color.r, color.g, color.b, color.a
#define sdl_colorp_expand(colorp) colorp->r, colorp->g, colorp->b, colorp->a

/* TODO: flesh this out */
typedef struct color_theme {
    SDL_Color background;
    SDL_Color track;
    SDL_Color clip;
    SDL_Color clone;
} ColorTheme;

#endif
