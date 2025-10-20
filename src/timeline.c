/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

/*****************************************************************************************************************
    timeline.c

    * Functions related to timeline positions; others in project.c
    * Translate between draw coordinates and absolute positions (time values) within the timeline
 *****************************************************************************************************************/

#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include "consts.h"
#include "audio_clip.h"
#include "clipref.h"
#include "grab.h"
#include "midi_io.h"
#include "piano_roll.h"
#include "project.h"
#include "session_endpoint_ops.h"
#include "session.h"
#include "tempo.h"
#include "thread_safety.h"
#include "timeline.h"
#include "timeview.h"
#include "transport.h"

#define MAX_SFPP 80000

#define TIMELINE_CATCHUP_W (main_win->w_pix / 2)

/* extern Project *proj;q */
extern Window *main_win;
/* extern pthread_t MAIN_THREAD_ID; */
/* extern pthread_t DSP_THREAD_ID; */

/* int timeline_get_draw_x(int32_t abs_x); */

/* Get the timeline position value -- in sample frames -- from a draw x coordinate  */
int32_t timeline_get_abspos_sframes(Timeline *tl, int draw_x)
{
    return timeview_get_pos_sframes(&tl->timeview, draw_x);
    /* Session *session = session_get(); */
    /* /\* Timeline *tl = ACTIVE_TL; *\/ */
    /* return (draw_x - session->gui.audio_rect->x) * tl->sample_frames_per_pixel + tl->display_offset_sframes; */
}

/* Get the current draw x coordinate for a given timeline offset value (sample frames) */
int timeline_get_draw_x(Timeline *tl, int32_t abs_x)
{
    return timeview_get_draw_x(&tl->timeview, abs_x);
    /* Timeline *tl = ACTIVE_TL; */
    /* Session *session = session_get(); */
    /* if (tl->sample_frames_per_pixel != 0) { */
    /*     double precise = (double)session->gui.audio_rect->x + ((double)abs_x - (double)tl->display_offset_sframes) / (double)tl->sample_frames_per_pixel; */
    /*     return (int) round(precise); */
    /* } else { */
    /*     fprintf(stderr, "Error: proj tl sfpp value 0\n"); */
    /* 	exit(1); */
    /*     /\* return 0; *\/ */
    /* } */
}

/* Get a draw width from a given length in sample frames */
int timeline_get_draw_w(Timeline *tl, int32_t abs_w)
{
    return timeview_get_draw_w(&tl->timeview, abs_w);
    /* Timeline *tl = ACTIVE_TL; */
    /* if (tl->sample_frames_per_pixel != 0) { */
    /*     return abs_w / tl->sample_frames_per_pixel; */
    /* } else { */
    /*     fprintf(stderr, "Error: proj tl sfpp value 0\n"); */
    /*     return 0; */
    /* } */
}

/* static double timeline_get_draw_w_precise(Timeline *tl, int32_t abs_w) */
/* { */
/*     return timeview_get_draw_w_precise(&tl->timeview, abs_w); */
/*     /\* /\\* Timeline *tl = ACTIVE_TL; *\\/ *\/ */
/*     /\* if (tl->sample_frames_per_pixel != 0) { *\/ */
/*     /\* 	return (double) abs_w / tl->sample_frames_per_pixel; *\/ */
/*     /\* } *\/ */
/*     /\* return 0.0; *\/ */
/* } */
 
/* Get a length in sample frames from a given draw width */
int32_t timeline_get_abs_w_sframes(Timeline *tl, int draw_w)
{
    /* Timeline *tl = ACTIVE_TL; */
    /* return draw_w * tl->sample_frames_per_pixel; */
    return timeview_get_w_sframes(&tl->timeview, draw_w);
}

void timeline_scroll_horiz(Timeline *tl, int scroll_by)
{
    /* Timeline *tl = ACTIVE_TL; */
    /* int32_t new_offset = tl->display_offset_sframes + timeline_get_abs_w_sframes(tl, scroll_by); */
    /* tl->display_offset_sframes = new_offset; */
    timeview_scroll_horiz(&tl->timeview, scroll_by);
    timeline_reset(tl, false);
}

double timeline_get_leftmost_seconds(Timeline *tl)
{
    /* Timeline *tl = ACTIVE_TL; */
    /* return (double) tl->display_offset_sframes / tl->proj->sample_rate; */
    return (double)(tl->timeview.offset_left_sframes) / tl->proj->sample_rate;
}

