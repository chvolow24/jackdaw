#ifndef JDAW_TIMELINE_H
#define JDAW_TIMELINE_H

#include <stdint.h>

uint32_t get_abs_tl_x(int draw_x);
int get_tl_draw_x(uint32_t abs_x);
int get_tl_draw_w(uint32_t abs_w);
void translate_tl(int translate_by_x, int translate_by_y);
void rescale_timeline(double scale_factor, uint32_t center_draw_position);

#endif
