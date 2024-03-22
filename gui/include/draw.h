#ifndef JDAW_GUI_DRAW_H
#define JDAW_GUI_DRAW_H

#include <stdbool.h>
#include "layout.h"
#include "window.h"

/* Main drawing function */
void layout_draw_main(Layout *clicked_lt);

void draw_layout(Window *win, Layout *lt);

#endif
