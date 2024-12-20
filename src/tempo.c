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

#include "thread_safety.h"

/* #include <stdarg.h> */
#include "assets.h"
#include "color.h"
#include "components.h"
#include "geometry.h"
#include "layout.h"
#include "layout_xml.h"
#include "portability.h"
#include "project.h"
#include "tempo.h"
#include "test.h"
#include "timeline.h"
#include "user_event.h"
#include "wav.h"

extern Window *main_win;
extern Project *proj;
extern SDL_Color color_global_tempo_track;
extern SDL_Color color_global_black;
extern SDL_Color color_global_white;
extern SDL_Color color_global_quickref_button_blue;
extern SDL_Color control_bar_bckgrnd;
extern SDL_Color mute_red;
extern SDL_Color color_global_play_green;

extern pthread_t MAIN_THREAD_ID;


enum beat_prominence {
    BP_SEGMENT=0,
    BP_MEASURE=1,
    BP_BEAT=2,
    BP_SUBDIV=3,
    BP_SUBDIV2=4,
    BP_NONE=5
};

void project_init_metronomes(Project *proj)
{
    Metronome *m = &proj->metronomes[0];
    m->name = "standard";

    float *L, *R;    
    int32_t buf_len = wav_load(proj, METRONOME_STD_HIGH_PATH, &L, &R);
    if (buf_len == 0) {
	fprintf(stderr, "Error: unable to load metronome buffer at %s\n", METRONOME_STD_LOW_PATH);
	exit(1);
    }

    free(R);

    m->buffers[0] = L;
    m->buf_lens[0] = buf_len;
    
    buf_len = wav_load(proj, METRONOME_STD_LOW_PATH, &L, &R);
    if (buf_len == 0) {
	fprintf(stderr, "Error: unable to load metronome buffer at %s\n", METRONOME_STD_LOW_PATH);
	exit(1);
    }
    free(R);

    m->buffers[1] = L;
    m->buf_lens[1] = buf_len;
}

static TempoSegment *tempo_track_get_segment_at_pos(TempoTrack *t, int32_t pos)
{
    TempoSegment *s = t->segments;
    if (!s) return NULL;
    
    /* THREAD SAFETY ISSUE: track may exist before segment assigned */
    while (1) {
	if (pos < s->start_pos) {
	    if (s->prev) {
		s = s->prev;
	    } else {
		/* fprintf(stderr, "Returning seg %d-%d (POS %d)\n", s->start_pos, s->end_pos, pos); */
		return s;
	    }
	} else {
	    if (s->end_pos == s->start_pos || pos < s->end_pos) {
		/* fprintf(stderr, "Returning seg %d-%d (POS %d)\n", s->start_pos, s->end_pos, pos); */
		return s;
	    } else {
		s = s->next;
	    }
	}
    }
    /* fprintf(stderr, "Returning seg %d-%d (POS %d)\n", s->start_pos, s->end_pos, pos); */
    return s;
}

static int32_t get_beat_pos(TempoSegment *s, int measure, int beat, int subdiv)
{
    
    int32_t pos = s->start_pos + (measure - s->first_measure_index) * s->cfg.dur_sframes;
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
    } else {
	(*measure)--;
	(*beat) = s->cfg.num_beats - 1;
	(*subdiv) = s->cfg.beat_subdiv_lens[*beat] - 1;
    }
}


