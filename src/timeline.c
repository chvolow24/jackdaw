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

    * Functions related to timeline positions; others in project.c
    * Translate between draw coordinates and absolute positions (time values) within the timeline
 *****************************************************************************************************************/

#include <stdio.h>
#include <stdint.h>
#include "project.h"
#include "transport.h"

#define MAX_SFPP 80000

#define TIMELINE_CATCHUP_W (main_win->w_pix / 2)

extern Project *proj;
extern Window *main_win;

int timeline_get_draw_x(int32_t abs_x);

/* Get the timeline position value -- in sample frames -- from a draw x coordinate  */
int32_t timeline_get_abspos_sframes(int draw_x)
{
    Timeline *tl = proj->timelines[proj->active_tl_index];
    return (draw_x - proj->audio_rect->x) * tl->sample_frames_per_pixel + tl->display_offset_sframes;
}

/* Get the current draw x coordinate for a given timeline offset value (sample frames) */
int timeline_get_draw_x(int32_t abs_x)
{
    Timeline *tl = proj->timelines[proj->active_tl_index];
    if (tl->sample_frames_per_pixel != 0) {
        float precise = (float)proj->audio_rect->x + ((float)abs_x - (float)tl->display_offset_sframes) / (float)tl->sample_frames_per_pixel;
        return (int) round(precise);
    } else {
        fprintf(stderr, "Error: proj tl sfpp value 0\n");
	exit(0);
        /* return 0; */
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

float timeline_get_draw_w_precise(int32_t abs_w)
{
    Timeline *tl = proj->timelines[proj->active_tl_index];
    if (tl->sample_frames_per_pixel != 0) {
	return (float) abs_w / tl->sample_frames_per_pixel;
    }
    return 0.0f;
}
 
/* Get a length in sample frames from a given draw width */
int32_t timeline_get_abs_w_sframes(int draw_w)
{
    Timeline *tl = proj->timelines[proj->active_tl_index];
    return draw_w * tl->sample_frames_per_pixel;
}

float timeline_get_leftmost_seconds();
float timeline_get_second_w();

void timeline_scroll_horiz(int scroll_by)
{
    Timeline *tl = proj->timelines[proj->active_tl_index];
    int32_t new_offset = tl->display_offset_sframes + timeline_get_abs_w_sframes(scroll_by);
    tl->display_offset_sframes = new_offset;
    timeline_reset(tl, false);
}

float timeline_get_leftmost_seconds()
{
    Timeline *tl = proj->timelines[proj->active_tl_index];
    return (float) tl->display_offset_sframes / tl->proj->sample_rate;
}

float timeline_get_second_w()
{
    return timeline_get_draw_w_precise(proj->sample_rate);
    /* return ret <= 0 ? 1 : ret; */
}

float timeline_first_second_tick_x(int *first_second)
{
    float lms = timeline_get_leftmost_seconds();
    float dec = lms - floor(lms);
    *first_second = (int)floor(lms) + 1;
    return timeline_get_second_w() * (1 - dec);
    /* return 1 - (timeline_get_second_w() * dec) + proj->audio_rect->x; */
}

void timeline_rescale(double sfpp_scale_factor, bool on_mouse)
{
    if (sfpp_scale_factor == 1.0f) return;
    Timeline *tl = proj->timelines[proj->active_tl_index];
    int32_t center_abs_pos = 0;
    if (on_mouse) {
	center_abs_pos = timeline_get_abspos_sframes(main_win->mousep.x);
    } else {
	center_abs_pos = tl->play_pos_sframes;
    }
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
        tl->sample_frames_per_pixel += sfpp_scale_factor <= 1.0f ? 1 : -1;
	if (tl->sample_frames_per_pixel < 1) tl->sample_frames_per_pixel = 1;
    } else {
        tl->sample_frames_per_pixel = new_sfpp;
    }

    int new_draw_pos = timeline_get_draw_x(center_abs_pos);
    int offset_draw_delta = new_draw_pos - init_draw_pos;
    tl->display_offset_sframes += (timeline_get_abs_w_sframes(offset_draw_delta));

    timeline_reset(tl, true);
    /* Track *track = NULL; */
    /* for (int i=0; i<tl->num_tracks; i++) { */
    /*     track = tl->tracks[i]; */
    /*     for (int j=0; j<track->num_clips; j++) { */
    /*         reset_cliprect(track->clips[j]); */
    /*     } */
    /* } */
}

void timecode_str_at(char *dst, size_t dstsize, int32_t pos)
{
    char sign = pos < 0 ? '-' : '+';
    uint32_t abs_play_pos = abs(pos);
    uint8_t seconds, minutes, hours;
    uint32_t frames;

    double total_seconds = (double) abs_play_pos / (double) proj->sample_rate;
    hours = total_seconds / 3600;
    minutes = (total_seconds - 3600 * hours) / 60;
    seconds = total_seconds - (3600 * hours) - (60 * minutes);
    frames = abs_play_pos - ((int) total_seconds * proj->sample_rate);

    snprintf(dst, dstsize, "%c%02d:%02d:%02d:%05d", sign, hours, minutes, seconds, frames);
}

void timeline_set_timecode()
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
    snprintf(tl->timecode.str, sizeof(tl->timecode.str), "%c%02d:%02d:%02d:%05d", sign, hours, minutes, seconds, frames);
    if (tl->timecode_tb) {
	/* fprintf(stdout, "Resetting tb\n"); */
	textbox_reset_full(tl->timecode_tb);
	/* fprintf(stdout, "TC content? \"%s\"\n", tl->timecode_tb->text->value_handle); */
	/* fprintf(stdout, "TC disp val? \"%s\"\n", tl->timecode_tb->text->display_value); */
	/* fprintf(stdout, "Lt w? %d\n", tl->timecode_tb->layout->rect.w); */
    }
}
static void track_handle_playhead_jump(Track *track)
{
    for (uint8_t i =0; i<track->num_automations; i++) {
	Automation *a = track->automations[i];
	a->current = NULL;
	a->ghost_valid = false;
    }
}


