#ifndef JDAW_GUI_WINDOW_H
#define JDAW_GUI_WINDOW_H

#include "SDL.h"
#include "text.h"


typedef struct window {
    SDL_Window *win;
    SDL_Renderer *rend;
    double dpi_scale_factor;
    int w;
    int h;
    Font *std_font;
} Window;

/* Create a new Window struct and initialize all members */
Window *window_create(int w, int h, const char *name);

/* Create a Font object, open TTF Fonts, and assign to window */
void window_assign_std_font(Window *win, const char *font_path);

/* Reset the values of the w and h members of a Window struct based on current window dimensions */
void window_auto_resize(Window *window);

/* Force resize a window */
void window_resize(Window *win, int w, int h);

#endif

