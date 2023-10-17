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

#include <stdio.h>
#include <stdint.h>
#include "project.h"
#include "gui.h"

#define MAX_SFPP 80000

extern Project *proj;
extern uint8_t scale_factor;

int get_tl_draw_x(uint32_t abs_x);

/* Get the timeline position value -- in sample frames -- from a draw x coordinate  */
uint32_t get_abs_tl_x(int draw_x)
{
    fprintf(stderr, "draw x into audio rect: %d\nsfpp: %f\n", draw_x - proj->tl->audio_rect.x, proj->tl->sample_frames_per_pixel);
    fprintf(stderr, "Return val: %d\n", (uint32_t) ((draw_x - proj->tl->audio_rect.x) * proj->tl->sample_frames_per_pixel + proj->tl->offset));
    fprintf(stderr, "Reverse: %d\n", get_tl_draw_x((uint32_t) ((draw_x - proj->tl->audio_rect.x) * proj->tl->sample_frames_per_pixel + proj->tl->offset)) - proj->tl->audio_rect.x);
    return (draw_x - proj->tl->audio_rect.x) * proj->tl->sample_frames_per_pixel + proj->tl->offset; 
}

/* Get the current draw x coordinate for a given timeline offset value (sample frames) */
int get_tl_draw_x(uint32_t abs_x) 
{
    if (proj->tl->sample_frames_per_pixel != 0) {
        float precise = (float) proj->tl->audio_rect.x + ((float) abs_x - (float) proj->tl->offset) / (float) proj->tl->sample_frames_per_pixel;
        return (int) precise;
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

float get_leftmost_seconds();
int get_second_w();

void translate_tl(int translate_by_x, int translate_by_y)
{
    if (abs(translate_by_y) >= abs(translate_by_x)) {
        translate_by_y = proj->tl->v_offset + translate_by_y < 0 ? translate_by_y : proj->tl->v_offset * -1;

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
    fprintf(stderr, "New leftmost seconds: %f\n", get_leftmost_seconds());
    fprintf(stderr, "Width of one second in pixels: %d\n", get_second_w());

}

float get_leftmost_seconds()
{
    if (!proj) {
        fprintf(stderr, "Error: request to get second w with no active project.\n");
        exit(1);
    }
    return (float) proj->tl->offset / proj->tl->proj->sample_rate;
}

int get_second_w()
{
    if (!proj) {
        fprintf(stderr, "Error: request to get second w with no active project.\n");
        exit(1);
    }
    int ret = get_tl_draw_w(proj->sample_rate);
    return ret <= 0 ? 1 : ret;
}

int first_second_tick_x()
{
    float lms = get_leftmost_seconds();
    float dec = lms - (int)lms;
    return 1 - (get_second_w() * dec) + proj->tl->audio_rect.x;
}

void rescale_timeline(double sfpp_scale_factor, uint32_t center_abs_pos) 
{
    if (sfpp_scale_factor == 0) {
        fprintf(stderr, "Warning! Scale factor 0 in rescale_timeline\n");
        return;
    }
    int init_draw_pos = get_tl_draw_x(center_abs_pos);
    double new_sfpp = proj->tl->sample_frames_per_pixel / sfpp_scale_factor;

    if (new_sfpp < 1 || new_sfpp > MAX_SFPP) {
        return;
    }
    if ((int)new_sfpp == proj->tl->sample_frames_per_pixel) {
        proj->tl->sample_frames_per_pixel += sfpp_scale_factor < 0 ? -1 : 1;
    } else {
        proj->tl->sample_frames_per_pixel = new_sfpp;
    }
    fprintf(stderr, "New sfpp f: %f, int: %d\n", new_sfpp, proj->tl->sample_frames_per_pixel);

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

void set_timecode()
{
    if (!proj) {
        fprintf(stderr, "Error: request to set timecode with no active project.\n");
        exit(1);
    }

    uint8_t seconds, minutes, hours;
    uint32_t frames;

    double total_seconds = (double) proj->tl->play_position / (double) proj->sample_rate;
    hours = total_seconds / 3600;
    minutes = (total_seconds - 3600 * hours) / 60;
    seconds = total_seconds - (3600 * hours) - (60 * minutes);
    frames = proj->tl->play_position - ((int) total_seconds * proj->sample_rate);

    proj->tl->timecode.hours = hours;
    proj->tl->timecode.minutes = minutes;
    proj->tl->timecode.seconds = seconds;
    proj->tl->timecode.frames = frames;
    sprintf(proj->tl->timecode.str, "%02d:%02d:%02d:%05d", hours, minutes, seconds, frames);
    reset_textbox_value(proj->tl->timecode_tb, proj->tl->timecode.str);


}