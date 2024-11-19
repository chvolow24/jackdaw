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
    tempo.c

    * Measure configuration
    * Drawing and arranging tempo tracks
    * Setting tempo and time signature
 *****************************************************************************************************************/

#include <stdarg.h>
#include "project.h"
#include "tempo.h"
#include "timeline.h"

extern Window *main_win;

static void tempo_segment_set_config(TempoSegment *s, int bpm, int num_beats, va_list subdivs)
{
    if (num_beats > MAX_BEATS_PER_BAR) {
	fprintf(stderr, "Error: num_beats exceeds maximum per bar (%d)\n", MAX_BEATS_PER_BAR);
	return;
    }
    
    s->cfg.bpm = bpm;
    s->cfg.num_beats = num_beats;
    
    double beat_dur = s->track->tl->proj->sample_rate * 60.0 / bpm;
    
    int min_subdivs = 0;
    for (int i=0; i<num_beats; i++) {
	s->cfg.beat_subdivs[i] = va_arg(subdivs, int);
	if (i == 0 || s->cfg.beat_subdivs[i] < min_subdivs) {
	    min_subdivs = s->cfg.beat_subdivs[i];
	}
    }
    double measure_dur = 0;
    for (int i=0; i<num_beats; i++) {
	measure_dur += beat_dur * s->cfg.beat_subdivs[i] / min_subdivs;
    }
    s->cfg.dur_sframes = (int32_t)(round(measure_dur));
    
}

/* Pass -1 to "num_measures" for infinity */
static TempoSegment *tempo_track_add_segment(TempoTrack *t, int32_t start_pos, int16_t num_measures, int bpm, int num_beats, ...)
{
    TempoSegment *s = calloc(1, sizeof(TempoSegment));
    s->track = t;
    /* s->num_measures = num_measures; */
    s->start_pos = start_pos;

    if (!t->segments) {
	t->segments = s;
	s->end_pos = s->start_pos;

    } else {
	TempoSegment *s_test = t->segments;
	while (s_test && s_test->end_pos < start_pos) {
	    if (!s_test->next) break;
	    s_test = s_test->next;
	}
	TempoSegment *next = s_test->next;
	s_test->next = s;
	s_test->end_pos = start_pos;
	s->next = next;
	if (next) {
	    next->prev = s;
	}
    }

    va_list ap;
    va_start(ap, num_beats);
    tempo_segment_set_config(s, bpm, num_beats, ap); 
    va_end(ap);
    /* t->num_segments++; */
    /* if (t->num_segments == 1) { */
    /* 	s->first_measure_index = 0; */
    /* } else { */

    /* } */
    return s;
}

TempoTrack *timeline_add_tempo_track(Timeline *tl)
{
    TempoTrack *t = calloc(1, sizeof(TempoTrack));
    t->tl = tl;
    tl->tempo_tracks[tl->num_tempo_tracks] = t;
    tl->num_tempo_tracks++;
    tempo_track_add_segment(t, 0, -1, 120, 4, 4, 4, 4, 4);
    return t;
}


void tempo_segment_draw(TempoSegment *s)
{
    int left_x = timeline_get_draw_x(s->start_pos);
    if (left_x > main_win->w_pix) {
	return;
    }
    

}

void tempo_track_draw(TempoTrack *t)
{
    TempoSegment *s = t->segments;

    while (s) {
	tempo_segment_draw(s);
	s = s->next;
    }

}
