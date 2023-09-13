#ifndef JDAW_THEME_H
#define JDAW_THEME_H

#include "SDL.h"
// #define get_color(color) (dark_mode ? color.dark : color.light)

typedef struct jdaw_color {
    SDL_Color light;
    SDL_Color dark;
} JDAW_Color;

SDL_Color get_color(JDAW_Color *color);

#endif