static void set_beat_prominence(TempoSegment *s, enum beat_prominence *bp, int measure, int beat, int subdiv)
{
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
	/* If segment is not last */
	if (s->next && *pos >= s->end_pos) {
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
    set_beat_prominence(s, bp, measure, beat, subdiv);
    /* if (measure == s->first_measure_index && beat == 0 && subdiv == 0) { */
    /* 	*bp = BP_SEGMENT; */
    /* } else if (subdiv == 0 && beat == 0) { */
    /* 	*bp = BP_MEASURE; */
    /* } else if (subdiv == 0 && beat != 0) { */
    /* 	*bp = BP_BEAT; */
    /* } else if (s->cfg.beat_subdiv_lens[beat] %2 == 0 && subdiv % 2 == 0) { */
    /* 	*bp = BP_SUBDIV; */
    /* } else { */
    /* 	*bp = BP_SUBDIV2; */
    /* } */
    return true;
}

static void tempo_segment_set_end_pos(TempoSegment *s, int32_t new_end_pos);

static void tempo_segment_set_config(TempoSegment *s, int num_measures, int bpm, uint8_t num_beats, uint8_t *subdivs)
{
    if (num_beats > MAX_BEATS_PER_BAR) {
	fprintf(stderr, "Error: num_beats exceeds maximum per bar (%d)\n", MAX_BEATS_PER_BAR);
	return;
    }
    s->cfg.bpm = bpm;
    s->cfg.num_beats = num_beats;

    double beat_dur = s->track->tl->proj->sample_rate * 60.0 / bpm;
    
    int min_subdiv_len_atoms = 0;
    s->cfg.num_atoms = 0;
    for (int i=0; i<num_beats; i++) {
	int subdiv_len_atoms = subdivs[i];
	/* int subdiv_len_atoms = va_arg(subdivs, int); */
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
    s->cfg.atom_dur_approx = measure_dur / s->cfg.num_atoms;
    if (num_measures > 0 && s->next) {
	s->end_pos = s->start_pos + num_measures * s->cfg.dur_sframes;
    } else {
	s->end_pos = s->start_pos;
    }
    if (s->next) {
	tempo_segment_set_end_pos(s, s->next->start_pos);
    }
}

/* Calculates the new segment dur in measures and sets the first measure index of the next segment */
static void tempo_segment_set_end_pos(TempoSegment *s, int32_t new_end_pos)
{
    /* fprintf(stderr, "SETTING end pos on segment with deets\n"); */
    /* tempo_segment_fprint(stderr, s); */
    int32_t segment_dur = new_end_pos - s->start_pos;
    double num_measures = (double)segment_dur / s->cfg.dur_sframes;
    s->num_measures = floor(0.9999999 + num_measures);
    s->end_pos = new_end_pos;
    while (s->next) {
	/* int32_t segment_dur = new_end_pos - s->start_pos; */
	/* double num_measures = (double)segment_dur / s->cfg.dur_sframes; */
	/* /\* fprintf(stderr, "Num measures in double is: %f\n", num_measures); *\/ */
	/* s->num_measures = floor(0.999999 + num_measures); */
	/* s->end_pos = new_end_pos; */
	/* /\* fprintf(stderr, "Num measures in int is: %d\n", s->num_measures); *\/ */
    
	s->next->first_measure_index = s->first_measure_index+ s->num_measures;
	s = s->next;
	/* fprintf(stderr, "First measure index of next one is %d + %d\n", s->first_measure_index, s->num_measures); */
    }
}

TempoSegment *tempo_track_add_segment(TempoTrack *t, int32_t start_pos, int16_t num_measures, int bpm, uint8_t num_beats, uint8_t *subdiv_lens)
{
    /* fprintf(stderr, "\n\n\nADDING SEGMENT TO TEMPO TRACK, start: %d, num measures: %d\n", start_pos, num_measures); */
    TempoSegment *s = calloc(1, sizeof(TempoSegment));
    s->track = t;
    s->num_measures = num_measures;
    s->start_pos = start_pos;

    if (!t->segments) {
	/* fprintf(stderr, "This is the first and I'm adding it\n"); */
	t->segments = s;
	s->end_pos = s->start_pos;
	s->first_measure_index = BARS_FOR_NOTHING * -1;
	goto set_config_and_exit;
    }
    TempoSegment *interrupt = tempo_track_get_segment_at_pos(t, start_pos);
    if (interrupt->start_pos == start_pos) {
	fprintf(stderr, "Error: cannot insert segment at existing segment loc\n");
	return NULL;
    }
    if (!interrupt->next) {
	interrupt->next = s;
	s->prev = interrupt;
	tempo_segment_set_end_pos(interrupt, start_pos);
    } else {
	s->next = interrupt->next;
	interrupt->next->prev = s;
	interrupt->next = s;
	s->prev = interrupt;
	tempo_segment_set_config(s, num_measures, bpm, num_beats, subdiv_lens); 
	tempo_segment_set_end_pos(interrupt, start_pos);
	tempo_segment_set_end_pos(s, s->next->start_pos);
	return s;
	goto set_config_and_exit;
	/* fprintf(stderr, "Error: cannot insert segment in the middle\n"); */
	/* return NULL; */
    }
set_config_and_exit:
    tempo_segment_set_config(s, num_measures, bpm, num_beats, subdiv_lens); 

    /* fprintf(stderr, "DONE DONE DONE DONE now it looks like this\n"); */
    /* tempo_track_fprint(stderr, t); */

    return s;
}

/* /\* Pass -1 to "num_measures" for infinity *\/ */
/* TempoSegment *tempo_track_add_segment(TempoTrack *t, int32_t start_pos, int16_t num_measures, int bpm, int num_beats, int *subdiv_lens) */
/* { */
/*     TempoSegment *s = tempo_track_add_segment_internal(t, start_pos, num_measures, bpm, num_beats, ap); */
/*     return s; */
/* } */

TempoSegment *tempo_track_add_segment_at_measure(TempoTrack *t, int16_t measure, int16_t num_measures, int bpm, uint8_t num_beats, uint8_t *subdiv_lens)
{
    int32_t start_pos = 0;
    TempoSegment *s = t->segments;
    if (s) {
	while (1) {
	    /* fprintf(stderr, "S: %p, first measure index %d (cmp %d)\n", s, s->first_measure_index, measure); */
	    if (s->first_measure_index > measure) {
		if (s->prev) {
		    s = s->prev;
		} else {
		    fprintf(stderr, "ERROR: attempting to insert before first measure index at head\n");
		    exit(1);
		}
		break;
	    } else if (!s->next) {
		break;
	    }
	    s = s->next;
	}
	start_pos = get_beat_pos(s, measure, 0, 0);
    }
    s = tempo_track_add_segment(t, start_pos, num_measures, bpm, num_beats, subdiv_lens);
    return s;
}

/* void timeline_rectify_tracks(Timeline *tl) */
/* { */
/*     int tempo_track_index = 0; */
/*     int track_index = 0; */
    
/*     Track *track_stack[tl->num_tracks]; */
/*     TempoTrack *tempo_track_stack[tl->num_tempo_tracks]; */
    
/*     for (int i=0; i<tl->track_area->num_children; i++) { */
/* 	Layout *lt = tl->track_area->children[i]; */
/* 	for (uint8_t j=0; j<tl->num_tracks; j++) { */
/* 	    Track *track = tl->tracks[j]; */
/* 	    if (track->layout == lt) { */
/* 		track_stack[track_index] = track; */
/* 		track_index++; */
/* 		fprintf(stderr, "LAYOUT %d is track %d\n", i, j); */
/* 		break; */
/* 	    } */
/* 	} */
/* 	for (uint8_t j=0; j<tl->num_tempo_tracks; j++) { */
/* 	    TempoTrack *tt = tl->tempo_tracks[j]; */
/* 	    if (tt->layout == lt) { */
/* 		tempo_track_stack[tempo_track_index] = tt; */
/* 		tempo_track_index++; */
/* 		fprintf(stderr, "LAYOUT %d is TEMPO track %d\n", i, j); */
/* 		break; */
/* 	    } */
/* 	} */
/*     } */
/*     fprintf(stderr, "\n"); */
/*     for (int i=0; i<tl->num_tempo_tracks; i++) { */
/* 	fprintf(stderr, "IN TL: %p\n", tl->tempo_tracks[i]); */
/* 	fprintf(stderr, "IN STACK: %p\n", tempo_track_stack[i]); */
/*     } */
/*     memcpy(tl->tracks, track_stack, sizeof(Track *) * tl->num_tracks); */
/*     memcpy(tl->tempo_tracks, tempo_track_stack, sizeof(TempoTrack *) * tl->num_tempo_tracks); */
/* } */

TempoTrack *timeline_add_tempo_track(Timeline *tl)
{
    if (tl->num_tempo_tracks == MAX_TEMPO_TRACKS) return NULL;
    TempoTrack *t = calloc(1, sizeof(TempoTrack));
    t->tl = tl;
    t->metronome = &tl->proj->metronomes[0];

    Layout *tempo_tracks_area = layout_get_child_by_name_recursive(tl->layout, "tracks_area");
    /* Layout *lt = layout_read_xml_to_lt(tempo_tracks_area, TEMPO_TRACK_LT_PATH); */
    Layout *lt = layout_read_from_xml(TEMPO_TRACK_LT_PATH);
    if (tl->layout_selector >= 0) {
	layout_insert_child_at(lt, tempo_tracks_area, tl->layout_selector);
    } else {
	layout_insert_child_at(lt, tempo_tracks_area, 0);
    }

    t->layout = lt;
    layout_size_to_fit_children_v(tempo_tracks_area, true, 0);
    layout_reset(tl->layout);
    tl->needs_redraw = true;

    Layout *tb_lt = layout_get_child_by_name_recursive(t->layout, "display");
    snprintf(t->pos_str, TEMPO_POS_STR_LEN, "0.0.0:00000");
    t->readout = textbox_create_from_str(
	t->pos_str,
	tb_lt,
	main_win->mono_bold_font,
	14,
	main_win);
    textbox_set_align(t->readout, CENTER_LEFT);
    textbox_set_background_color(t->readout, &color_global_black);
    textbox_set_text_color(t->readout, &color_global_white);
    /* textbox_size_to_fit_v(t->readout, 4); */
    /* layout_center_agnostic(t->readout->layout, false, true); */
    textbox_set_pad(t->readout, 10, 0);
    textbox_set_trunc(t->readout, false);
    textbox_reset_full(t->readout);

    t->colorbar_rect = &layout_get_child_by_name_recursive(t->layout, "colorbar")->rect;
    t->console_rect = &layout_get_child_by_name_recursive(t->layout, "console")->rect;
    t->right_console_rect = &layout_get_child_by_name_recursive(t->layout, "right_console")->rect;
    t->right_colorbar_rect = &layout_get_child_by_name_recursive(t->layout, "right_colorbar")->rect;

    Layout *edit_button_lt = layout_get_child_by_name_recursive(t->layout, "edit_button");
    t->edit_button = textbox_create_from_str(
	"✎",
	edit_button_lt,
	main_win->symbolic_font,
	16,
	main_win);
    t->edit_button->corner_radius = MUTE_SOLO_BUTTON_CORNER_RADIUS;
    textbox_set_text_color(t->edit_button, &color_global_white);
    textbox_set_background_color(t->edit_button, &color_global_quickref_button_blue);
    /* textbox_set_border(t->edit_button, &color_global_black, 1); */
    t->edit_button->corner_radius = BUTTON_CORNER_RADIUS;
    textbox_set_border(t->edit_button, &color_global_white, 1);
    
    Layout *metro_button_lt = layout_get_child_by_name_recursive(t->layout, "metronome_button");
    t->metronome_button = textbox_create_from_str(
	"M",
	metro_button_lt,
	main_win->bold_font,
	14,
	main_win);
    textbox_set_background_color(t->metronome_button, &color_global_play_green);
    t->metronome_button->corner_radius = MUTE_SOLO_BUTTON_CORNER_RADIUS;
    textbox_set_border(t->metronome_button, &color_global_black, 1);
    /* textbox_set_background_color(track->tb_mute_button, &color_mute_solo_grey); */



    t->metronome_vol = 1.0;
    Layout *vol_lt = layout_get_child_by_name_recursive(t->layout, "metronome_vol_slider");
    t->metronome_vol_slider = slider_create(
	vol_lt,
	(void *)(&t->metronome_vol),
	JDAW_FLOAT,
	SLIDER_HORIZONTAL,
	SLIDER_FILL,
	NULL,
	/* &slider_label_amp_to_dbstr, */
	NULL,
	NULL,
	&tl->proj->dragged_component);
    
    Value min, max;
    min.float_v = 0.0f;
    max.float_v = TRACK_VOL_MAX;
    slider_set_range(t->metronome_vol_slider, min, max);
    slider_reset(t->metronome_vol_slider);

    uint8_t subdivs[] = {4, 4, 4, 4};
    tempo_track_add_segment(t, 0, -1, 120, 4, subdivs);
    
    tl->tempo_tracks[tl->num_tempo_tracks] = t;
    t->index = tl->num_tempo_tracks;
    tl->num_tempo_tracks++;


    /* TESTING ONLY */
    /* uint8_t subdivs_l[] = {4, 4, 4, 4}; */
    if (tl->num_tempo_tracks == 1) {
	subdivs[0] = 3;
	subdivs[1] = 3;
	subdivs[2] = 3;	
	tempo_track_add_segment(t, 10000, -1, 120, 3, subdivs);
	subdivs[2] = 2;
	subdivs[3] = 2;
	tempo_track_add_segment_at_measure(t, 4, -1, 500, 4, subdivs);
	TempoSegment *s = t->segments;
	while (s) {
	    /* tempo_segment_fprint(stderr, s); */
	    s = s->next;
	}
    }

    timeline_rectify_track_indices(tl);
    return t;
}

/* Destroy functions */

static void tempo_segment_destroy(TempoSegment *s)
{
    free(s);
}

void tempo_track_destroy(TempoTrack *tt)
{
    textbox_destroy(tt->metronome_button);
    textbox_destroy(tt->edit_button);
    slider_destroy(tt->metronome_vol_slider);
    layout_destroy(tt->layout);
    
    TempoSegment *s = tt->segments;
    while (s) {
	TempoSegment *next = s->next;
	tempo_segment_destroy(s);
	s = next;
    }
}

	

/* MID-LEVEL INTERFACE */

void timeline_edit_tempo_track_at_cursor(Timeline *tl, int num_measures, int bpm, int num_beats, uint8_t *subdiv_lens)
{
    TempoTrack *tt = timeline_selected_tempo_track(tl);
    if (!tt) return;
    TempoSegment *s = tempo_track_get_segment_at_pos(tt, tl->play_pos_sframes);
    tempo_segment_set_config(s, num_measures, bpm, num_beats, subdiv_lens);
    tl->needs_redraw = true;
}

static void simple_tempo_segment_remove(TempoSegment *s)
{
    fprintf(stderr, "SEGMENT REMOVE %p\n", s);
    TempoSegment *test = s->track->segments;
    while (test) {
	fprintf(stderr, "\t%p", test);
	if (test == s) {
	    fprintf( stderr, " <---\n");
	} else {
	    fprintf(stderr, "\n");
	}
	test = test->next;
    }
    fprintf(stderr, "Before:\n");
    tempo_track_fprint(stderr, s->track);
    TempoTrack *tt = s->track;
    if (s->prev) {
	s->prev->next = s->next;
	if (s->next) {
	    tempo_segment_set_end_pos(s->prev, s->next->start_pos);
	    s->next->prev = s->prev;
	} else {
	    s->prev->end_pos = s->prev->start_pos;
	    s->prev->num_measures = -1;
	}
    } else if (s->next) {
	tt->segments = s->next;
    }
    fprintf(stderr, "After:\n");
    tempo_track_fprint(stderr, s->track);
}



static TempoSegment *tempo_track_cut_at(TempoTrack *tt, int32_t at)
{
    TempoSegment *s = tempo_track_get_segment_at_pos(tt, at);
    uint8_t subdiv_lens[s->cfg.num_beats];
    memcpy(subdiv_lens, s->cfg.beat_subdiv_lens, s->cfg.num_beats * sizeof(uint8_t));
    return tempo_track_add_segment(tt, at, -1, s->cfg.bpm, s->cfg.num_beats, subdiv_lens);
}


NEW_EVENT_FN(undo_cut_tempo_track, "undo cut tempo track")
    TempoTrack *tt = (TempoTrack *)obj1;
    TempoSegment *s = (TempoSegment *)obj2;
    /* int32_t at = val1.int32_v; */
    /* s = tempo_track_get_segment_at_pos(tt, at); */
    simple_tempo_segment_remove(s);
    tt->tl->needs_redraw = true;
}

NEW_EVENT_FN(redo_cut_tempo_track, "redo cut tempo track")
    TempoTrack *tt = (TempoTrack *)obj1;
    int32_t at = val1.int32_v;
    TempoSegment *s = tempo_track_cut_at(tt, at);
    self->obj2 = (void *)s;
}

