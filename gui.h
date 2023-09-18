/*****************************************************************************************************************
 * Jackdaw | a stripped-down, keyboard-focused Digital Audio Workstation | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023 Charlie Volow

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

**************************************************************************************************/

#ifndef JDAW_GUI_H
#define JDAW_GUI_H
#include "SDL.h"
#include "project.h"

#define STD_RAD 20

#define PLAYHEAD_TRI_H (10 * proj->scale_factor)
#define TRACK_SPACING (5 * proj->scale_factor)
#define PADDING (4 * proj->scale_factor)
#define MAX_SFPP 8000
#define COLOR_BAR_W (5 * proj->scale_factor)

#define TRACK_CONSOLE_WIDTH 140

#define TRACK_CONSOLE (Dim) {ABS, 0}, (Dim) {ABS, 0}, (Dim) {ABS, TRACK_CONSOLE_WIDTH}, (Dim) {REL, 100}
#define TRACK_NAME_RECT (Dim) {REL, 1}, (Dim) {ABS, 4}, (Dim) {REL, 75}, (Dim) {ABS, 16}
#define CLIP_NAME_RECT (Dim) {ABS, 5}, (Dim) {REL, 3}, (Dim) {ABS, 50}, (Dim) {ABS, 10}
#define TRACK_IN_ROW (Dim) {REL, 1}, (Dim) {ABS, 24}, (Dim) {REL, 99}, (Dim) {ABS, 16}
#define TRACK_IN_LABEL (Dim) {ABS, 4}, (Dim) {ABS, 0}, (Dim) {REL, 20}, (Dim) {REL, 100}
#define TRACK_IN_NAME (Dim) {ABS, 40}, (Dim) {ABS, 0}, (Dim) {REL, 60}, (Dim) {REL, 100}
#define NAMEBOX_W (100 * proj->scale_factor)


enum dimType {REL, ABS};

typedef struct dim {
    enum dimType dimType;
    int value;
} Dim;

typedef struct textbox Textbox;

typedef struct textbox{
    // int x;
    // int y;
    // int fixed_w;    // optional creation argument; box sized to text if null
    // int fixed_h;    // optional creation argument; box sized to text if null
    int padding;    // optional; default used if null
    TTF_Font *font;
    char *value;
    SDL_Rect container; // populated at creation with values determined by preceding members
    SDL_Rect txt_container;     // populated at creation ''
    JDAW_Color *txt_color;  // optional; default if null
    JDAW_Color *border_color;   // optional; default if null
    JDAW_Color *bckgrnd_color;  // optional; default if null
    void (*onclick)(Textbox *self); // optional; function to run when txt box clicked
    void (*onhover)(Textbox *self); // optional; function to run when mouse hovers over txt box
    char *tooltip;  // optional
} Textbox;

SDL_Rect get_rect(SDL_Rect parent_rect, Dim x, Dim y, Dim w, Dim h);
SDL_Rect relative_rect(SDL_Rect *win_rect, float x_rel, float y_rel, float w_rel, float h_rel);
void draw_project(Project *proj);
void rescale_timeline(double scale_factor, uint32_t center_draw_position);

uint32_t get_abs_tl_x(int draw_x);
int get_tl_draw_x(uint32_t abs_x);
int get_tl_draw_w(uint32_t abs_w);
void translate_tl(int translate_by_w);

Textbox *create_textbox(
    int fixed_w, 
    int fixed_h, 
    int padding, 
    TTF_Font *font, 
    char *value,
    JDAW_Color *txt_color,
    JDAW_Color *border_color,
    JDAW_Color *bckgrnd_color,
    void (*onclick)(Textbox *self),
    void (*onhover)(Textbox *self),
    char *tooltip
);

#endif