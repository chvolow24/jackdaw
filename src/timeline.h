/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

/*****************************************************************************************************************
    timeline.h

    * operations related to timeline positions
    * translate between draw coordinates and absolute positions
      (time values in sample frames)
    * other timeline operations in project.h
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
double timeline_get_leftmost_seconds(Timeline *tl);
double timeline_get_second_w(Timeline *tl);
double timeline_first_second_tick_x(Timeline *tl, int *first_second);
void timeline_rescale(Timeline *tl, double sfpp_scale_factor, bool on_mouse);

/* Invalidates continuous-play-dependent caches.
   Use this any time a "jump" occurs.
   MAIN (GUI) THREAD ONLY */
void timeline_set_play_position(Timeline *tl, int32_t abs_pos_sframes);

/* Use this for continuous playback incrementation.
   SDL AUDIO THREAD ONLY */
void timeline_move_play_position(Timeline *tl, int32_t move_by_sframes);

void timeline_set_timecode(Timeline *tl);
void timeline_catchup(Timeline *tl);

void timecode_str_at(Timeline *tl, char *dst, size_t dstsize, int32_t pos);

int32_t timeline_get_play_pos_now(Timeline *tl);
#endif
