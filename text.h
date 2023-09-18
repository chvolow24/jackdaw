/*****************************************************************************************************************
  Jackdaw | a stripped-down, keyboard-focused Digital Audio Workstation | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023 Charlie Volow
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:
  
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
  
  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.

*****************************************************************************************************************/

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