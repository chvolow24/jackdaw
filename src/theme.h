/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

#ifdef UNDEF_DEPRECATED

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

#endif
