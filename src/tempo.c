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



static void tempo_segment_set_config(TempoSegment *s, int bpm, int num_beats, ...)
{
    if (num_beats > MAX_BEATS_PER_BAR) {
	fprintf(stderr, "Error: num_beats exceeds maximum per bar (%d)\n", MAX_BEATS_PER_BAR);
	return;
    }
    
    s->cfg.bpm = bpm;
    s->cfg.num_beats = num_beats;
    
    double beat_dur = s->track->tl->proj->sample_rate * 60.0 / bpm;
    
    va_list ap;
    va_start(ap, num_beats);
    int min_subdivs = 0;
    for (int i=0; i<num_beats; i++) {
	s->cfg.beat_subdivs[i] = va_arg(ap, int);
	if (i == 0 || s->cfg.beat_subdivs[i] < min_subdivs) {
	    min_subdivs = s->cfg.beat_subdivs[i];
	}
    }
    double measure_dur = 0;
    for (int i=0; i<num_beats; i++) {
	measure_dur += beat_dur * s->cfg.beat_subdivs[i] / min_subdivs;
    }
    s->cfg.dur_sframes = (int32_t)(round(measure_dur));
    va_end(ap);
    
}

static TempoSegment *tempo_track_add_segment(TempoTrack *t, int32_t start_pos)
{
    TempoSegment *s = calloc(1, sizeof(TempoSegment));
    s->track = t;
    s->start_pos = start_pos;
}

TempoTrack *timeline_add_tempo_track(Timeline *tl)
{
    TempoTrack *t = calloc(1, sizeof(TempoTrack));
    t->tl = tl;
    tl->tempo_tracks[tl->num_tempo_tracks] = t;
    tl->num_tempo_tracks++;
    return t;
}

