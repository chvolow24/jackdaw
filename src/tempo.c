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
#include "assets.h"
#include "color.h"
#include "components.h"
#include "geometry.h"
#include "layout_xml.h"
#include "portability.h"
#include "project.h"
#include "tempo.h"
#include "timeline.h"
#include "wav.h"

extern Window *main_win;
extern SDL_Color color_global_tempo_track;

enum beat_prominence {
    BP_SEGMENT=0,
    BP_MEASURE=1,
    BP_BEAT=2,
    BP_SUBDIV=3,
    BP_SUBDIV2=4,
};

void project_init_metronomes(Project *proj)
{
    Metronome *m = &proj->metronomes[0];
    m->name = "standard";
    
    float *L, *R;    
    wav_load(proj, METRONOME_STD_HIGH_PATH, &L, &R);
    free(R);

    m->buffers[0] = L;
    
    wav_load(proj, METRONOME_STD_LOW_PATH, &L, &R);
    free(R);

    m->buffers[1] = L;    
}

static TempoSegment *tempo_track_get_segment_at_pos(TempoTrack *t, int32_t pos)
{
    static JDAW_THREAD_LOCAL TempoSegment *s = NULL;
    if (!s) s = t->segments;
    
    while (1) {
	if (pos < s->start_pos) {
	    if (s->prev) {
		s = s->prev;
	    } else {
		return s;
	    }
	} else {
	    if (s->end_pos == s->start_pos || pos < s->end_pos) {
		return s;
	    } else {
		s = s->next;
	    }
	}
    }
    /* while (s) { */
    /* 	if (pos < s->start_pos) break; */
    /* 	if (!s->next || (pos >= s->start_pos && (s->end_pos == s->start_pos || pos < s->end_pos))) { */
    /* 	    break; */
    /* 	} */
    /* 	s = s->next; */
    /* } */
    return s;
}

