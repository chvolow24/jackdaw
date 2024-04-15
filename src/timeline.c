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

#define MAX_SFPP 80000

extern Project *proj;
extern Window *main_win;

int timeline_get_draw_x(int32_t abs_x);

/* Get the timeline position value -- in sample frames -- from a draw x coordinate  */
int32_t timeline_get_abspos_sfrmaes(int draw_x)
{
    Timeline *tl = proj->timelines[proj->active_tl_index];
    return (draw_x - proj->audio_rect->x) * proj->timelines[proj->active_tl_index]->sample_frames_per_pixel + proj->timelines[proj->active_tl_index]->display_offset_sframes;
}

/* Get the current draw x coordinate for a given timeline offset value (sample frames) */
int timeline_get_draw_x(int32_t abs_x)
{
    Timeline *tl = proj->timelines[proj->active_tl_index];
    if (tl->sample_frames_per_pixel != 0) {
        float precise = (float)proj->audio_rect->x + ((float)abs_x - (float)tl->display_offset_sframes) / (float)tl->sample_frames_per_pixel;
        return (int) precise;
    } else {
        fprintf(stderr, "Error: proj tl sfpp value 0\n");
        return 0;
    }
}

/* Get a draw width from a given length in sample frames */
int timeline_get_draw_w(int32_t abs_w)
{
    Timeline *tl = proj->timelines[proj->active_tl_index];
    if (tl->sample_frames_per_pixel != 0) {
        return abs_w / tl->sample_frames_per_pixel;
    } else {
        fprintf(stderr, "Error: proj tl sfpp value 0\n");
        return 0;
    }
}

/* Get a length in sample frames from a given draw width */
int32_t timeline_get_abs_w_sframes(int draw_w)
{
    Timeline *tl = proj->timelines[proj->active_tl_index];
    return draw_w * tl->sample_frames_per_pixel;
}

float timeline_get_leftmost_seconds();
int timeline_get_second_w();

void timeline_scroll_horiz(int scroll_by)
{

    Timeline *tl = proj->timelines[proj->active_tl_index];
    int32_t new_offset = tl->display_offset_sframes + timeline_get_abs_w_sframes(scroll_by);
    tl->display_offset_sframes = new_offset;
    timeline_reset(tl);

}

float timeline_get_leftmost_seconds()
{
    if (!proj) {
        fprintf(stderr, "Error: request to get second w with no active project.\n");
        exit(1);
    }
    Timeline *tl = proj->timelines[proj->active_tl_index];
    return (float) tl->display_offset_sframes / tl->proj->sample_rate;
}

int timeline_get_second_w()
{
    if (!proj) {
        fprintf(stderr, "Error: request to get second w with no active project.\n");
        exit(1);
    }
    int ret = timeline_get_draw_w(proj->sample_rate);
    return ret <= 0 ? 1 : ret;
}

int timeline_first_second_tick_x()
{
    float lms = timeline_get_leftmost_seconds();
    float dec = lms - (int)lms;
    return 1 - (timeline_get_second_w() * dec) + proj->audio_rect->x;
}

void timeline_rescale(double sfpp_scale_factor, int32_t center_abs_pos)
{
    Timeline *tl = proj->timelines[proj->active_tl_index];
    if (sfpp_scale_factor == 0) {
        fprintf(stderr, "Warning! Scale factor 0 in rescale_timeline\n");
        return;
    }
    int init_draw_pos = timeline_get_draw_x(center_abs_pos);
    double new_sfpp = tl->sample_frames_per_pixel / sfpp_scale_factor;

    if (new_sfpp < 1 || new_sfpp > MAX_SFPP) {
        return;
    }
    if ((int)new_sfpp == tl->sample_frames_per_pixel) {
        tl->sample_frames_per_pixel += sfpp_scale_factor < 0 ? -1 : 1;
    } else {
        tl->sample_frames_per_pixel = new_sfpp;
    }

    int new_draw_pos = timeline_get_draw_x(center_abs_pos);
    int offset_draw_delta = new_draw_pos - init_draw_pos;
    tl->display_offset_sframes += (timeline_get_abs_w_sframes(offset_draw_delta));

    timeline_reset(tl);
    /* Track *track = NULL; */
    /* for (int i=0; i<tl->num_tracks; i++) { */
    /*     track = tl->tracks[i]; */
    /*     for (int j=0; j<track->num_clips; j++) { */
    /*         reset_cliprect(track->clips[j]); */
    /*     } */
    /* } */
}

static void timeline_set_timecode()
{
    if (!proj) {
        fprintf(stderr, "Error: request to set timecode with no active project.\n");
        exit(1);
    }

    Timeline *tl = proj->timelines[proj->active_tl_index];
    char sign = tl->play_pos_sframes < 0 ? '-' : '+';
    uint32_t abs_play_pos = abs(tl->play_pos_sframes);
    uint8_t seconds, minutes, hours;
    uint32_t frames;

    double total_seconds = (double) abs_play_pos / (double) proj->sample_rate;
    hours = total_seconds / 3600;
    minutes = (total_seconds - 3600 * hours) / 60;
    seconds = total_seconds - (3600 * hours) - (60 * minutes);
    frames = abs_play_pos - ((int) total_seconds * proj->sample_rate);

    tl->timecode.hours = hours;
    tl->timecode.minutes = minutes;
    tl->timecode.seconds = seconds;
    tl->timecode.frames = frames;
    sprintf(tl->timecode.str, "%c%02d:%02d:%02d:%05d", sign, hours, minutes, seconds, frames);
    textbox_reset(tl->timecode_tb);
}

void timeline_set_play_position(int32_t abs_pos_sframes)
{
    Timeline *tl = proj->timelines[proj->active_tl_index];
    tl->play_pos_sframes = abs_pos_sframes;
    timeline_set_timecode();
}

void timeline_move_play_position(int32_t move_by_sframes)
{
    Timeline *tl = proj->timelines[proj->active_tl_index];
    tl->play_pos_sframes += move_by_sframes;
    timeline_set_timecode();
}