NEW_EVENT_FN(dispose_forward_cut_tempo_track, "")
    tempo_segment_destroy((TempoSegment *)obj2);
}


void timeline_cut_tempo_track_at_cursor(Timeline *tl)
{
    TempoTrack *tt = timeline_selected_tempo_track(tl);
    if (!tt) return;
    TempoSegment *s = tempo_track_cut_at(tt, tl->play_pos_sframes);
    tl->needs_redraw = true;
    Value cut_pos = {.int32_v = tl->play_pos_sframes};
    Value nullval = {.int_v = 0};
    user_event_push(
	&proj->history,
	undo_cut_tempo_track, redo_cut_tempo_track,
	NULL, dispose_forward_cut_tempo_track,
	(void *)tt, (void *)s,
	cut_pos, nullval,
	cut_pos, nullval,
	0, 0, false, false);
}

void timeline_increment_tempo_at_cursor(Timeline *tl, int inc_by)
{
    TempoTrack *tt = timeline_selected_tempo_track(tl);
    if (!tt) return;
    TempoSegment *s = tempo_track_get_segment_at_pos(tt, tl->play_pos_sframes);
    int new_tempo = s->cfg.bpm + inc_by;
    uint8_t subdiv_lens[s->cfg.num_beats];
    memcpy(subdiv_lens, s->cfg.beat_subdiv_lens, s->cfg.num_beats * sizeof(uint8_t));
    tempo_segment_set_config(s, s->num_measures, new_tempo, s->cfg.num_beats, subdiv_lens);
    tl->needs_redraw = true;
}

