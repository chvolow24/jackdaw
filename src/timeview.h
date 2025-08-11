/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

/*****************************************************************************************************************
    timeview.h

    * GUI representations of linear time
    * translate between draw coordinates (int) and absolute time
      values in sample frames (int32_t)
    * zoom and offset (move left and right)
    * other timeline operations in project.h
 *****************************************************************************************************************/

#ifndef JDAW_TIMEVIEW_H
#define JDAW_TIMEVIEW_H

#include <stdbool.h>
#include <stdint.h>
#include "SDL.h"

typedef struct {
    double sample_frames_per_pixel;
    int32_t offset_left_sframes;
    SDL_Rect *rect;
    int32_t *play_pos;
    int32_t *in_mark;
    int32_t *out_mark;
    bool restrict_view;
    int32_t view_min;
    int32_t view_max;
    double max_sfpp;
} TimeView;

int32_t timeview_get_pos_sframes(TimeView *tv, int draw_x);

double timeview_get_draw_x_precise(TimeView *tv, int32_t pos);
double timeview_get_draw_w_precise(TimeView *tv, int32_t abs_w);
int timeview_get_draw_x(TimeView *tv, int32_t pos);
int timeview_get_draw_w(TimeView *tv, int32_t abs_w);



int32_t timeview_get_w_sframes(TimeView *tv, int draw_w_pix);
void timeview_scroll_horiz(TimeView *tl, int scroll_by_pix);
void timeview_rescale(TimeView *tv, double sfpp_scale_factor, bool on_mouse, SDL_Point mousep);
void timeview_catchup(TimeView *tv);


#endif
