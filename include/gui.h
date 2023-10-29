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

#define DEFAULT_WINDOW_FLAGS SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE
#define DEFAULT_RENDER_FLAGS SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC

#define STD_RAD 20

#define TB_TRUNC_THRESHOLD (12 * scale_factor)

#define TL_X_PADDING_UNSCALED 5
#define TL_X_PADDING (5 * scale_factor)

#define TL_Y_UNSCALED 150
// #define TL_Y_PADDING (20 * scale_factor)

#define CTRL_X_PADDING_UNSCALED 14
#define CTRL_Y_PADDING_UNSCALED 20
#define CTRL_RECT_COL_W_UNSCALED 350

#define PLAYHEAD_TRI_H (10 * scale_factor)
#define TRACK_SPACING (4 * scale_factor)
#define PADDING_UNSCALED 4
#define PADDING (PADDING_UNSCALED * scale_factor)

#define TL_RECT (Dim) {ABS, TL_X_PADDING_UNSCALED}, (Dim) {ABS, TL_Y_UNSCALED}, (Dim) {REL, 100}, (Dim) {REL, 76}

#define TRACK_CONSOLE_W_UNSCALED 160
#define TRACK_CONSOLE_W (TRACK_CONSOLE_W_UNSCALED * scale_factor)

#define TRACK_CONSOLE_ROW_HEIGHT_UNSCALED 24
#define RULER_HEIGHT_UNSCALED 30
#define RULER_HEIGHT (RULER_HEIGHT_UNSCALED * scale_factor)

#define TC_W_UNSCALED (TRACK_CONSOLE_W_UNSCALED - (PADDING_UNSCALED * 2))
#define TC_H_UNSCALED 20

#define TRACK_CONSOLE_RECT (Dim) {ABS, 0}, (Dim) {ABS, 0}, (Dim) {ABS, TRACK_CONSOLE_W_UNSCALED}, (Dim) {REL, 100}
#define TRACK_NAME_RECT (Dim) {REL, 1}, (Dim) {ABS, 4}, (Dim) {REL, 75}, (Dim) {ABS, 16}
#define CLIP_NAME_RECT (Dim) {ABS, 5}, (Dim) {REL, 3}, (Dim) {ABS, 50}, (Dim) {ABS, 10}

#define TRACK_NAME_ROW (Dim) {ABS, 4}, (Dim) {ABS, 4}, (Dim) {REL, 100}, (Dim) {ABS, TRACK_CONSOLE_ROW_HEIGHT_UNSCALED} // child of console
#define TRACK_VOL_ROW (Dim) {ABS, 4}, (Dim) {ABS, 4 + TRACK_CONSOLE_ROW_HEIGHT_UNSCALED}, (Dim) {REL, 100}, (Dim) {ABS, TRACK_CONSOLE_ROW_HEIGHT_UNSCALED} // child of console
#define TRACK_PAN_ROW (Dim) {ABS, 4}, (Dim) {ABS, 4 + TRACK_CONSOLE_ROW_HEIGHT_UNSCALED * 2}, (Dim) {REL, 100}, (Dim) {ABS, TRACK_CONSOLE_ROW_HEIGHT_UNSCALED} // child of console
#define TRACK_IN_ROW (Dim) {ABS, 4}, (Dim) {ABS, 4 + TRACK_CONSOLE_ROW_HEIGHT_UNSCALED * 3}, (Dim) {REL, 100}, (Dim) {ABS, TRACK_CONSOLE_ROW_HEIGHT_UNSCALED} // child of console

#define RULER_TC_CONTAINER (Dim) {ABS, 0}, (Dim) {ABS, 0}, (Dim) {REL, 100}, (Dim) {ABS, RULER_HEIGHT_UNSCALED} // child of tl
#define RULER_RECT (Dim) {ABS, TRACK_CONSOLE_W_UNSCALED + COLOR_BAR_W_UNSCALED + TL_X_PADDING_UNSCALED}, (Dim) {ABS, 0}, (Dim) {REL, 100}, (Dim) {REL, 100}
#define TC_RECT (Dim) {ABS, 12}, (Dim) {ABS, 5}, (Dim) {ABS, TC_W_UNSCALED}, (Dim) {ABS, TC_H_UNSCALED}

#define CTRL_RECT (Dim) {ABS, CTRL_X_PADDING_UNSCALED}, (Dim) {REL, 4}, (Dim) {REL, 100}, (Dim) {ABS, TL_Y_UNSCALED - RULER_HEIGHT_UNSCALED}
#define CTRL_RECT_COL_A (Dim) {ABS, 0}, (Dim) {ABS, 0}, (Dim) {ABS, CTRL_RECT_COL_W_UNSCALED}, (Dim) {REL, 100}
#define AUDIO_OUT_ROW (Dim) {ABS, 4}, (Dim) {ABS, 4}, (Dim) {REL, 100}, (Dim) {ABS, TRACK_CONSOLE_ROW_HEIGHT_UNSCALED}
#define AUDIO_OUT_W (120 * scale_factor)