/* static enum beat_prominence timeline_get_beat_pos_range(Timeline *tl, int32_t *pos) */
/* { */
/*     TempoTrack *tt = timeline_selected_tempo_track(tl); */
/*     if (!tt) return BP_NONE; */
/*     TempoSegment *s = tempo_track_get_segment_at_pos(tt, tl->play_pos_sframes); */
/*     int32_t pos_n = s->start_pos; */
/*     int32_t pos_m; */
/*     while ((pos_m = pos_n + s->cfg.dur_sframes) >= *pos) { */
/* 	pos_n = pos_m; */
/*     } */
/*     enum beat_prominence bp; */
/*     int bar, beat, subdiv; */
/*     bar = beat = subdiv = 0; */
/*     set_beat_prominence(s, &bp, bar, beat, subdiv); */
    
/*     return bp; */
    
/*     /\* timeline_move_play_position(tl, pos_m); *\/ */
/*     /\* /\\* tl->needs_redraw = true; *\\/ *\/ */
/* } */

/* void tempo_track_fill_metronome_buffer(TempoTrack *tt, float *L, float *R, int32_t start_from) */
/* { */
/*     static TempoSegment *s = NULL; */
    
/*     if (!s) { */
/* 	s = tempo_track_get_segment_at_pos(tt, start_from); */
/*     } else { */
/* 	while (s->next && start_from < s->start_pos) { */
/* 	    s = s->next; */
/* 	} */
/*     } */
/* } */