static int32_t get_beat_pos(TempoSegment *s, int measure, int beat, int subdiv)
{
    int32_t pos = s->start_pos + (measure - s->first_measure_index) * s->cfg.dur_sframes;
    /* fprintf(stderr, "\t\tstart pos: %d; w/measure: %d\n", s->start_pos, pos); */
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

static void do_decrement(TempoSegment *s, int *measure, int *beat, int *subdiv)
{
    if (*subdiv > 0) {
	(*subdiv)--;
    } else if (*beat > 0) {
	(*beat)--;
	(*subdiv) = s->cfg.beat_subdiv_lens[*beat] - 1;
    } else if (*measure > 0) {
	(*measure)--;
	(*beat) = s->cfg.num_beats - 1;
	(*subdiv) = s->cfg.beat_subdiv_lens[*beat] - 1;
    }
}

/* Stateful function, repeated calls to which will get the next beat or subdiv position on a tempo track */
static bool tempo_track_get_next_pos(TempoTrack *t, bool start, int32_t start_from, int32_t *pos, enum beat_prominence *bp)
{
    static JDAW_THREAD_LOCAL TempoSegment *s;
    static JDAW_THREAD_LOCAL int beat = 0;
    static JDAW_THREAD_LOCAL int subdiv = 0;
    static JDAW_THREAD_LOCAL int measure = 0;
    if (start) {
	s = tempo_track_get_segment_at_pos(t, start_from);
	int32_t current_pos = s->start_pos;
	measure = s->first_measure_index;
	beat = 0;
	subdiv = 0;
	while (1) {
	    current_pos = get_beat_pos(s, measure, beat, subdiv);
	    if (current_pos >= start_from) {
		*pos = current_pos;
		goto set_prominence_and_exit;
	    }

	    do_increment(s, &measure, &beat, &subdiv);
	}
    } else {
	do_increment(s, &measure, &beat, &subdiv);
	*pos = get_beat_pos(s, measure, beat, subdiv);
	if (s->start_pos != s->end_pos && *pos >= s->end_pos) {
	    s = s->next;
	    if (!s) {
		fprintf(stderr, "Fatal error: no tempo track segment where expected\n");
		exit(1);
	    }
	    *pos = s->start_pos;
	    measure = s->first_measure_index;
	    beat = 0;
	    subdiv = 0;
	}
    }
set_prominence_and_exit:
    if (measure == s->first_measure_index && beat == 0 && subdiv == 0) {
	*bp = BP_SEGMENT;
    } else if (subdiv == 0 && beat == 0) {
	*bp = BP_MEASURE;
    } else if (subdiv == 0 && beat != 0) {
	*bp = BP_BEAT;
    } else if (s->cfg.beat_subdiv_lens[beat] %2 == 0 && subdiv % 2 == 0) {
	*bp = BP_SUBDIV;
    } else {
	*bp = BP_SUBDIV2;
    }
    return true;
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


static void tempo_segment_set_end_pos(TempoSegment *s, int32_t new_end_pos)
{
    int32_t segment_dur = new_end_pos - s->start_pos;
    double num_measures = (double)segment_dur / s->cfg.dur_sframes;
    s->num_measures = floor(1.0 + num_measures);
    s->end_pos = new_end_pos;
    while (s) {
	if (s->prev) {
	    s->first_measure_index = s->prev->first_measure_index + s->prev->num_measures;
	} else {
	    s->first_measure_index = 0;
	}
	s = s->next;
    }
}

/* Pass -1 to "num_measures" for infinity */
TempoSegment *tempo_track_add_segment(TempoTrack *t, int32_t start_pos, int16_t num_measures, int bpm, int num_beats, ...)
{
    TempoSegment *s = calloc(1, sizeof(TempoSegment));
    s->track = t;
    s->num_measures = num_measures;
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
	s->prev = s_test;
	/* s_test->end_pos = start_pos; */
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
    if (s->prev) 
	tempo_segment_set_end_pos(s->prev, start_pos);
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
    t->metronome = &tl->proj->metronomes[0];
    tl->tempo_tracks[tl->num_tempo_tracks] = t;
    tl->num_tempo_tracks++;

    Layout *tempo_tracks_area = layout_get_child_by_name_recursive(tl->layout, "tempo_tracks_area");
    Layout *lt = layout_read_xml_to_lt(tempo_tracks_area, TEMPO_TRACK_LT_PATH);
    t->layout = lt;
    layout_size_to_fit_children_v(tempo_tracks_area, true, 0);
    layout_reset(tl->layout);
    tl->needs_redraw = true;
    if (tl->num_tempo_tracks == 1) {
	tempo_track_add_segment(t, 0, -1, 60, 4, 4, 4, 4, 4);
	tempo_track_add_segment(t, 96000 * 3, -1, 120, 3, 3, 3, 3);
	tempo_track_add_segment(t, 96000 * 6, -1, 130, 5, 3, 3, 2, 2, 2);
	tempo_track_add_segment(t, 96000 * 9, -1, 900, 4, 4, 4, 4);
    } else if (tl->num_tempo_tracks == 2) {
	tempo_track_add_segment(t, 0, -1, 120, 3, 3, 3, 2);
    } else if (tl->num_tempo_tracks == 3) {
	tempo_track_add_segment(t, 0, -1, 130, 4, 4, 4, 4, 4);
    }
    return t;
}

void tempo_track_fill_metronome_buffer(TempoTrack *tt, float *L, float *R, int32_t start_from)
{
    static TempoSegment *s = NULL;
    
    if (!s) {
	s = tempo_track_get_segment_at_pos(tt, start_from);
    } else {
	while (s->next && start_from < s->start_pos) {
	    s = s->next;
	}
    }
}

void tempo_track_draw(TempoTrack *tt)
{
    SDL_Rect ttr = tt->layout->rect;
    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(color_global_tempo_track));
    SDL_RenderFillRect(main_win->rend, &ttr);
    static SDL_Color line_colors[] =  {
	{255, 250, 125, 255},
	{255, 255, 255, 255},
	{170, 170, 170, 255},
	{130, 130, 130, 255},
	{100, 100, 100, 255}
    };

    int32_t pos = tt->tl->display_offset_sframes;
    enum beat_prominence bp;
    tempo_track_get_next_pos(tt, true, pos, &pos, &bp);
    int x = timeline_get_draw_x(pos);

    int main_top_y = tt->layout->rect.y;
    int bttm_y = main_top_y + tt->layout->rect.h;
    int h = tt->layout->rect.h;

    int top_y = main_top_y + h * (int)bp / 5;
    double dpi = main_win->dpi_scale_factor;
    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(line_colors[(int)bp]));
    if (bp == BP_SEGMENT) {
	SDL_Rect seg = {x - dpi, top_y, dpi * 2, h};
	SDL_RenderFillRect(main_win->rend, &seg);

	/* SDL_RenderDrawLine(main_win->rend, x-dpi, top_y, x-dpi, bttm_y); */
	/* SDL_RenderDrawLine(main_win->rend, x+dpi, top_y, x+dpi, bttm_y); */
    } else {
	SDL_RenderDrawLine(main_win->rend, x, top_y, x, bttm_y);
    }
    /* SDL_RenderDrawLine(main_win->rend, x, top_y, x, bttm_y); */

    while (x < main_win->w_pix) {
	tempo_track_get_next_pos(tt, false, 0, &pos, &bp);
	x = timeline_get_draw_x(pos);
	top_y = main_top_y + h * (int)bp / 5;
	/* top_y = main_top_y + h / (1 + (int)bp); */
	SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(line_colors[(int)bp]));
	if (bp == BP_SEGMENT) {
	    SDL_Rect seg = {x - dpi, top_y, dpi * 2, h};
	    SDL_RenderFillRect(main_win->rend, &seg);

	    /* double dpi = main_win->dpi_scale_factor; */
	    /* SDL_RenderDrawLine(main_win->rend, x-dpi, top_y, x-dpi, bttm_y); */
	    /* SDL_RenderDrawLine(main_win->rend, x+dpi, top_y, x+dpi, bttm_y); */
	} else {
	    SDL_RenderDrawLine(main_win->rend, x, top_y, x, bttm_y);
	}
    }
    SDL_SetRenderDrawColor(main_win->rend, 100, 100, 100, 255);
    SDL_RenderDrawRect(main_win->rend, &ttr);	
}

