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

/*****************************************************************************************************************
    timeline.c

    * Translate between draw coordinates and absolute positions (time values) within the timeline
 *****************************************************************************************************************/


#include <stdint.h>
#include "project.h"
#include "gui.h"

#define MAX_SFPP 80000

extern Project *proj;
extern uint8_t scale_factor;

/* Get the timeline position value -- in sample frames -- from a draw x coordinate  */
uint32_t get_abs_tl_x(int draw_x)
{
    return (draw_x - proj->tl->audio_rect.x) * proj->tl->sample_frames_per_pixel + proj->tl->offset; 
}

/* Get the current draw x coordinate for a given timeline offset value (sample frames) */
int get_tl_draw_x(uint32_t abs_x) 
{
    if (proj->tl->sample_frames_per_pixel != 0) {
        return proj->tl->audio_rect.x + ((int) abs_x - (int) proj->tl->offset) / (int) proj->tl->sample_frames_per_pixel;
    } else {
        fprintf(stderr, "Error: proj tl sfpp value 0\n");
        return 0;
    }
}

/* Get a draw width from a given length in sample frames */
int get_tl_draw_w(uint32_t abs_w) 
{
    if (proj->tl->sample_frames_per_pixel != 0) {
        return abs_w / proj->tl->sample_frames_per_pixel;
    } else {
        fprintf(stderr, "Error: proj tl sfpp value 0\n");
        return 0;
    }
}

/* Get a length in sample frames from a given draw width */
int32_t get_tl_abs_w(int draw_w) 
{
    return draw_w * proj->tl->sample_frames_per_pixel;
}

void translate_tl(int translate_by_x, int translate_by_y)
{
    if (abs(translate_by_y) >= abs(translate_by_x)) {
        proj->tl->v_offset += translate_by_y;
        Track *track = NULL;
        for (int i=0; i<proj->tl->num_tracks; i++) {
            track = proj->tl->tracks[i];
            track->rect.y += translate_by_y;

            /* TEMPORARY */
            reset_track_internal_rects(track);
            // track->console_rect = get_rect(track->rect, TRACK_CONSOLE);
            // track->input_row_rect = get_rect(track->console_rect, TRACK_IN_ROW);
            // track->vol_row_rect = get_rect(track->console_rect, TRACK_IN_ROW);
            /* END TEMPORARY */

            for (int j=0; j<track->num_clips; j++) {
                reset_cliprect(track->clips[j]);
            }
        }
    } else {
        int32_t new_offset = proj->tl->offset + get_tl_abs_w(translate_by_x);
        if (new_offset < 0) {
            proj->tl->offset = 0;
        } else {
            proj->tl->offset = new_offset;
        }
        Track *track = NULL;
        for (int i=0; i<proj->tl->num_tracks; i++) {
            track = proj->tl->tracks[i];
            for (int j=0; j<track->num_clips; j++) {
                reset_cliprect(track->clips[j]);
            }
        }
    }


}

void rescale_timeline(double scale_factor, uint32_t center_abs_pos) 
{
    if (scale_factor == 0) {
        fprintf(stderr, "Warning! Scale factor 0 in rescale_timeline\n");
        return;
    }
    int init_draw_pos = get_tl_draw_x(center_abs_pos);
    double new_sfpp = proj->tl->sample_frames_per_pixel / scale_factor;

    if (new_sfpp < 2 || new_sfpp > MAX_SFPP) {
        return;
    }
    proj->tl->sample_frames_per_pixel = new_sfpp;

    int new_draw_pos = get_tl_draw_x(center_abs_pos);
    int offset_draw_delta = new_draw_pos - init_draw_pos;
    if (offset_draw_delta < (-1 * get_tl_draw_w(proj->tl->offset))) {
        proj->tl->offset = 0;
    } else {
        proj->tl->offset += (get_tl_abs_w(offset_draw_delta));
    }
    Track *track = NULL;
    for (int i=0; i<proj->tl->num_tracks; i++) {
        track = proj->tl->tracks[i];
        for (int j=0; j<track->num_clips; j++) {
            reset_cliprect(track->clips[j]);
        }
    }
}