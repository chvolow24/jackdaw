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
Window *create_window(int w, int h, const char *name);

/* Create a Font object, open TTF Fonts, and assign to window */
void assign_std_font(Window *win, const char *font_path);

/* Reset the values of the w and h members of a Window struct based on current window dimensions */
void auto_resize_window(Window *window);

/* Force resize a window */
void resize_window(Window *win, int w, int h);

#endif

