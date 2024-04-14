#ifndef JDAW_GUI_GEOM_H
#define JDAW_GUI_GEOM_H

#include "SDL.h"


void geom_draw_rounded_rect(SDL_Renderer *rend, SDL_Rect *rect, int r);

void geom_fill_rounded_rect(SDL_Renderer *rend, SDL_Rect *rect, int r);

void geom_draw_rect_thick(SDL_Renderer *rend, SDL_Rect *rect, int r, double dpi_scale_factor);

#endif
