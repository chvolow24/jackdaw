#include <stdio.h>
#include "SDL.h"
#include "window.h"

#define DEFAULT_WINDOW_FLAGS SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE
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
    SDL_SetRenderDrawBlendMode(window->rend, SDL_BLENDMODE_BLEND);

    return window;
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