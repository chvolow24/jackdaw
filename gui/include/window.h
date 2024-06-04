#ifndef JDAW_GUI_WINDOW_H
#define JDAW_GUI_WINDOW_H

#define MAX_WINDOW_MENUS 8

#include "SDL.h"
#include "input_mode.h"
#include "text.h"


#define WINDOW_MAX_MODES 8
#define WINDOW_MAX_MODALS 8

typedef struct menu Menu;
typedef struct modal Modal;


/* typedef enum input_mode : uint8_t InputMode; */
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
    Font *bold_font;
    Font *mono_font;
    Font *mono_bold_font;
    Layout *layout;

    uint16_t i_state;

    Menu *menus[MAX_WINDOW_MENUS];
    uint8_t num_menus;

    InputMode modes[WINDOW_MAX_MODES];
    uint8_t num_modes;

    Modal *modals[WINDOW_MAX_MODALS];
    uint8_t num_modals;

    bool screenrecording;
} Window;

/* Create a new Window struct and initialize all members */
Window *window_create(int w, int h, const char *name);

/* When window is moved, run check to ensure that dpi scale factor is reset */
void window_check_monitor_dpi(Window *win);

/* Destroy a Window, its renderer and canvas, its std_font, and its main layout */
void window_destroy(Window *win);

/* Create a Font object, open TTF Fonts, and assign to window */
void window_assign_font(Window *win, const char *font_path, FontType type);


/* Reset the values of the w and h members of a Window struct based on current window dimensions */
void window_auto_resize(Window *window);

/* Force resize a window */
void window_resize(Window *win, int w, int h);

/* Passively resize a window, in response to system-driven resize */
void window_resize_passive(Window *win, int w, int h);

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
void window_pop_menu(Window *win);
void window_add_menu(Window *win, Menu *menu);

Menu *window_top_menu(Window *win);
void window_draw_menus(Window *win);

void window_push_mode(Window *win, InputMode im);
void window_pop_mode(Window *win);

void window_push_modal(Window *win, Modal *modal);
void window_pop_modal(Window *win);
void window_draw_modals(Window *win);


#endif

