/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
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
#include <time.h>
#include "project.h"
#include "tempo.h"
#include "thread_safety.h"
#include "timeline.h"
#include "transport.h"

#define MAX_SFPP 80000

#define TIMELINE_CATCHUP_W (main_win->w_pix / 2)

/* extern Project *proj; */
extern Window *main_win;
extern pthread_t MAIN_THREAD_ID;
extern pthread_t DSP_THREAD_ID;

/* int timeline_get_draw_x(int32_t abs_x); */

/* Get the timeline position value -- in sample frames -- from a draw x coordinate  */
int32_t timeline_get_abspos_sframes(Timeline *tl, int draw_x)
{
    /* Timeline *tl = proj->timelines[proj->active_tl_index]; */
    return (draw_x - tl->proj->audio_rect->x) * tl->sample_frames_per_pixel + tl->display_offset_sframes;
}

/* Get the current draw x coordinate for a given timeline offset value (sample frames) */
int timeline_get_draw_x(Timeline *tl, int32_t abs_x)
{
    /* Timeline *tl = proj->timelines[proj->active_tl_index]; */
    if (tl->sample_frames_per_pixel != 0) {
        double precise = (double)tl->proj->audio_rect->x + ((double)abs_x - (double)tl->display_offset_sframes) / (double)tl->sample_frames_per_pixel;
        return (int) round(precise);
    } else {
        fprintf(stderr, "Error: proj tl sfpp value 0\n");
	exit(0);
        /* return 0; */
    }
}

/* Get a draw width from a given length in sample frames */
int timeline_get_draw_w(Timeline *tl, int32_t abs_w)
{
    /* Timeline *tl = proj->timelines[proj->active_tl_index]; */
    if (tl->sample_frames_per_pixel != 0) {
        return abs_w / tl->sample_frames_per_pixel;
    } else {
        fprintf(stderr, "Error: proj tl sfpp value 0\n");
        return 0;
    }
}

static double timeline_get_draw_w_precise(Timeline *tl, int32_t abs_w)
{
    /* Timeline *tl = proj->timelines[proj->active_tl_index]; */
    if (tl->sample_frames_per_pixel != 0) {
	return (double) abs_w / tl->sample_frames_per_pixel;
    }
    return 0.0;
}
 
/* Get a length in sample frames from a given draw width */
int32_t timeline_get_abs_w_sframes(Timeline *tl, int draw_w)
{
    /* Timeline *tl = proj->timelines[proj->active_tl_index]; */
    return draw_w * tl->sample_frames_per_pixel;
}

void timeline_scroll_horiz(Timeline *tl, int scroll_by)
{
    /* Timeline *tl = proj->timelines[proj->active_tl_index]; */
    int32_t new_offset = tl->display_offset_sframes + timeline_get_abs_w_sframes(tl, scroll_by);
    tl->display_offset_sframes = new_offset;
    timeline_reset(tl, false);
}

double timeline_get_leftmost_seconds(Timeline *tl)
{
    /* Timeline *tl = proj->timelines[proj->active_tl_index]; */
    return (double) tl->display_offset_sframes / tl->proj->sample_rate;
}

double timeline_get_second_w(Timeline *tl)
{
    return timeline_get_draw_w_precise(tl, tl->proj->sample_rate);
    /* return ret <= 0 ? 1 : ret; */
}

double timeline_first_second_tick_x(Timeline *tl, int *first_second)
{
    double lms = timeline_get_leftmost_seconds(tl);
    double dec = lms - floor(lms);
    *first_second = (int)floor(lms) + 1;
    return timeline_get_second_w(tl) * (1 - dec);
    /* return 1 - (timeline_get_second_w() * dec) + proj->audio_rect->x; */
}