#define COLOR_BAR_W_UNSCALED 5
#define COLOR_BAR_W (COLOR_BAR_W_UNSCALED * scale_factor)

#define NAMEBOX_W (96 * scale_factor)
#define TRACK_IN_W (100 * scale_factor)
#define TRACK_INTERNAL_PADDING (6 * scale_factor)

#define MUTE_SOLO_W (20 * scale_factor)

#define CURSOR_COUNTDOWN 50
#define CURSOR_WIDTH (1 * scale_factor)

#define MAX_TB_LEN 255
#define MAX_TB_LIST_LEN 100
#define MAX_MLI_LABEL_LEN 255

#define NUM_FONT_SIZES 11

#define MAX_ACTIVE_MENUS 8

#define CLIP_BORDER_W (2 * scale_factor)
#define ARROW_TL_STEP (20 * scale_factor)
#define ARROW_TL_STEP_SMALL (1 * scale_factor)
#define FSLIDER_PADDING (2 * scale_factor)

#define MENU_LIST_R 14

#define CLIP_NAME_PADDING_Y (4 * scale_factor)
#define CLIP_NAME_PADDING_X (6 * scale_factor)


typedef struct textbox_list TextboxList;

/* For convenient initialization of windows and drawing resources */
typedef struct jdaw_window {
    SDL_Window *win;
    SDL_Renderer *rend;
    TTF_Font *fonts[NUM_FONT_SIZES];
    TTF_Font *bold_fonts[NUM_FONT_SIZES];
    TTF_Font *mono_fonts[NUM_FONT_SIZES];
    uint8_t scale_factor;
    int w;
    int h;
    TextboxList *active_menus[MAX_ACTIVE_MENUS];
    uint8_t num_active_menus;
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
    char display_value[MAX_TB_LEN];
    SDL_Rect container; // populated at creation with values determined by preceding members
    SDL_Rect txt_container;     // populated at creation ''
    TTF_Font *font;
    JDAW_Color *txt_color;  // optional; default if null
    JDAW_Color *border_color;   // optional; default if null
    JDAW_Color *bckgrnd_color;  // optional; default if null
    void (*onclick)(Textbox *self, void *object); // optional; function to run when txt box clicked
    void *target;
    void (*onhover)(void *object); // optional; function to run when mouse hovers over txt box
    int padding;
    int radius;
    bool show_cursor;
    uint8_t cursor_countdown;
    uint8_t cursor_pos;
    char *tooltip;  // optional
    bool mouse_hover;
    bool available;
    bool allow_truncate;
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

typedef struct menulist_item {
    char label[MAX_MLI_LABEL_LEN];
    bool available;
} MenulistItem;

typedef enum fslider_type {
    FILL,
    LINE
} FSlider_type;

/* Float-valued slider */
typedef struct f_slider {
    float value;
    float max;
    float min;
    SDL_Rect rect;
    SDL_Rect bar_rect;
    FSlider_type type;
    // bool orientation_vertical;
} FSlider;




JDAWWindow *create_jwin(const char *title, int x, int y, int w, int h);
void destroy_jwin(JDAWWindow *jwin);

void reset_dims(JDAWWindow *jwin);
SDL_Rect get_rect(SDL_Rect parent_rect, Dim x, Dim y, Dim w, Dim h);
SDL_Rect relative_rect(SDL_Rect *win_rect, float x_rel, float y_rel, float w_rel, float h_rel);
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
    void (*onclick)(Textbox *self, void *object),
    void *target,
    void (*onhover)(void *self),
    char *tooltip,
    int radius,
    bool available,
    bool allow_truncate
);
TextboxList *create_textbox_list(
    int fixed_w,
    int padding,
    TTF_Font *font,
    MenulistItem **items,
    uint8_t num_items,
    JDAW_Color *txt_color,
    JDAW_Color *border_color,
    JDAW_Color *bckgrnd_clr,
    void (*onclick)(Textbox *self, void *object),
    void *target,
    void (*onhover)(void *object),
    char *tooltip,
    int radius
);
char *edit_textbox(Textbox *tb);
void position_textbox_list(TextboxList *tbl, int x, int y);
TextboxList *create_menulist(
    JDAWWindow *jwin,
    int fixed_w,
    int padding,
    TTF_Font *font,
    MenulistItem **items,
    uint8_t num_values,
    void (*onclick)(Textbox *self, void *object),
    void *target
);
void destroy_pop_menulist(JDAWWindow *jwin);
void destroy_textbox(Textbox *tb);
void menulist_hover(JDAWWindow *jwin, SDL_Point *mouse_p);
bool triage_menulist_mouseclick(JDAWWindow *jwin, SDL_Point *mouse_p);
void reset_textbox_value(Textbox *tb, char *new_val);

void reset_fslider(FSlider *fslider);
void set_fslider_rect(FSlider *fslider, SDL_Rect *rect, uint8_t padding);
bool adjust_fslider(FSlider *fslider, float change_by);

#endif