#ifndef JDAW_GUI_H
#define JDAW_GUI_H
#include "SDL.h"
#include "project.h"

#define STD_RAD 20

enum dimType {REL, ABS};

typedef struct dim {
    enum dimType dimType;
    int value;
} Dim;

SDL_Rect get_rect(SDL_Rect parent_rect, Dim x, Dim y, Dim w, Dim h);
SDL_Rect relative_rect(SDL_Rect *win_rect, float x_rel, float y_rel, float w_rel, float h_rel);
void draw_project(Project *proj);
void rescale_timeline(double scale_factor, uint32_t center_draw_position);

uint32_t get_abs_tl_x(int draw_x);
int get_tl_draw_x(uint32_t abs_x);
int get_tl_draw_w(uint32_t abs_w);
void translate_tl(int translate_by_w);

#endif