void timeline_rescale(Timeline *tl, double sfpp_scale_factor, bool on_mouse)
{
   if (sfpp_scale_factor == 1.0f) return;
    /* Timeline *tl = proj->timelines[proj->active_tl_index]; */
    int32_t center_abs_pos = 0;
    if (on_mouse) {
	center_abs_pos = timeline_get_abspos_sframes(tl, main_win->mousep.x);
    } else {
	center_abs_pos = tl->play_pos_sframes;
    }
    if (sfpp_scale_factor == 0) {
        fprintf(stderr, "Warning! Scale factor 0 in rescale_timeline\n");
        return;
    }
    int init_draw_pos = timeline_get_draw_x(tl, center_abs_pos);
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

    int new_draw_pos = timeline_get_draw_x(tl, center_abs_pos);
    int offset_draw_delta = new_draw_pos - init_draw_pos;
    tl->display_offset_sframes += (timeline_get_abs_w_sframes(tl, offset_draw_delta));

    timeline_reset(tl, true);
    /* Track *track = NULL; */
    /* for (int i=0; i<tl->num_tracks; i++) { */
    /*     track = tl->tracks[i]; */
    /*     for (int j=0; j<track->num_clips; j++) { */
    /*         reset_cliprect(track->clips[j]); */
    /*     } */
    /* } */
}

void timecode_str_at(Timeline *tl, char *dst, size_t dstsize, int32_t pos)
{
    char sign = pos < 0 ? '-' : '+';
    uint32_t abs_play_pos = abs(pos);
    uint8_t seconds, minutes, hours;
    uint32_t frames;

    double total_seconds = (double) abs_play_pos / (double) tl->proj->sample_rate;
    hours = total_seconds / 3600;
    minutes = (total_seconds - 3600 * hours) / 60;
    seconds = total_seconds - (3600 * hours) - (60 * minutes);
    frames = abs_play_pos - ((int) total_seconds * tl->proj->sample_rate);

    snprintf(dst, dstsize, "%c%02d:%02d:%02d:%05d", sign, hours, minutes, seconds, frames);
}


/* Called in two scenarios:
   1. timeline_set_play_position() (i.e., when playhead jumps)
   2. project_loop(), while play speed != 0
*/
void timeline_set_timecode(Timeline *tl)
{
    /* if (!proj) { */
    /*     fprintf(stderr, "Error: request to set timecode with no active project.\n"); */
    /*     exit(1); */
    /* } */

    /* Timeline *tl = proj->timelines[proj->active_tl_index]; */
    char sign = tl->play_pos_sframes < 0 ? '-' : '+';
    uint32_t abs_play_pos = abs(tl->play_pos_sframes);
    uint8_t seconds, minutes, hours;
    uint32_t frames;

    double total_seconds = (double) abs_play_pos / (double) tl->proj->sample_rate;
    hours = total_seconds / 3600;
    minutes = (total_seconds - 3600 * hours) / 60;
    seconds = total_seconds - (3600 * hours) - (60 * minutes);
    frames = abs_play_pos - ((int) total_seconds * tl->proj->sample_rate);

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
	automation_clear_cache(a);
	/* a->current = NULL; */
	/* a->ghost_valid = false; */
    }
}

/* Invalidates continuous-play-dependent caches.
   Use this any time a "jump" occurrs */
