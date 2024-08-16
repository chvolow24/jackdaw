#ifndef JDAW_GUI_DRAW_H
#define JDAW_GUI_DRAW_H

#include <stdbool.h>
#include "layout.h"
#include "window.h"

/* Main drawing function */
void layout_draw_main();

void layout_draw(Window *win, Layout *lt);

#endif