static void tempo_track_deferred_draw(void *tempo_track_v)
{
    
    TempoTrack *tt = (TempoTrack *)tempo_track_v;
    SDL_Rect *audio_rect = tt->tl->proj->audio_rect;
    if (audio_rect->y + audio_rect->h > tt->tl->layout->rect.y) {
	SDL_RenderSetClipRect(main_win->rend, audio_rect);
	textbox_draw(tt->metronome_button);
	textbox_draw(tt->edit_button);
	slider_draw(tt->metronome_vol_slider);
	SDL_RenderSetClipRect(main_win->rend, NULL);
    }

}

void tempo_track_draw(TempoTrack *tt)
{
    Timeline *tl = tt->tl;
    SDL_Rect ttr = tt->layout->rect;
    
    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(color_global_tempo_track));
    SDL_RenderFillRect(main_win->rend, &ttr);

    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(color_global_black));
    SDL_RenderFillRect(main_win->rend, tt->console_rect);


    
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
    int x = timeline_get_draw_x(tl, pos);
    int main_top_y = tt->layout->rect.y;
    int bttm_y = main_top_y + tt->layout->rect.h - 1; /* TODO: figure out why decremet to bttm_y is necessary */
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

    /* const int draw_subdiv_thresh = 2; */
    /* const int draw_beat_thresh = 1; */
    /* bool draw_subdivs = true; */
    /* bool draw_beats = true; */
    while (x > 0) {
	tempo_track_get_next_pos(tt, false, 0, &pos, &bp);
	/* int prev_x = x; */
	x = timeline_get_draw_x(tl, pos);
	if (x > tt->right_console_rect->x) break;
	/* int x_diff; */
	/* if (draw_beats && (x_diff = x - prev_x) <= draw_beat_thresh) { */
	/*     draw_beats = false; */
	/*     draw_subdivs = false; */
	/* } else if (draw_subdivs && x_diff <= draw_subdiv_thresh) { */
	/*     draw_subdivs = false; */
	/* } */
	/* if (!draw_subdivs && (bp >= BP_SUBDIV)) continue; */
	/* if (!draw_beats && (bp >= BP_BEAT)) continue; */
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
    textbox_draw(tt->readout);

    /* Fill right console */
    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(control_bar_bckgrnd));
    SDL_RenderFillRect(main_win->rend, tt->right_console_rect);

    /* Draw right console elements */
    window_defer_draw(main_win, tempo_track_deferred_draw, tt);

    /* Draw outline */
    SDL_SetRenderDrawColor(main_win->rend, 100, 100, 100, 255);
    SDL_RenderDrawRect(main_win->rend, &ttr);
    SDL_RenderFillRect(main_win->rend, tt->colorbar_rect);
    SDL_RenderFillRect(main_win->rend, tt->right_colorbar_rect);

}