void timeline_set_play_position(Timeline *tl, int32_t abs_pos_sframes)
{
    MAIN_THREAD_ONLY("timeline_set_play_position");

    if (tl->play_pos_sframes == abs_pos_sframes) return;

    int32_t pos_diff = abs_pos_sframes - tl->play_pos_sframes;
    
    if (tl->proj->dragging && tl->num_grabbed_clips > 0) {
	timeline_cache_grabbed_clip_positions(tl);
    }

    tl->play_pos_sframes = abs_pos_sframes;
    tl->read_pos_sframes = abs_pos_sframes;
    int x = timeline_get_draw_x(tl, tl->play_pos_sframes);
    SDL_Rect *audio_rect = tl->proj->audio_rect;
    if (x < audio_rect->x || x > audio_rect->x + audio_rect->w) {
	int diff = x - (audio_rect->x + audio_rect->w / 2);
	tl->display_offset_sframes += timeline_get_abs_w_sframes(tl, diff);
    }
    for (uint8_t i=0; i<tl->num_tracks; i++) {
	track_handle_playhead_jump(tl->tracks[i]);
    }
    timeline_set_timecode(tl);
    for (int i=0; i<tl->num_tempo_tracks; i++) {	
	tempo_track_bar_beat_subdiv(tl->tempo_tracks[i], tl->play_pos_sframes, NULL, NULL, NULL, NULL, true);
    }
    if (tl->proj->dragging && tl->num_grabbed_clips > 0) {
	for (int i=0; i<tl->num_grabbed_clips; i++) {
	    tl->grabbed_clips[i]->pos_sframes += pos_diff;
	}
	timeline_push_grabbed_clip_move_event(tl);
    }
    timeline_reset(tl, false);

}



void timeline_move_play_position(Timeline *tl, int32_t move_by_sframes)
{
    RESTRICT_NOT_DSP("timeline_move_play_position");
    RESTRICT_NOT_MAIN("timeline_move_play_position");

    int64_t new_pos = (int64_t)tl->play_pos_sframes + move_by_sframes;
    /* fprintf(stderr, "NEW POS: %lld\n", new_pos); */
    if (new_pos > INT32_MAX || new_pos < INT32_MIN) {
	/* fprintf(stderr, "CMPS (to %d, %d) %d %d\n", INT32_MAX, INT32_MIN, new_pos > INT32_MAX, new_pos < INT32_MIN); */
	if (tl->proj->playing) transport_stop_playback();
	status_set_errstr("reached end of timeline");
    } else {
	tl->play_pos_sframes += move_by_sframes;
	clock_gettime(CLOCK_MONOTONIC, &tl->play_pos_moved_at);
	if (tl->proj->dragging) {
	    for (uint8_t i=0; i<tl->num_grabbed_clips; i++) {
		ClipRef *cr = tl->grabbed_clips[i];
		cr->pos_sframes += move_by_sframes;
		/* clipref_reset(cr); */
	    }
	}
    }
    tl->needs_redraw = true;

    /* tempo_track_bar_beat_subdiv(tl->tempo_tracks[0], tl->play_pos_sframes); */
}


void timeline_catchup(Timeline *tl)
{
    /* Timeline *tl = proj->timelines[proj->active_tl_index]; */
    int tl_draw_x;
    /* uint32_t move_by_sframes; */
    int catchup_w = TIMELINE_CATCHUP_W;
    if (tl->proj->audio_rect->w <= 0) {
	return;
    }
    /* while (catchup_w > proj->audio_rect->w / 2 && catchup_w > 10) { */
    /* 	catchup_w /= 2; */
    /* } */
    if ((tl_draw_x = timeline_get_draw_x(tl, tl->play_pos_sframes)) > tl->proj->audio_rect->x + tl->proj->audio_rect->w) {
	tl->display_offset_sframes = tl->play_pos_sframes - timeline_get_abs_w_sframes(tl, tl->proj->audio_rect->w - catchup_w);
	timeline_reset(tl, false);
    } else if (tl_draw_x < tl->proj->audio_rect->x) {
	tl->display_offset_sframes = tl->play_pos_sframes - timeline_get_abs_w_sframes(tl, catchup_w);
	timeline_reset(tl, false);
    }
}

int32_t timeline_get_play_pos_now(Timeline *tl)
{
    if (!tl->proj->playing) {
	return tl->play_pos_sframes;
    }
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    double elapsed_s = now.tv_sec + ((double)now.tv_nsec / 1e9) - tl->play_pos_moved_at.tv_sec - ((double)tl->play_pos_moved_at.tv_nsec / 1e9);
    return tl->play_pos_sframes + elapsed_s * tl->proj->sample_rate * tl->proj->play_speed;
}
