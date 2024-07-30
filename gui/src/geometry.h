#ifndef JDAW_GUI_GEOM_H
#define JDAW_GUI_GEOM_H


#include "SDL.h"

#define STD_CORNER_RAD (10 * main_win->dpi_scale_factor)

void geom_draw_circle(SDL_Renderer *rend, int orig_x, int orig_y, int r);

void geom_fill_circle(SDL_Renderer *rend, int orig_x, int orig_y, int r);

void geom_draw_rounded_rect(SDL_Renderer *rend, SDL_Rect *rect, int r);

void geom_fill_rounded_rect(SDL_Renderer *rend, SDL_Rect *rect, int r);

void geom_draw_rect_thick(SDL_Renderer *rend, SDL_Rect *rect, int r, double dpi_scale_factor);

#endif
