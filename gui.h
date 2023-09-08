#ifndef JDAW_GUI_H
#define JDAW_GUI_H
#include "SDL2/SDL.h"
#include "project.h"

SDL_Rect relative_rect(SDL_Rect *win_rect, float x_rel, float y_rel, float w_rel, float h_rel);
void draw_project(Project *proj);

#endif