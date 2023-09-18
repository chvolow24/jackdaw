/**************************************************************************************************
 * Jackdaw | a stripped-down, keyboard-focused Digital Audio Workstation. Built on SDL.
***************************************************************************************************

  Copyright (C) 2023 Charlie Volow

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

**************************************************************************************************/

#ifndef JDAW_TEXT_H
#define JDAW_TEXT_H

#include "SDL.h"
#include "SDL_ttf.h"
#include "theme.h"

#define FREE_SANS "assets/ttf/free_sans.ttf"
#define OPEN_SANS "assets/ttf/OpenSans-Regular.ttf"
#define OPEN_SANS_BOLD "assets/ttf//OpenSans-Bold.ttf"
#define OPEN_SANS_VAR "assets/ttf/OpenSans-Variable.ttf"
#define COURIER_NEW "assets/ttf/CourierNew.ttf"
#define STD_FONT_SIZES {10, 12, 14, 16, 18, 24, 30, 36, 48, 60, 72}
#define STD_FONT_ARRLEN 11

// typedef struct textbox {
//     SDL_Rect *container;
//     char *text;
//     TTF_Font *font;
// } Textbox;

void init_SDL_ttf();
TTF_Font* open_font(const char* path, int size);
void init_fonts(TTF_Font **font_array, const char *path, int arrlen);
void close_font(TTF_Font* font);
int write_text(
    SDL_Renderer *rend, 
    SDL_Rect *rect, 
    TTF_Font* font, 
    JDAW_Color* color, 
    const char *text, 
    bool allow_resize
);

#endif