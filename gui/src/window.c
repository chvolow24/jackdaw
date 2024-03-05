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

#include <stdio.h>
#include "SDL.h"
#include "window.h"

#define DEFAULT_WINDOW_FLAGS SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI
#define DEFAULT_RENDER_FLAGS SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC

/* Create a window struct and initialize all members */
Window *create_window(int w, int h, const char *name) 
{

    Window *window = malloc(sizeof(Window));
    SDL_Window *win = SDL_CreateWindow(name, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, w, h, (Uint32)DEFAULT_WINDOW_FLAGS);
    if (!win) {
        fprintf(stderr, "Error creating window: %s", SDL_GetError());
        exit(1);
    }

    SDL_Renderer *rend = SDL_CreateRenderer(win, -1, (Uint32)DEFAULT_RENDER_FLAGS);
    if (!rend) {
        fprintf(stderr, "Error creating renderer: %s", SDL_GetError());
    }

    window->dpi_scale_factor = 0;
    int rw = 0, rh = 0, ww = 0, wh = 0;
    SDL_GetWindowSize(win, &ww, &wh);
    SDL_GetRendererOutputSize(rend, &rw, &rh);

    window->dpi_scale_factor = (double) rw / ww;
    
    if (window->dpi_scale_factor == 0 || window->dpi_scale_factor != (double) rh / wh) {
        fprintf(stderr, "Error setting scale factor: %s", SDL_GetError());
        exit(1);
    }
    window->win = win;
    window->rend = rend;
    window->w = w * window->dpi_scale_factor;
    window->h = h * window->dpi_scale_factor;

    window->std_font = NULL;
    
    SDL_SetRenderDrawBlendMode(window->rend, SDL_BLENDMODE_BLEND);

    return window;
}

void assign_std_font(Window *win, const char *font_path)
{
    win->std_font = init_font(font_path, win);
    if (win->std_font == NULL) {
	fprintf(stderr, "Error: Unable to initialize font at %s.\n", font_path);
    }
}

/* Reset the w and h members of a window struct (usually in response to a resize event) */
void auto_resize_window(Window *win)
{
    SDL_GetWindowSize(win->win, &(win->w), &(win->h));
    win->w *= win->dpi_scale_factor;
    win->h *= win->dpi_scale_factor;
}


void resize_window(Window *win, int w, int h)
{
    SDL_SetWindowSize(win->win, (double)w / win->dpi_scale_factor, (double)h / win->dpi_scale_factor);
    win->w = w;
    win->h = h;
}
