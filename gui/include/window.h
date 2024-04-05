#ifndef JDAW_GUI_WINDOW_H
#define JDAW_GUI_WINDOW_H

#include "SDL.h"
#include "text.h"


typedef struct window {
    SDL_Window *win;
    SDL_Renderer *rend;
    SDL_Texture *canvas;
    double dpi_scale_factor;
    int w;
    int h;
    SDL_Point mousep;
    SDL_Point mousep_screen;
    double zoom_scale_factor;
    SDL_Rect canvas_src;
    Font *std_font;
    Layout *layout;
} Window;

/* Create a new Window struct and initialize all members */
Window *window_create(int w, int h, const char *name);

/* Destroy a Window, its renderer and canvas, its std_font, and its main layout */
void window_destroy(Window *win);

/* Create a Font object, open TTF Fonts, and assign to window */
void window_assign_std_font(Window *win, const char *font_path);

/* Reset the values of the w and h members of a Window struct based on current window dimensions */
void window_auto_resize(Window *window);

/* Force resize a window */
void window_resize(Window *win, int w, int h);

/* Set the mouse position, accounting for scaling of canvas. */
void window_set_mouse_point(Window *win, int logical_x, int logical_y);

/* DO NOT USE (TODO: fix this) */
void window_zoom(Window *win, float zoom_by);

/* Start a frame draw operation, initializing window with bckgrnd_color if given */
void window_start_draw(Window *win, SDL_Color *bckgrnd_color);

/* End a frame draw operation */
void window_end_draw(Window *win);

/* Set a window's main layout */
void window_set_layout(Window *win, Layout *layout);

/* Window *window_init(int w, int h, const char *name, const char *font_path); */


#endif

