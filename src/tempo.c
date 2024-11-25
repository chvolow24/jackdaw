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
#include "layout_xml.h"
#include "project.h"
#include "tempo.h"
#include "timeline.h"

extern Window *main_win;

enum beat_prominence {
    BP_MEASURE=0,
    BP_BEAT=1,
    BP_SUBDIV=2,
    BP_SUBDIV2=3,
};


static TempoSegment *tempo_track_get_segment_at_pos(TempoTrack *t, int32_t pos)
{
    TempoSegment *s = t->segments;
    while (s) {
	if (pos >= s->start_pos && (s->end_pos == s->start_pos || pos < s->end_pos)) {
	    break;
	}
	s = s->next;
    }
    return s;
}

static int32_t beat_pos(TempoSegment *s, int measure, int beat, int subdiv)
{
    int32_t pos = s->start_pos + measure * s->cfg.dur_sframes;
    for (int i=0; i<beat; i++) {
	pos += s->cfg.dur_sframes * s->cfg.beat_subdiv_lens[i] / s->cfg.num_atoms;
    }
    pos += s->cfg.dur_sframes * subdiv / s->cfg.num_atoms;
    return pos;
}

static void do_increment(TempoSegment *s, int *measure, int *beat, int *subdiv)
{
    if (*subdiv == s->cfg.beat_subdiv_lens[*beat] - 1) {
	if (*beat == s->cfg.num_beats - 1) {
	    *beat = 0;
	    *subdiv = 0;
	    (*measure)++;
	} else {
	    (*beat)++;
	    *subdiv = 0;
	}
    } else {
	(*subdiv)++;
    }

}

/* Stateful function, repeated calls to which will get the next beat or subdiv position on a tempo track */
void tempo_track_get_next_pos(TempoTrack *t, bool start, int32_t start_from, int32_t *pos, enum beat_prominence *bp)
{
    static TempoSegment *s;
    static int beat = 0;
    static int subdiv = 0;
    static int measure = 0;
    if (start) {
	s = tempo_track_get_segment_at_pos(t, start_from);
	int32_t current_pos = s->start_pos;
	measure = 0;
	beat = 0;
	subdiv = 0;
	while (1) {
	    current_pos = beat_pos(s, measure, beat, subdiv);
	    if (current_pos >= start_from) {
		*pos = current_pos;
		goto set_prominence_and_exit;
	    }

	    do_increment(s, &measure, &beat, &subdiv);
	}
    } else {
	do_increment(s, &measure, &beat, &subdiv);
	*pos = beat_pos(s, measure, beat, subdiv);
	if (s->start_pos != s->end_pos && *pos >= s->end_pos) {
	    s = s->next;
	    if (!s) {
		fprintf(stderr, "Fatal error: no tempo track segment where expected\n");
		exit(1);
	    }
	    *pos = s->start_pos;
	    measure = 0;
	    beat = 0;
	    subdiv = 0;
	    fprintf(stderr, "SWITCHED SEGMENT\n");
	    fprintf(stderr, "NEW SEGMENT ATOM DUR: %d\n", s->cfg.dur_sframes / s->cfg.num_atoms);
	}
    }
set_prominence_and_exit:
    if (subdiv == 0 && beat == 0) {
	*bp = BP_MEASURE;
    } else if (subdiv == 0 && beat != 0) {
	*bp = BP_BEAT;
    } else if (s->cfg.beat_subdiv_lens[beat] %2 == 0 && subdiv % 2 == 0) {
	*bp = BP_SUBDIV;
    } else {
	*bp = BP_SUBDIV2;
    }
    return;
}

static void tempo_segment_set_config(TempoSegment *s, int num_measures, int bpm, int num_beats, va_list subdivs)
{
    if (num_beats > MAX_BEATS_PER_BAR) {
	fprintf(stderr, "Error: num_beats exceeds maximum per bar (%d)\n", MAX_BEATS_PER_BAR);
	return;
    }
    
    s->cfg.bpm = bpm;
    s->cfg.num_beats = num_beats;
    
    double beat_dur = s->track->tl->proj->sample_rate * 60.0 / bpm;
    
    int min_subdiv_len_atoms = 0;
    for (int i=0; i<num_beats; i++) {
	int subdiv_len_atoms = va_arg(subdivs, int);
	s->cfg.beat_subdiv_lens[i] = subdiv_len_atoms;
	s->cfg.num_atoms += subdiv_len_atoms;
	if (i == 0 || s->cfg.beat_subdiv_lens[i] < min_subdiv_len_atoms) {
	    min_subdiv_len_atoms = s->cfg.beat_subdiv_lens[i];
	}
    }
    double measure_dur = 0;
    for (int i=0; i<num_beats; i++) {
	measure_dur += beat_dur * s->cfg.beat_subdiv_lens[i] / min_subdiv_len_atoms;
    }
    s->cfg.dur_sframes = (int32_t)(round(measure_dur));
    if (num_measures > 0 && s->next) {
	s->end_pos = s->start_pos + num_measures * s->cfg.dur_sframes;
    } else {
	s->end_pos = s->start_pos;
    }
}

/* Pass -1 to "num_measures" for infinity */
TempoSegment *tempo_track_add_segment(TempoTrack *t, int32_t start_pos, int16_t num_measures, int bpm, int num_beats, ...)
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
    tempo_segment_set_config(s, num_measures, bpm, num_beats, ap); 
    va_end(ap);

    if (s->next) {
	s->next->start_pos = s->end_pos;
    }
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

    Layout *lt = layout_read_xml_to_lt(tl->track_area, TEMPO_TRACK_LT_PATH);
    /* tempo_track_add_segment(t, 0, -1, 120, 4, 4, 4, 4, 4); */
    return t;
}



void tempo_track_draw(TempoTrack *tt)
{
    int32_t pos = tt->tl->display_offset_sframes;
    enum beat_prominence bp;
    tempo_track_get_next_pos(tt, true, pos, &pos, &bp);
    int x = timeline_get_draw_x(pos);
    fprintf(stderr, "First draw x: %d;, bp: %d\n", x, bp);
    while (x < main_win->w_pix) {
	tempo_track_get_next_pos(tt, false, 0, &pos, &bp);
	x = timeline_get_draw_x(pos);
	fprintf(stderr, "Next draw x: %d; bp: %d\n", x, bp);
    }
	

}

void tempo_segment_fprint(FILE *f, TempoSegment *s)
{
    fprintf(f, "\nSegment start/end: %d-%d\n", s->start_pos, s->end_pos);
    fprintf(f, "Segment tempo (bpm): %d\n", s->cfg.bpm);
    fprintf(f, "\tCfg beats: %d\n", s->cfg.num_beats);
    fprintf(f, "\tCfg beat subdiv lengths:\n");
    for (int i=0; i<s->cfg.num_beats; i++) {
	fprintf(f, "\t\t%d: %d\n", i, s->cfg.beat_subdiv_lens[i]);
    }
    fprintf(f, "\tCfg num atoms: %d\n", s->cfg.num_atoms);
    fprintf(f, "\tCfg measure dur: %d\n", s->cfg.dur_sframes);
    fprintf(f, "\tCfg atom dur: %d\n", s->cfg.dur_sframes / s->cfg.num_atoms);

}

void tempo_track_fprint(FILE *f, TempoTrack *tt)
{
    fprintf(f, "\nTEMPO TRACK\n");
    TempoSegment *s = tt->segments;
    while (s) {
	fprintf(f, "SEGMENT:\n");
	tempo_segment_fprint(f, s);
	s = s->next;
    }
}