/* sets bar, beat, and pos; returns remainder in sframes */
int32_t tempo_track_bar_beat_subdiv(TempoTrack *tt, int32_t pos, int *bar_p, int *beat_p, int *subdiv_p, TempoSegment **segment_p, bool set_readout)
{
    /* bool debug = false; */
    /* if (strcmp(get_thread_name(), "main") == 0) { */
    /* 	debug = true; */
    /* } */
    /* fprintf(stderr, "CALL TO bar beat subdiv in thread %s\n", get_thread_name()); */
    static JDAW_THREAD_LOCAL int measures[MAX_TEMPO_TRACKS];
    static JDAW_THREAD_LOCAL int beats[MAX_TEMPO_TRACKS];
    static JDAW_THREAD_LOCAL int subdivs[MAX_TEMPO_TRACKS];
    /* static JDAW_THREAD_LOCAL int32_t prev_positions[MAX_TEMPO_TRACKS]; */
    
    int measure = measures[tt->index];
    int beat = beats[tt->index];
    int subdiv = subdivs[tt->index];

    TempoSegment *s = tempo_track_get_segment_at_pos(tt, pos);
    if (measure < s->first_measure_index) {
	/* fprintf(stderr, "Cache invalid MEASURE %d (s num: %d)\n", measure, s->num_measures); */
	measure = s->first_measure_index;
	beat = 0;
	subdiv = 0;
    } else if (beat > s->cfg.num_beats -1) {
	/* fprintf(stderr, "Cache invalid BEAT\n"); */
	beat = 0;
	subdiv = 0;
    } else if (subdiv > s->cfg.beat_subdiv_lens[beat] - 1) {
	/* fprintf(stderr, "Cache invalid SUBDIV\n"); */
	subdiv = 0;
    }
    
    /* if (measure == 0) measure = s->first_measure_index; */
    /* int32_t prev_pos = prev_positions[tt->index]; */
    int32_t beat_pos = 0;
    #ifdef TESTBUILD
    int ops = 0;
    clock_t c;
    c = clock();
    #endif
    int32_t remainder = 0;
    /* if (debug) { */
    /* 	/\* fprintf(stderr, "\t\tSTART loop\n"); *\/ */
    /* } */
    while (1) {
	
	
	beat_pos = get_beat_pos(s, measure, beat, subdiv);
	remainder = pos - beat_pos;

	#ifdef TESTBUILD
	ops++;
	if (ops > 1000000 - 5) {
	    fprintf(stderr, "ABORTING soon... ops = %d\n", ops);
	    fprintf(stderr, "Segment start/end: %d-%d\n", s->start_pos, s->end_pos);
	    fprintf(stderr, "Beat pos: %d\n", beat_pos);
	    fprintf(stderr, "Pos: %d\n", pos);
	    if (ops > 1000000) {
		fprintf(stderr, "ABORT\n");
		tempo_track_fprint(stderr, tt);
		breakfn();
		goto set_dst_values;
		/* return 0; */
		/* exit(1); */
	    }
	}
	#endif
	
	/* if (debug) { */
	/*     /\* fprintf(stderr, "\t\t\tremainder: (%d - %d) %d cmp dur approx: %d\n", pos, beat_pos, remainder, s->cfg.atom_dur_approx); *\/ */
	/* } */
	if (remainder < 0) {
	    do_decrement(s, &measure, &beat, &subdiv);
	} else {
	    if (remainder < s->cfg.atom_dur_approx) {
		break;
	    }
	    do_increment(s, &measure, &beat, &subdiv);
	    /* prev_pos = beat_pos; */
	}

    }
    /* if (debug) { */
    /* 	/\* fprintf(stderr, "\t\t->exit loop, ops %d\n", ops); *\/ */
    /* } */
    /* v_positions[tt->index] = pos; */
    
    /* do_decrement(s, &measure, &beat, &subdiv); */
    /* double dur = ((double)clock() - c) / CLOCKS_PER_SEC; */
    /* fprintf(stderr, "Ops; %d, time msec: %f\n", ops, dur * 1000); */


    /* Set destination pointer values */
set_dst_values:
    if (bar_p && beat_p && subdiv_p) {
	*bar_p = measure;
	*beat_p = beat;
	*subdiv_p = subdiv;
    }
    if (segment_p) {
	*segment_p = s;
    }

    /* Set cache values */
    measures[tt->index] = measure;
    beats[tt->index] = beat;
    subdivs[tt->index] = subdiv;
    
    /* Translate from zero-indexed for readout */
    measure = measure >= 0 ? measure + 1 : measure;
    beat++;
    subdiv++;
    
    /* remainder = pos - prev_pos; */
    /* fprintf(stderr, "AT %d:\t%d, %d, %d\n", pos, measure, beat, subdiv); */
    if (set_readout) {
	static const int sample_str_len = 7;
	char sample_str[sample_str_len];
	if (remainder < -99999) {
	    snprintf(sample_str, sample_str_len, "%s", "-∞");
	} else if (remainder > 999999) {
	    snprintf(sample_str, sample_str_len, "%s", "∞");
	} else {
	    snprintf(sample_str, sample_str_len, "%d", remainder);
	}
	/* fprintf(stderr, "MEASURE RAW: %d\n", measure); */
	snprintf(tt->pos_str, TEMPO_POS_STR_LEN, "%d.%d.%d:%s", measure, beat, subdiv, sample_str);
	textbox_reset_full(tt->readout);
    /* fprintf(stderr, "%s\n", tt->pos_str); */
    }
    return remainder;
}

