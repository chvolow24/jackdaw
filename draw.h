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


#ifndef JDAW_DRAW_H
#define JDAW_DRAW_H

#include "SDL.h"
#include "project.h"
#include "gui.h"
#include "theme.h"

void draw_project(Project *proj);
void set_rend_color(SDL_Renderer *rend, JDAW_Color *color);
void draw_textbox_list(SDL_Renderer *rend, TextboxList *tbl);
void draw_quadrant(SDL_Renderer *rend, int xinit, int yinit, int r, const register uint8_t quad);
void draw_rounded_rect(SDL_Renderer *rend, SDL_Rect *rect, int r);
void fill_rounded_rect(SDL_Renderer *rend, SDL_Rect *rect, int r);
void draw_menu_list(SDL_Renderer *rend, TextboxList* tbl);

#endif