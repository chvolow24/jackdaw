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

#ifndef JDAW_GUI_H
#define JDAW_GUI_H
#include "SDL.h"
#include "project.h"

#define DEFAULT_WINDOW_FLAGS SDL_WINDOW_ALLOW_HIGHDPI
#define DEFAULT_RENDER_FLAGS SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC

#define STD_RAD 20

#define PLAYHEAD_TRI_H (10 * scale_factor)
#define TRACK_SPACING (5 * scale_factor)
#define PADDING (4 * scale_factor)
// #define MAX_SFPP 80000
#define COLOR_BAR_W (5 * scale_factor)

#define TRACK_CONSOLE_WIDTH (160 * scale_factor)

#define TRACK_CONSOLE (Dim) {ABS, 0}, (Dim) {ABS, 0}, (Dim) {ABS, TRACK_CONSOLE_WIDTH}, (Dim) {REL, 100}
#define TRACK_NAME_RECT (Dim) {REL, 1}, (Dim) {ABS, 4}, (Dim) {REL, 75}, (Dim) {ABS, 16}
#define CLIP_NAME_RECT (Dim) {ABS, 5}, (Dim) {REL, 3}, (Dim) {ABS, 50}, (Dim) {ABS, 10}
#define TRACK_IN_ROW (Dim) {REL, 1}, (Dim) {ABS, 24}, (Dim) {REL, 99}, (Dim) {ABS, 16}
#define TRACK_IN_LABEL (Dim) {ABS, 4}, (Dim) {ABS, 0}, (Dim) {REL, 20}, (Dim) {REL, 100}
#define TRACK_IN_NAME (Dim) {ABS, 40}, (Dim) {ABS, 0}, (Dim) {REL, 60}, (Dim) {REL, 100}
#define NAMEBOX_W 75
#define TRACK_IN_W 64
#define TRACK_INTERNAL_PADDING (6 * scale_factor)
#define CURSOR_COUNTDOWN 50
#define CURSOR_WIDTH (1 * scale_factor)
#define MAX_TB_LIST_LEN 100

#define MENU_LIST_R 14

/* For convenient initialization of windows and drawing resources */
typedef struct jdaw_window {
    SDL_Window *win;
    SDL_Renderer *rend;
    TTF_Font *fonts[11];
    TTF_Font *bold_fonts[11];
    uint8_t scale_factor;
    int w;
    int h;
} JDAWWindow;

/* Draw at an absolute position or relative to dimensions of parent */
enum dimType {REL, ABS};

typedef struct dim {
    enum dimType dimType;
    int value;
} Dim;

typedef struct textbox Textbox;

typedef struct textbox {
    char *value;
    SDL_Rect container; // populated at creation with values determined by preceding members
    SDL_Rect txt_container;     // populated at creation ''
    TTF_Font *font;
    JDAW_Color *txt_color;  // optional; default if null
    JDAW_Color *border_color;   // optional; default if null
    JDAW_Color *bckgrnd_color;  // optional; default if null
    void (*onclick)(void *object); // optional; function to run when txt box clicked
    void (*onhover)(void *object); // optional; function to run when mouse hovers over txt box
    int padding;
    int radius;
    bool show_cursor;
    uint8_t cursor_countdown;
    uint8_t cursor_pos;
    char *tooltip;  // optional
    bool mouse_hover;
} Textbox;

/* An array of textboxes, all of which share may share an onclick functions */
typedef struct textbox_list {
    Textbox *textboxes[MAX_TB_LIST_LEN];
    uint8_t num_textboxes;
    SDL_Rect container;
    JDAW_Color *txt_color;  // optional; default if null
    JDAW_Color *border_color;   // optional; default if null
    JDAW_Color *bckgrnd_color;  // optional; default if null
    int padding;
    int radius;
} TextboxList;


JDAWWindow *create_jwin(const char *title, int x, int y, int w, int h);
void destroy_jwin(JDAWWindow *jwin);
void reset_dims(JDAWWindow *jwin);

SDL_Rect get_rect(SDL_Rect parent_rect, Dim x, Dim y, Dim w, Dim h);
SDL_Rect relative_rect(SDL_Rect *win_rect, float x_rel, float y_rel, float w_rel, float h_rel);
void rescale_timeline(double scale_factor, uint32_t center_draw_position);

uint32_t get_abs_tl_x(int draw_x);
int get_tl_draw_x(uint32_t abs_x);
int get_tl_draw_w(uint32_t abs_w);
void translate_tl(int translate_by_x, int translate_by_y);
void position_textbox(Textbox *tb, int x, int y);

Textbox *create_textbox(
    int fixed_w, 
    int fixed_h, 
    int padding, 
    TTF_Font *font, 
    char *value,
    JDAW_Color *txt_color,
    JDAW_Color *border_color,
    JDAW_Color *bckgrnd_clr,
    void (*onclick)(void *object),
    void (*onhover)(void *object),
    char *tooltip,
    int radius
);

TextboxList *create_textbox_list_from_strings(
    int fixed_w,
    int padding,
    TTF_Font *font,
    char **values,
    uint8_t num_values,
    JDAW_Color *txt_color,
    JDAW_Color *border_color,
    JDAW_Color *bckgrnd_clr,
    void (*onclick)(void *object),
    void (*onhover)(void *object),
    char *tooltip,
    int radius
);


char *edit_textbox(Textbox *tb);
void position_textbox_list(TextboxList *tbl, int x, int y);

#endif