double timeline_get_second_w(Timeline *tl)
{
    return timeview_get_draw_w_precise(&tl->timeview, tl->proj->sample_rate);
    /* return timeline_get_draw_w_precise(tl, tl->proj->sample_rate); */
    /* return ret <= 0 ? 1 : ret; */
}

double timeline_first_second_tick_x(Timeline *tl, int *first_second)
{
    double lms = timeline_get_leftmost_seconds(tl);
    double dec = lms - floor(lms);
    *first_second = (int)floor(lms) + 1;
    return timeline_get_second_w(tl) * (1 - dec);
    /* return 1 - (timeline_get_second_w() * dec) + session->gui.audio_rect->x; */
}

void timeline_rescale(Timeline *tl, double sfpp_scale_factor, bool on_mouse)
{
    if (sfpp_scale_factor == 1.0f) return;
    timeview_rescale(&tl->timeview, sfpp_scale_factor, on_mouse, main_win->mousep);
    /* /\* Timeline *tl = ACTIVE_TL; *\/ */
    /* int32_t center_abs_pos = 0; */
    /* if (on_mouse) { */
    /* 	center_abs_pos = timeline_get_abspos_sframes(tl, main_win->mousep.x); */
    /* } else { */
    /* 	center_abs_pos = tl->play_pos_sframes; */
    /* } */
    /* if (sfpp_scale_factor == 0) { */
    /*     fprintf(stderr, "Warning! Scale factor 0 in rescale_timeline\n"); */
    /*     return; */
    /* } */
    /* int init_draw_pos = timeline_get_draw_x(tl, center_abs_pos); */
    /* double new_sfpp = tl->sample_frames_per_pixel / sfpp_scale_factor; */
    /* if (new_sfpp < 1 || new_sfpp > MAX_SFPP) { */
    /*     return; */
    /* } */
    /* if ((int)new_sfpp == tl->sample_frames_per_pixel) { */
    /*     tl->sample_frames_per_pixel += sfpp_scale_factor <= 1.0f ? 1 : -1; */
    /* 	if (tl->sample_frames_per_pixel < 1) tl->sample_frames_per_pixel = 1; */
    /* } else { */
    /*     tl->sample_frames_per_pixel = new_sfpp; */
    /* } */

    /* int new_draw_pos = timeline_get_draw_x(tl, center_abs_pos); */
    /* int offset_draw_delta = new_draw_pos - init_draw_pos; */
    /* tl->display_offset_sframes += (timeline_get_abs_w_sframes(tl, offset_draw_delta)); */

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

    /* Timeline *tl = ACTIVE_TL; */
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
    Session *session = session_get();
    if (session->gui.timecode_tb) {
	textbox_reset_full(session->gui.timecode_tb);
    }
}
static void track_handle_playhead_jump(Track *track)
{
    for (uint8_t i =0; i<track->num_automations; i++) {
	Automation *a = track->automations[i];
	automation_clear_cache(a);
    }
    /* eq_clear(&track->eq); */
}

/* Invalidates continuous-play-dependent caches.
   Use this any time a "jump" occurs */
void timeline_set_play_position(Timeline *tl, int32_t abs_pos_sframes, bool move_grabbed_clips)
{
    MAIN_THREAD_ONLY("timeline_set_play_position");
    Session *session = session_get();

    /* if (tl->play_pos_sframes == abs_pos_sframes) return; */
    
    int32_t pos_diff = abs_pos_sframes - tl->play_pos_sframes;

    if (move_grabbed_clips) {
	if (session->piano_roll && session->dragging) {
	    piano_roll_start_moving();
	} else if (session->dragging) {
	    timeline_cache_grabbed_clip_positions(tl);
	}
    }

    tl->play_pos_sframes = abs_pos_sframes;
    tl->read_pos_sframes = abs_pos_sframes;
    int x = timeline_get_draw_x(tl, tl->play_pos_sframes);
    SDL_Rect *audio_rect = session->gui.audio_rect;
    if (x < audio_rect->x || x > audio_rect->x + audio_rect->w) {
	int diff = x - (audio_rect->x + audio_rect->w / 2);
	tl->timeview.offset_left_sframes += timeline_get_abs_w_sframes(tl, diff);
    }
    for (uint8_t i=0; i<tl->num_tracks; i++) {
	track_handle_playhead_jump(tl->tracks[i]);
    }
    timeline_set_timecode(tl);
    for (int i=0; i<tl->num_click_tracks; i++) {	
	click_track_bar_beat_subdiv(tl->click_tracks[i], tl->play_pos_sframes, NULL, NULL, NULL, NULL, true);
    }
    if (move_grabbed_clips && session->dragging) {
	if (session->piano_roll) {
	    piano_roll_grabbed_notes_move(pos_diff);
	    piano_roll_stop_moving();
	} else {
	    timeline_grabbed_clips_move(tl, pos_diff);
	    timeline_push_grabbed_clip_move_event(tl);
	}
    }

    if (session->playback.lock_view_to_playhead) {
	timeview_scroll_sframes(&tl->timeview, pos_diff);
	tl->needs_reset = true;
    }


    timeline_flush_unclosed_midi_notes();
    timeline_reset(tl, false);
}