void tempo_track_bar_beat_subdiv(TempoTrack *tt, int32_t pos)
{
    TempoSegment *s = tempo_track_get_segment_at_pos(tt, pos);

    /* int measure = s->first_measure_index; */
    /* int32_t test_pos = s->start_pos; */
    /* while (test_pos + s->cfg.dur_sframes < pos) { */
    /* 	test_pos += s->cfg.dur_sframes; */
    /* 	measure++; */
    /* } */

    /* int beat = 0; */
    /* int subdiv = 0; */
    int measure = s->first_measure_index;
    int beat = 0;
    int subdiv = 0;
    int32_t prev_pos = 0;
    int32_t beat_pos = 0;
    while (1) {
	beat_pos = get_beat_pos(s, measure, beat, subdiv);
	if (beat_pos >= pos) {
	    break;
	}
	do_increment(s, &measure, &beat, &subdiv);
	prev_pos = beat_pos;
    }
    do_decrement(s, &measure, &beat, &subdiv);
    /* fprintf(stderr, "AT %d:\t%d, %d, %d\n", pos, measure, beat, subdiv); */
    fprintf(stderr, "Copying at most %d chars\n", TEMPO_POS_STR_LEN);
    int32_t samples = pos - prev_pos;
    static const int sample_str_len = 7;
    char sample_str[sample_str_len];
    if (samples < -99999) {
	snprintf(sample_str, sample_str_len, "%s", "-∞");
    } else if (samples > 999999) {
	snprintf(sample_str, sample_str_len, "%s", "∞");
    } else {
	snprintf(sample_str, sample_str_len, "%d", samples);
    }
    snprintf(tt->pos_str, TEMPO_POS_STR_LEN, "m%d.%d.%d:%s", measure + 1, beat + 1, subdiv + 1, sample_str);
    fprintf(stderr, "%s\n", tt->pos_str);
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
