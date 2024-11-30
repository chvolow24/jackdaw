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

#ifndef JDAW_TIMELINE_H
#define JDAW_TIMELINE_H

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

#define TL_SHIFT_STEP (50 * main_win->dpi_scale_factor)
#define TL_SCROLL_STEP_H (10 * main_win->dpi_scale_factor)
#define TL_SCROLL_STEP_V (10 * main_win->dpi_scale_factor)

#define SFPP_STEP 1.2 // Sample Frames Per Pixel
#define CATCHUP_STEP (600 * scale_factor)

typedef struct timeline Timeline;
int32_t timeline_get_abspos_sframes(Timeline *tl, int draw_x);
int timeline_get_draw_x(Timeline *tl, int32_t abs_x);
int timeline_get_draw_w(Timeline *tl, int32_t abs_w);
int32_t timeline_get_abs_w_sframes(Timeline *tl, int draw_w);
void timeline_scroll_horiz(Timeline *tl, int scroll_by);
float timeline_get_leftmost_seconds(Timeline *tl);
float timeline_get_second_w(Timeline *tl);
float timeline_first_second_tick_x(Timeline *tl, int *first_second);
void timeline_rescale(Timeline *tl, double sfpp_scale_factor, bool on_mouse);
void timeline_set_play_position(Timeline *tl, int32_t abs_pos_sframes);
void timeline_move_play_position(Timeline *tl, int32_t move_by_sframes);

void timeline_set_timecode(Timeline *tl);
void timeline_catchup(Timeline *tl);

void timecode_str_at(Timeline *tl, char *dst, size_t dstsize, int32_t pos);

#endif