void timeline_move_play_position(Timeline *tl, int32_t move_by_sframes)
{
    RESTRICT_NOT_DSP("timeline_move_play_position");
    RESTRICT_NOT_MAIN("timeline_move_play_position");
    Session *session = session_get();

    static const int32_t end_tl_buffer = DEFAULT_SAMPLE_RATE * 30; /* 30 seconds at dmax sample rate*/
    
    int64_t new_pos = (int64_t)tl->play_pos_sframes + move_by_sframes;
    if (session->playback.loop_play) {
	int32_t loop_len = tl->out_mark_sframes - tl->in_mark_sframes;
	if (loop_len > 0) {
	    int64_t remainder = new_pos - tl->out_mark_sframes;
	    if (remainder > 0) {
		if (remainder > loop_len) remainder = 0;
		new_pos = tl->in_mark_sframes + remainder;
	    }
	}
    }
    if (new_pos > INT32_MAX - end_tl_buffer || new_pos < INT32_MIN + end_tl_buffer) {
	/* fprintf(stderr, "CMPS (to %d, %d) %d %d\n", INT32_MAX, INT32_MIN, new_pos > INT32_MAX, new_pos < INT32_MIN); */
	if (session->playback.playing) transport_stop_playback();
	status_set_errstr("reached end of timeline");
    } else {
	tl->play_pos_sframes = new_pos;
	clock_gettime(CLOCK_MONOTONIC, &tl->play_pos_moved_at);
	if (session->dragging) {
	    timeline_grabbed_clips_move(tl, move_by_sframes);
	}
    }
    if (session->playback.lock_view_to_playhead) {
	timeview_scroll_sframes(&tl->timeview, move_by_sframes);
	tl->needs_reset = true;
    }
    tl->needs_redraw = true;
}


void timeline_catchup(Timeline *tl)
{
    /* Timeline *tl = ACTIVE_TL; */
    int tl_draw_x;
    /* uint32_t move_by_sframes; */
    int catchup_w = TIMELINE_CATCHUP_W;
    Session *session = session_get();
    if (session->gui.audio_rect->w <= 0) {
	return;
    }
    /* while (catchup_w > session->gui.audio_rect->w / 2 && catchup_w > 10) { */
    /* 	catchup_w /= 2; */
    /* } */
    if ((tl_draw_x = timeline_get_draw_x(tl, tl->play_pos_sframes)) > session->gui.audio_rect->x + session->gui.audio_rect->w) {
	tl->timeview.offset_left_sframes = tl->play_pos_sframes - timeline_get_abs_w_sframes(tl, session->gui.audio_rect->w - catchup_w);
	timeline_reset(tl, false);
    }
    else if (tl_draw_x < session->gui.audio_rect->x) {
	tl->timeview.offset_left_sframes = tl->play_pos_sframes - timeline_get_abs_w_sframes(tl, catchup_w);
	timeline_reset(tl, false);
    }
}

int32_t timeline_get_play_pos_now(Timeline *tl)
{
    Session *session = session_get();
    if (!session->playback.playing) {
	return tl->play_pos_sframes;
    }
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    double elapsed_s = now.tv_sec + ((double)now.tv_nsec / 1e9) - tl->play_pos_moved_at.tv_sec - ((double)tl->play_pos_moved_at.tv_nsec / 1e9);
    return tl->play_pos_sframes + elapsed_s * tl->proj->sample_rate * session->playback.play_speed;
}