void tempo_track_mix_metronome(TempoTrack *tt, float *mixdown_buf, int32_t mixdown_buf_len, int32_t tl_start_pos_sframes, int32_t tl_end_pos_sframes, float step)
{
    if (tt->muted) return;
    if (step < 0.0) return;
    if (step > 4.0) return;
    /* fprintf(stderr, "TEMPO TRACK MIX METRONOME\n"); */
    /* clock_t start = clock(); */
    int bar, beat, subdiv;
    TempoSegment *s;
    float *buf = NULL;
    int32_t buf_len;
    int32_t tl_chunk_len = tl_end_pos_sframes - tl_start_pos_sframes;
    enum beat_prominence bp;
    /* clock_t c; */
    /* c = clock(); */
    int32_t remainder = tempo_track_bar_beat_subdiv(tt, tl_end_pos_sframes, &bar, &beat, &subdiv, &s, false);
    /* double dur = ((double)clock() - c)/CLOCKS_PER_SEC; */
    /* fprintf(stderr, "Remainder dur msec: %f\n", dur * 1000); */
    
    remainder -= tl_chunk_len;
    int i=0;
    /* int outer_ops = 0; */
    /* int inner_ops = 0; */
    while (1) {
	/* outer_ops++; */
	i++;
	if (i>10) {
	    /* fprintf(stderr, "Throttle\n"); */
	    /* fprintf(stderr, "OPS outer: %d inner: %d\n", outer_ops, inner_ops); */
	    /* fprintf(stderr, "->EXIT metro\n"); */
	    return;
	}

	int32_t tick_start_in_chunk = remainder * -1 / step;
	if (tick_start_in_chunk > mixdown_buf_len) {
	    goto previous_beat;
	}
	set_beat_prominence(s, &bp, bar, beat, subdiv);
	if (bp < 2) {
	    buf = tt->metronome->buffers[0];
	    buf_len = tt->metronome->buf_lens[0];

	} else if (bp == 2) {
	    buf = tt->metronome->buffers[1];
	    buf_len = tt->metronome->buf_lens[1];
	} else {
	    /* Beat is not audible */
	    goto previous_beat;
	}

	int32_t chunk_end_i = tick_start_in_chunk + mixdown_buf_len;
	/* Normal exit point */
	if (chunk_end_i < 0) {
	    break;
	}
	int32_t buf_i = 0;
	double buf_i_d = 0.0;
	/* double tick_i_d = (double)tick_start_in_chunk; */
	while (tick_start_in_chunk < mixdown_buf_len) {
	    if (tick_start_in_chunk > 0 && buf_i > 0 && buf_i < buf_len) {
		mixdown_buf[tick_start_in_chunk] += buf[buf_i] * tt->metronome_vol;
	    }
	    buf_i_d += step;
	    buf_i = (int32_t)round(buf_i_d);
	    tick_start_in_chunk++;
	    /* tick_start_in_chunk = (round((double)tick_start_in_chunk + step)); */
	    /* inner_ops++; */
	}
    previous_beat:
	do_decrement(s, &bar, &beat, &subdiv);
	/* c = clock(); */
	remainder = (tl_start_pos_sframes - get_beat_pos(s, bar, beat, subdiv));
    }
}

