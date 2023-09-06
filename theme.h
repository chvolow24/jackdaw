#ifndef JDAW_THEME_H
#define JDAW_THEME_H

#include "SDL2/SDL.h"

typedef struct jdaw_color {
    SDL_Color light;
    SDL_Color dark;
} JDAW_Color;

JDAW_Color bckgrnd_color = {{255, 240, 200, 255}, {22, 28, 34, 255}};
JDAW_Color txt_soft = {{50, 50, 50, 255}, {200, 200, 200, 255}};
JDAW_Color txt_main = {{10, 10, 10, 255}, {240, 240, 240, 255}};

#endif