void timeline_set_play_position(int32_t abs_pos_sframes)
{
    Timeline *tl = proj->timelines[proj->active_tl_index];
    /* float saved_speed = proj->play_speed; */
    /* bool restart = false; */
    /* if (proj->playing) { */
    /* 	transport_stop_playback(); */
    /* 	restart = true; */
    /* } */
    tl->play_pos_sframes = abs_pos_sframes;
    tl->read_pos_sframes = abs_pos_sframes;
    /* if (restart) { */
    /* 	proj->play_speed = saved_speed; */
    /* 	transport_start_playback(); */
    /* } */

    for (uint8_t i=0; i<tl->num_tracks; i++) {
	track_handle_playhead_jump(tl->tracks[i]);
    }
    timeline_set_timecode();
    tl->needs_redraw = true;
}



void timeline_move_play_position(int32_t move_by_sframes)
{
    Timeline *tl = proj->timelines[proj->active_tl_index];
    tl->play_pos_sframes += move_by_sframes;

    if (proj->dragging) {
	for (uint8_t i=0; i<tl->num_grabbed_clips; i++) {
	    ClipRef *cr = tl->grabbed_clips[i];
	    cr->pos_sframes += move_by_sframes;
	    /* clipref_reset(cr); */
	}
    }
    tl->needs_redraw = true;
    
    /* timeline_set_timecode(); */
}


void timeline_catchup()
{
    Timeline *tl = proj->timelines[proj->active_tl_index];
    int tl_draw_x;
    /* uint32_t move_by_sframes; */
    int catchup_w = TIMELINE_CATCHUP_W;
    if (proj->audio_rect->w <= 0) {
	return;
    }
    /* while (catchup_w > proj->audio_rect->w / 2 && catchup_w > 10) { */
    /* 	catchup_w /= 2; */
    /* } */
    if ((tl_draw_x = timeline_get_draw_x(tl->play_pos_sframes)) > proj->audio_rect->x + proj->audio_rect->w) {
	tl->display_offset_sframes = tl->play_pos_sframes - timeline_get_abs_w_sframes(proj->audio_rect->w - catchup_w);
	timeline_reset(tl, false);
    } else if (tl_draw_x < proj->audio_rect->x) {
	tl->display_offset_sframes = tl->play_pos_sframes - timeline_get_abs_w_sframes(catchup_w);
	timeline_reset(tl, false);
    }
}