void tempo_track_mute_unmute(TempoTrack *t)
{
    t->muted = !t->muted;
    if (t->muted) {
	textbox_set_background_color(t->metronome_button, &mute_red);
    } else {
	textbox_set_background_color(t->metronome_button, &color_global_play_green);
    }
    t->tl->needs_redraw = true;
}

/* void track_increment_vol(Track *track) */
/* { */
/*     track->vol += TRACK_VOL_STEP; */
/*     if (track->vol > track->vol_ctrl->max.float_v) { */
/* 	track->vol = track->vol_ctrl->max.float_v; */
/*     } */
/*     slider_edit_made(track->vol_ctrl); */
/*     slider_reset(track->vol_ctrl); */
/* } */

void tempo_track_increment_vol(TempoTrack *tt)
{
    tt->metronome_vol += TRACK_VOL_STEP;
    if (tt->metronome_vol > tt->metronome_vol_slider->max.float_v) {
	tt->metronome_vol = tt->metronome_vol_slider->max.float_v;
    }
    slider_edit_made(tt->metronome_vol_slider);
    slider_reset(tt->metronome_vol_slider);
}
void tempo_track_decrement_vol(TempoTrack *tt)
{
    tt->metronome_vol -= TRACK_VOL_STEP;
    if (tt->metronome_vol < 0.0f) {
	tt->metronome_vol = 0.0f;
    }
    slider_edit_made(tt->metronome_vol_slider);
    slider_reset(tt->metronome_vol_slider);
}


bool tempo_track_triage_click(uint8_t button, TempoTrack *t)
{
    if (!SDL_PointInRect(&main_win->mousep, &t->layout->rect)) {
	return false;
    }
    if (SDL_PointInRect(&main_win->mousep, t->right_console_rect)) {
	if (slider_mouse_click(t->metronome_vol_slider, main_win)) {
	    return true;
	}
	if (SDL_PointInRect(&main_win->mousep, &t->metronome_button->layout->rect)) {
	    tempo_track_mute_unmute(t);
	    return true;
	}
	return true;
    }
    if (SDL_PointInRect(&main_win->mousep, t->console_rect)) {
	fprintf(stderr, "Left console\n");
    }
    return false;
}
   

extern Project *proj;


void tempo_segment_fprint(FILE *f, TempoSegment *s)
{
    fprintf(f, "\nSegment at %p start/end: %d-%d (%f-%fs)\n", s, s->start_pos, s->end_pos, (float)s->start_pos / proj->sample_rate, (float)s->end_pos / proj->sample_rate);
    fprintf(f, "Segment tempo (bpm): %d\n", s->cfg.bpm);
    fprintf(stderr, "Segment measure start i: %d; num measures: %d\n", s->first_measure_index, s->num_measures);
    fprintf(f, "\tCfg beats: %d\n", s->cfg.num_beats);
    fprintf(f, "\tCfg beat subdiv lengths:\n");
    for (int i=0; i<s->cfg.num_beats; i++) {
	fprintf(f, "\t\t%d: %d\n", i, s->cfg.beat_subdiv_lens[i]);
    }
    fprintf(f, "\tCfg num atoms: %d\n", s->cfg.num_atoms);
    fprintf(f, "\tCfg measure dur: %d\n", s->cfg.dur_sframes);
    fprintf(f, "\tCfg atom dur: %d\n", s->cfg.dur_sframes / s->cfg.num_atoms);

}

void timeline_segment_at_cursor_fprint(FILE *f, Timeline *tl)
{
    TempoTrack *tt = timeline_selected_tempo_track(tl);
    TempoSegment *s = tempo_track_get_segment_at_pos(tt, tl->play_pos_sframes);
    tempo_segment_fprint(f, s);
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
