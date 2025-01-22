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
    tempo.c

    * Measure configuration
    * Drawing and arranging tempo tracks
    * Setting tempo and time signature
 *****************************************************************************************************************/

#include "endpoint_callbacks.h"
#include "page.h"
#include "text.h"
#include "thread_safety.h"

/* #include <stdarg.h> */
#include "assets.h"
#include "color.h"
#include "components.h"
#include "geometry.h"
#include "layout.h"
#include "layout_xml.h"
#include "modal.h"
#include "project.h"
#include "tempo.h"
#include "test.h"
#include "timeline.h"
#include "user_event.h"
#include "wav.h"

extern Window *main_win;
extern Project *proj;
extern SDL_Color color_global_tempo_track;
extern SDL_Color color_global_light_grey;
extern SDL_Color color_global_black;
extern SDL_Color color_global_white;
extern SDL_Color color_global_quickref_button_blue;
extern SDL_Color control_bar_bckgrnd;
extern SDL_Color mute_red;
extern SDL_Color color_global_play_green;

extern pthread_t MAIN_THREAD_ID;

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

void project_destroy_metronomes(Project *proj)
{
    for (int i=0; i<PROJ_NUM_METRONOMES; i++) {
	Metronome *m = proj->metronomes + i;
	if (m->buffers[0])
	    free(m->buffers[0]);
	if (m->buffers[1])
	    free(m->buffers[1]);
    }
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

static void tempo_segment_set_config(TempoSegment *s, int num_measures, int bpm, uint8_t num_beats, uint8_t *subdivs, enum ts_end_bound_behavior ebb)
{
    if (num_beats > MAX_BEATS_PER_BAR) {
	fprintf(stderr, "Error: num_beats exceeds maximum per bar (%d)\n", MAX_BEATS_PER_BAR);
	return;
    }
    s->cfg.bpm = bpm;
    s->cfg.num_beats = num_beats;

    int32_t old_dur_sframes = s->cfg.dur_sframes;
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

    if (!s->next) {
	s->end_pos = s->start_pos;
	s->num_measures = -1;
    } else {
	if (ebb == SEGMENT_FIXED_NUM_MEASURES) {
	    float num_measures_f = ((float)s->end_pos - s->start_pos)/old_dur_sframes;
	    int32_t end_pos = s->start_pos + num_measures_f * s->cfg.dur_sframes;
	    tempo_segment_set_end_pos(s, end_pos);
	} else {
	    tempo_segment_set_end_pos (s, s->next->start_pos);
	}
    }
    /* if (s->track->end_bound_behavior == SEGMENT_FIXED_NUM_MEASURES) { */
	
    /* 	tempo_segment_set_end_pos(s, s->start_pos + measure_dur * s->num_measures); */
    /* } else { */
    /* 	if (num_measures > 0 && s->next) { */
    /* 	    s->end_pos = s->start_pos + num_measures * s->cfg.dur_sframes; */
    /* 	} else { */
    /* 	    s->end_pos = s->start_pos; */
    /* 	} */
    /* 	if (s->next) { */
    /* 	    tempo_segment_set_end_pos(s, s->next->start_pos); */
    /* 	} */
    /* } */
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
	int32_t dur_next = s->next->end_pos - s->next->start_pos;
	s->next->start_pos = s->end_pos;
	if (s->next->next) {
	    s->next->end_pos = s->next->start_pos + dur_next;
	} else {
	    s->next->end_pos = s->next->start_pos;
	}
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
	tempo_segment_set_config(s, num_measures, bpm, num_beats, subdiv_lens, SEGMENT_FIXED_END_POS);
	tempo_segment_set_end_pos(interrupt, start_pos);
	/* tempo_segment_set_end_pos(s, s->next->start_pos); */
	return s;
	/* goto set_config_and_exit; */
	/* fprintf(stderr, "Error: cannot insert segment in the middle\n"); */
	/* return NULL; */
    }
set_config_and_exit:
    tempo_segment_set_config(s, num_measures, bpm, num_beats, subdiv_lens, SEGMENT_FIXED_END_POS); 

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

/* void tempo_track_reset(TempoTrack *tt) */
/* { */
/*     layout_reset(tt->layout); */
/*     textbox_reset(tt->readout); */
/* } */

static void tempo_track_remove(TempoTrack *tt)
{
    Timeline *tl = tt->tl;
    for (int i=tt->index; i<tl->num_tempo_tracks - 1; i++) {
	tl->tempo_tracks[i] = tl->tempo_tracks[i+1];
    }
    tl->num_tempo_tracks--;
    layout_remove_child(tt->layout);
    timeline_rectify_track_indices(tl);
    timeline_rectify_track_area(tl);

}
static void tempo_track_reinsert(TempoTrack *tt)
{
    Timeline *tl = tt->tl;
    for (int i=tl->num_tempo_tracks; i>tt->index; i--) {
	tl->tempo_tracks[i] = tl->tempo_tracks[i-1];
    }
    tl->tempo_tracks[tt->index] = tt;
    tl->num_tempo_tracks++;
    layout_insert_child_at(tt->layout, tl->track_area, tt->layout->index);
    tl->layout_selector = tt->layout->index;
    timeline_rectify_track_indices(tl);
    timeline_rectify_track_area(tl);
}

NEW_EVENT_FN(undo_add_tempo_track, "undo add tempo track")
    TempoTrack *tt = obj1;
    tempo_track_remove(tt);

}
NEW_EVENT_FN(redo_add_tempo_track, "redo add tempo track")
    TempoTrack *tt = obj1;
    tempo_track_reinsert(tt);
}

NEW_EVENT_FN(dispose_forward_add_tempo_track, "")
    tempo_track_destroy((TempoTrack *)obj1);
}

TempoTrack *timeline_add_tempo_track(Timeline *tl)
{
    if (tl->num_tempo_tracks == MAX_TEMPO_TRACKS) return NULL;
    TempoTrack *t = calloc(1, sizeof(TempoTrack));
    snprintf(t->name, MAX_NAMELENGTH, "click_track_%d", tl->num_tempo_tracks + 1);
    t->tl = tl;
    t->metronome = &tl->proj->metronomes[0];
    snprintf(t->num_beats_str, 3, "4");
    for (int i=0; i<MAX_BEATS_PER_BAR; i++) {
	snprintf(t->subdiv_len_strs[i], 2, "4");
    }    

    Layout *tempo_tracks_area = layout_get_child_by_name_recursive(tl->layout, "tracks_area");
    if (!tempo_tracks_area) {
	fprintf(stderr, "Error: no tempo tracks area\n");
	exit(1);
    }
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

    layout_force_reset(lt);
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



    t->metronome_vol = 1.0f;
	
    Layout *vol_lt = layout_get_child_by_name_recursive(t->layout, "metronome_vol_slider");
    t->metronome_vol_slider = slider_create(
	vol_lt,
	&t->metronome_vol_ep,
	(Value){.float_v = 0.0f},
	(Value){.float_v = TRACK_VOL_MAX},
	SLIDER_HORIZONTAL,
	SLIDER_FILL,
	/* NULL, */
	/* /\* &slider_label_amp_to_dbstr, *\/ */
	/* NULL, */
	NULL,
	/* NULL, */
	&tl->proj->dragged_component);
    
    /* Value min, max; */
    /* min.float_v = 0.0f; */
    /* max.float_v = TRACK_VOL_MAX; */
    /* slider_set_range(t->metronome_vol_slider, min, max); */
    /* slider_reset(t->metronome_vol_slider); */

    uint8_t subdivs[] = {4, 4, 4, 4};
    tempo_track_add_segment(t, 0, -1, 120, 4, subdivs);
    
    tl->tempo_tracks[tl->num_tempo_tracks] = t;
    t->index = tl->num_tempo_tracks;
    tl->num_tempo_tracks++;

    timeline_rectify_track_indices(tl);
    /* timeline_reset(tl, false); */
    /* layout_force_reset(tl->layout); */
    /* layout_reset(lt); */
    Value nullval = {.int_v = 0};
    user_event_push(
	&proj->history,
	undo_add_tempo_track,
	redo_add_tempo_track,
	NULL,
	dispose_forward_add_tempo_track,
	(void *)t, NULL,
	nullval, nullval,
	nullval, nullval,
	0, 0, false, false);
    timeline_rectify_track_area(tl);

    endpoint_init(
	&t->metronome_vol_ep,
	&t->metronome_vol,
	JDAW_FLOAT,
	"metro_vol",
	"undo/redo adj metronome vol",
	JDAW_THREAD_MAIN,
	NULL, NULL, NULL,
	NULL, NULL, NULL, NULL);
    endpoint_set_allowed_range(
	&t->metronome_vol_ep,
	(Value){.float_v = 0.0},
	(Value){.float_v = TRACK_VOL_MAX});

    endpoint_init(
	&t->end_bound_behavior_ep,
	&t->end_bound_behavior,
	JDAW_INT,
	"segment_end_bound_behavior",
	"undo/redo set tempo track end bound behavior",
	JDAW_THREAD_MAIN,
        tempo_track_ebb_gui_cb, NULL, NULL,
	NULL, NULL, NULL, NULL);
    
    endpoint_set_allowed_range(
	&t->metronome_vol_ep,
	(Value){.int_v = 0},
	(Value){.int_v = 1});



    /* timeline_reset(tl, false); */
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



/* utility functions */
/* static void tempo_segment_set_tempo(TempoSegment *s, unsigned int new_tempo, bool from_undo); */
static void tempo_segment_set_tempo(TempoSegment *s, unsigned int new_tempo, enum ts_end_bound_behavior ebb,  bool from_undo);

NEW_EVENT_FN(undo_redo_set_tempo, "undo/redo set tempo")
    TempoSegment *s = (TempoSegment *)obj1;
    int tempo = val1.int_v;
    enum ts_end_bound_behavior ebb = (enum ts_end_bound_behavior)val2.int_v;
    tempo_segment_set_tempo(s, tempo, ebb, true);
}


static void tempo_segment_set_tempo(TempoSegment *s, unsigned int new_tempo, enum ts_end_bound_behavior ebb,  bool from_undo)
{
    if (new_tempo == 0) {
	status_set_errstr("Tempo cannot be 0 bpm");
    }
    int old_tempo = s->cfg.bpm;
    /* float num_measures; */
    /* if (s->next && ebb == SEGMENT_FIXED_NUM_MEASURES) { */
    /* 	if (s->cfg.dur_sframes * s->num_measures == s->end_pos - s->start_pos) { */
    /* 	    num_measures = s->num_measures; */
    /* 	} else { */
    /* 	    num_measures = ((float)s->end_pos - s->start_pos) / s->cfg.dur_sframes; */
    /* 	} */

    /* } */
    uint8_t subdiv_lens[s->cfg.num_beats];
    memcpy(subdiv_lens, s->cfg.beat_subdiv_lens, s->cfg.num_beats * sizeof(uint8_t));
    tempo_segment_set_config(s, s->num_measures, new_tempo, s->cfg.num_beats, subdiv_lens, ebb);

    /* if (s->next && ebb == SEGMENT_FIXED_NUM_MEASURES) { */
    /* 	tempo_segment_set_end_pos(s, s->start_pos + num_measures * s->cfg.dur_sframes); */
    /* } */
    
    s->track->tl->needs_redraw = true;
    if (!from_undo) {
	Value old_val = {.int_v = old_tempo};
	Value new_val = {.int_v = new_tempo};
	Value ebb = {.int_v = (int)(s->track->end_bound_behavior)};
	user_event_push(
	    &proj->history,
	    undo_redo_set_tempo,
	    undo_redo_set_tempo,
	    NULL, NULL,
	    (void *)s, NULL,
	    old_val, ebb,
	    new_val, ebb,
	    0, 0, false, false);
    }
}

	

/* MID-LEVEL INTERFACE */

static int set_tempo_submit_form(void *mod_v, void *target)
{
    Modal *mod = (Modal *)mod_v;
    TempoSegment *s = (TempoSegment *)mod->stashed_obj;
    for (uint8_t i=0; i<mod->num_els; i++) {
	ModalEl *el = mod->els[i];
	if (el->type == MODAL_EL_TEXTENTRY) {
	    char *value = ((TextEntry *)el->obj)->tb->text->value_handle;
	    int tempo = atoi(value);
	    tempo_segment_set_tempo(s, tempo, s->track->end_bound_behavior, false);
	    break;
	}
    }
    window_pop_modal(main_win);
    return 0;
}

/* static int tempo_rb_action(void *self, void *target) */
/* { */
/*     RadioButton *rb = (RadioButton *)self; */
/*     enum ts_end_bound_behavior *ebb = target; */
/*     *ebb = rb->selected_item; */
/*     return 0; */
/* } */

#define TEMPO_STRLEN 5
void timeline_tempo_track_set_tempo_at_cursor(Timeline *tl)
{
    TempoTrack *tt = timeline_selected_tempo_track(tl);
    if (!tt) {
	status_set_errstr("No tempo track selected");
	return;
    }
    TempoSegment *s = tempo_track_get_segment_at_pos(tt, tl->play_pos_sframes);
    Layout *mod_lt = layout_add_child(main_win->layout);
    layout_set_default_dims(mod_lt);
    Modal *mod = modal_create(mod_lt);
    static char tempo_str[TEMPO_STRLEN];
    snprintf(tempo_str, TEMPO_STRLEN, "%d", s->cfg.bpm);
    modal_add_header(mod, "Set tempo:", &color_global_light_grey, 4);
    ModalEl *el = modal_add_textentry(
	mod,
	tempo_str,
	TEMPO_STRLEN,
	txt_integer_validation,
	NULL, NULL);
    el->layout->y.value += 15.0;
    TextEntry *te = (TextEntry *)el->obj;
    te->target = (void *)mod;
    te->tb->text->completion = NULL;
    te->tb->text->completion_target = NULL;

    if (s->next) {
	char opt1[64];
	char opt2[64];
	char timestr[64];
	timecode_str_at(tt->tl, timestr, 64, s->end_pos);
	snprintf(opt1, 64, "Fixed end pos (%s)", timestr);

	if (s->cfg.dur_sframes * s->num_measures == s->end_pos - s->start_pos) {
	    snprintf(opt2, 64, "Fixed num measures (%d)", s->num_measures);
	} else {
	    snprintf(opt2, 64, "Fixed num measures (%f)", (float)(s->end_pos - s->start_pos)/s->cfg.dur_sframes);
	}
	char *options[] = {opt1, opt2};

	
	/* TODO: FIX THIS */
	el = modal_add_radio(
	    mod,
	    &color_global_white,
	    /* NULL, */
	    &tt->end_bound_behavior_ep,
	    /* &tt->end_bound_behavior, */
	    /* tempo_rb_action, */
	    (const char **)options,
	    2);	

	radio_button_reset_from_endpoint(el->obj);
	/* te->tb->text->max_len = TEMPO_STRLEN; */
    }
    mod->layout->w.value = 450;

    
    el = modal_add_button(
	mod,
	"Submit",
	set_tempo_submit_form);

    layout_reset(mod->layout);
    layout_center_agnostic(el->layout, true, false);

    mod->stashed_obj = (void *)s;
    mod->submit_form = set_tempo_submit_form;
    window_push_modal(main_win, mod);

    modal_reset(mod);
    modal_move_onto(mod);
    tl->needs_redraw = true;
 }

static void tempo_track_delete(TempoTrack *tt, bool from_undo);

NEW_EVENT_FN(undo_delete_tempo_track, "undo delete tempo track")
    TempoTrack *tt = (TempoTrack *)obj1;
    tempo_track_reinsert(tt);
}

NEW_EVENT_FN(redo_delete_tempo_track, "redo delete tempo track")
    TempoTrack *tt = (TempoTrack *)obj1;
    tempo_track_delete(tt, true);
}

NEW_EVENT_FN(dispose_delete_tempo_track, "")
    TempoTrack *tt = (TempoTrack *)obj1;
    tempo_track_destroy(tt);
}

static void tempo_track_delete(TempoTrack *tt, bool from_undo)
{
    tempo_track_remove(tt);
    Value nullval = {.int_v = 0};
    if (!from_undo) {
	user_event_push(
	    &proj->history,
	    undo_delete_tempo_track,
	    redo_delete_tempo_track,
	    dispose_delete_tempo_track,
	    NULL,
	    (void *)tt, NULL,
	    nullval, nullval, nullval, nullval,
	    0, 0, false, false);	    
    }
}

bool timeline_tempo_track_delete(Timeline *tl)
{
    TempoTrack *tt = timeline_selected_tempo_track(tl);
    if (!tt) return false;
    tempo_track_delete(tt, false);
    timeline_reset(tl, false);
    return true;
}


static void tempo_track_populate_settings_internal(TempoSegment *s, TabView *tv, bool set_from_cfg);
static void reset_settings_page_subdivs(TempoSegment *s, TabView *tv, const char *selected_el_id)
{
    /* TempoTrack *tt = s->track; */
    tempo_track_populate_settings_internal(s, tv, false);
    Page *p = tv->tabs[0];
    page_select_el_by_id(p, selected_el_id);
}

txt_int_range_completion(1, 13)
static int num_beats_completion(Text *txt, void *s_v)
{

    txt_int_range_completion_1_13(txt, NULL);
    TabView *tv = main_win->active_tabview;
    if (!tv) {
	fprintf(stderr, "Error: no tabview on num beats completion\n");
	exit(1);
    }
    TempoSegment *s = (TempoSegment *)s_v;
    if (atoi(txt->display_value) != s->cfg.num_beats) {
	reset_settings_page_subdivs(s_v, tv, "tempo_segment_num_beats_value");
    }
    return 0;
}

static TempoSegment *tempo_segment_copy(TempoSegment *s)
{
    TempoSegment *cpy = calloc(1, sizeof(TempoSegment));
    memcpy(cpy, s, sizeof(TempoSegment));
    return cpy;
}

NEW_EVENT_FN(undo_redo_set_segment_params, "undo/redo edit tempo segment")
    TempoSegment *s = (TempoSegment *)obj1;
    s = tempo_track_get_segment_at_pos(s->track, s->start_pos);
    self->obj1 = s;
    TempoSegment *cpy = (TempoSegment *)obj2;
    enum ts_end_bound_behavior ebb = val1.int_v;

    TempoSegment *redo_cpy = tempo_segment_copy(s);
    tempo_segment_set_config(s, -1, cpy->cfg.bpm, cpy->cfg.num_beats, cpy->cfg.beat_subdiv_lens, ebb);
    tempo_segment_destroy(cpy);
    self->obj2 = redo_cpy;
    s->track->tl->needs_redraw = true;
}


static int time_sig_submit_button_action(void *self, void *s_v)
{
    TempoSegment *s = (TempoSegment *)s_v;
    TempoSegment *cpy = tempo_segment_copy(s);
    TempoTrack *tt = s->track;

    int num_beats = atoi(tt->num_beats_str);
    int tempo = atoi(tt->tempo_str);
    uint8_t subdivs[num_beats];
    for (int i=0; i<num_beats; i++) {
	subdivs[i] = atoi(tt->subdiv_len_strs[i]);
    }
    tempo_segment_set_config(s, -1, tempo, atoi(tt->num_beats_str), subdivs, tt->end_bound_behavior);
    TabView *tv = main_win->active_tabview;
    tabview_close(tv);
    tt->tl->needs_redraw = true;

    Value ebb = {.int_v = tt->end_bound_behavior};
    user_event_push(
	&proj->history,
	undo_redo_set_segment_params,
	undo_redo_set_segment_params,
	NULL, NULL,
	s,
	cpy,
	ebb, ebb,
	ebb, ebb,
	0, 0,
	false, true);
    return 0;
}

static void draw_time_sig(void *tt_v, void *rect_v)
{
    TempoTrack *tt = (TempoTrack *)tt_v;
    SDL_Rect *rect = (SDL_Rect *)rect_v;
    int num_beats = atoi(tt->num_beats_str);
    int subdivs[num_beats];
    int num_atoms = 0;
    for (int i=0; i<num_beats; i++) {
	subdivs[i] = atoi(tt->subdiv_len_strs[i]);
	num_atoms += subdivs[i];
    }
    SDL_SetRenderDrawColor(main_win->rend, 255, 255, 255, 255);
    SDL_RenderDrawLine(main_win->rend, rect->x, rect->y + rect->h, rect->x + rect->w, rect->y + rect->h);
    SDL_RenderDrawLine(main_win->rend, rect->x, rect->y, rect->x, rect->y + rect->h);
    SDL_RenderDrawLine(main_win->rend, rect->x + rect->w, rect->y, rect->x + rect->w, rect->y + rect->h);
    int beat_i = 0;
    int subdiv_i = 1;
    for (int i=1; i<num_atoms; i++) {
	int x = rect->x + rect->w * i / num_atoms;
	float h_prop = 0.75;
	if (subdiv_i != 0) {
	    if (subdivs[beat_i] % 2 == 0 && subdiv_i % 2 == 0) {
		h_prop = 0.4;
	    } else {
		h_prop = 0.2;
	    }
	}
	SDL_RenderDrawLine(main_win->rend, x, rect->y + rect->h, x, rect->y + rect->h - rect->h * h_prop);
	if (subdiv_i >= subdivs[beat_i] - 1) {
	    subdiv_i = 0;
	    beat_i++;
	} else {
	    subdiv_i++;
	}

    }
}

/* txt_int_validation_range(txt, input, 1, 9) */
/* txt_int_validation_range(txt, input, 1, 13) */
txt_int_range_completion(1, 9)

static int segment_next_action(void *self, void *targ)
{
    TempoSegment *s = (TempoSegment *)targ;
    tempo_track_populate_settings_internal(s->next, main_win->active_tabview, true);
    Page *p = main_win->active_tabview->tabs[0];
    page_select_el_by_id(p, "segment_right");

    /* reset_settings_page_subdivs(s->next, main_win->active_tabview, "segment_right"); */
    return 0;
}

static int segment_prev_action(void *self, void *targ)
{
    TempoSegment *s = (TempoSegment *)targ;
    tempo_track_populate_settings_internal(s->prev, main_win->active_tabview, true);
    Page *p = main_win->active_tabview->tabs[0];
    page_select_el_by_id(p, "segment_left");
    /* reset_settings_page_subdivs(s->prev, main_win->active_tabview, "segment_left"); */
    return 0;
}

static void tempo_track_populate_settings_internal(TempoSegment *s, TabView *tv, bool set_from_cfg)
{

    TempoTrack *tt = s->track;
    /* TempoSegment *s = tempo_track_get_segment_at_pos(tt, tt->tl->play_pos_sframes); */
    
    static SDL_Color page_colors[] = {
	{40, 40, 80, 255},
	{50, 50, 80, 255},
	{70, 40, 70, 255}
    };

    tabview_clear_all_contents(tv);
    
    Page *page = tab_view_add_page(
	tv,
	"Tempo track config",
	TEMPO_TRACK_SETTINGS_LT_PATH,
	page_colors,
	&color_global_white,
	main_win);
    
    layout_force_reset(page->layout);

    PageElParams p;

    char label[255];
    if (s->next) {	
	snprintf(label, 255, "Segment from m%d to m%d", s->first_measure_index, s->next->first_measure_index);
    } else {
	snprintf(label, 255, "Segment from m%d to ∞", s->first_measure_index);
    }
    /* timecode_str_at(tt->tl, label + offset, 255 - offset, s->start_pos); */
    
    /* offset = strlen(label); */
    /* offset += snprintf(label + offset, 255 - offset, " - "); */
    /* timecode_str_at(tt->tl, label + offset, 255 - offset, s->end_pos); */

    p.textbox_p.font = main_win->mono_bold_font;
    p.textbox_p.text_size = 16;
    p.textbox_p.set_str = label;
    p.textbox_p.win = page->win;

    PageEl *el = page_add_el(page, EL_TEXTBOX, p, "", "segment_label");
    Textbox *tb = (Textbox *)el->component;
    textbox_set_align(tb, CENTER_LEFT);
    textbox_reset_full(tb);
    layout_size_to_fit_children_h(el->layout, true, 0);

    /* Add next/previous segment navigators */
    if (s->prev || s->next) {
	p.button_p.font = main_win->mono_font;
	p.button_p.win = page->win;
	p.button_p.target = NULL;
	p.button_p.text_color = &color_global_white;
	p.button_p.text_size = 16;
	p.button_p.background_color = &color_global_quickref_button_blue;
	p.button_p.target = s;

    }
    if (s->prev) {
	p.button_p.set_str = "←";
	p.button_p.action = segment_prev_action;
	page_add_el(page, EL_BUTTON, p, "segment_left", "segment_left");

    }
    if (s->next) {
	p.button_p.set_str = "→";
	p.button_p.action = segment_next_action;
	page_add_el(page, EL_BUTTON, p, "segment_right", "segment_right");
    }
    
    p.textbox_p.font = main_win->mono_font;
    p.textbox_p.text_size = 16;
    p.textbox_p.set_str = "Num beats:";
    p.textbox_p.win = page->win;

    el = page_add_el(page, EL_TEXTBOX, p, "num_beats_label", "num_beats_label");
    tb = (Textbox *)el->component;
    textbox_set_align(tb, CENTER_LEFT);
    textbox_reset_full(tb);

    p.textbox_p.set_str = "Subdivisions:";
    el = page_add_el(page, EL_TEXTBOX, p, "beat_subdiv_label", "beat_subdiv_label");
    tb = (Textbox *)el->component;
    textbox_set_align(tb, CENTER_LEFT);
    textbox_reset_full(tb);

    p.textbox_p.set_str = "Tempo (bpm):";
    el = page_add_el(page, EL_TEXTBOX, p, "tempo_label", "tempo_label");
    tb = (Textbox *)el->component;
    textbox_set_align(tb, CENTER_LEFT);
    textbox_reset_full(tb);


    int num_beats;
    if (set_from_cfg) {
	num_beats = s->cfg.num_beats;
	snprintf(tt->num_beats_str, 3, "%d", s->cfg.num_beats);
    } else {
	num_beats = atoi(tt->num_beats_str);
    }
    if (num_beats > MAX_BEATS_PER_BAR) {
	num_beats = MAX_BEATS_PER_BAR;
	snprintf(tt->num_beats_str, 3, "%d", num_beats);
	char errstr[128];
	snprintf(errstr, 128, "Cannot exceed %d beats per bar", num_beats);
	status_set_errstr(errstr);
    }
    p.textentry_p.font = main_win->bold_font;
    p.textentry_p.text_size = 16;
    p.textentry_p.value_handle = tt->num_beats_str;
    p.textentry_p.buf_len = 3;
    p.textentry_p.validation = txt_integer_validation;
    p.textentry_p.completion = num_beats_completion;
    p.textentry_p.completion_target = (void *)s;
    /* p.textbox_p.set_str = tt->num_beats_str; */
    /* p.textbox_p.font = main_win->std_font; */
    el = page_add_el(page, EL_TEXTENTRY, p, "tempo_segment_num_beats_value", "num_beats_value");
    Layout *num_beats_lt = el->layout;
    layout_size_to_fit_children_v(num_beats_lt, false, 1);
    layout_center_agnostic(num_beats_lt, false, true);
    textentry_reset(el->component);
    /* TextEntry *num_beats_te = el->component; */
    /* textentry_reset(num_beats_te); */
    /* (num_beats_te)->tb->text->max_len = 2; */

    Layout *subdiv_area = layout_get_child_by_name_recursive(page->layout, "beat_subdiv_values");
    for (int i=0; i<num_beats; i++) {
	int subdivs;
	if (set_from_cfg) {
	    subdivs = s->cfg.beat_subdiv_lens[i];
	} else {
	    subdivs = atoi(tt->subdiv_len_strs[i]);
	}
	snprintf(tt->subdiv_len_strs[i], 2, "%d", subdivs);
	Layout *child_l = layout_add_child(subdiv_area);
	child_l->x.type = STACK;
	child_l->h.type = SCALE;
	child_l->h.value = 1.0;
	/* child_l->y.value = 5; */
	/* child_l->h.type = PAD; */
	child_l->w.value = 30;
	
	char name[64];
	snprintf(name, 64, "beat_subdiv_lt_%d", i);
	layout_set_name(child_l, name);
	layout_force_reset(subdiv_area);
	
	p.textentry_p.value_handle = tt->subdiv_len_strs[i];
	p.textentry_p.buf_len = 2;
	p.textentry_p.text_size = 16;
	p.textentry_p.validation = txt_integer_validation;
	p.textentry_p.completion = txt_int_range_completion_1_9;
	p.textentry_p.completion_target = NULL;
	char buf[255];
	snprintf(buf, 255, "beat_%d_subdiv_len", i);
	el = page_add_el(page, EL_TEXTENTRY, p, buf, name);
	/* layout_size_to_fit_children_v(el->layout, false, 2); */
	/* layout_center_agnostic(el->layout, false, true); */

	/* ((TextEntry *)el->component)->tb->text->max_len = 2; */
	textentry_reset(el->component);
	if (i != num_beats - 1) {
	    Layout *child_r = layout_copy(child_l, child_l->parent);
	    child_r->w.value *= 0.75;
	    /* Layout *child_r = layout_add_child(child); */
	    snprintf(name, 64, "plus %d", i);
	    layout_set_name(child_r, name);
	    layout_force_reset(subdiv_area);
	    PageElParams pt;
	    pt.textbox_p.font = main_win->bold_font;
	    pt.textbox_p.text_size = 16;
	    pt.textbox_p.set_str = "+";
	    pt.textbox_p.win = page->win;

	    el = page_add_el(page, EL_TEXTBOX, pt, "", name);
	    /* textbox_set_pad(el->component, 24, 0); */
	    textbox_set_align(el->component, CENTER);
	    textbox_reset_full(el->component);
	}
    }


    /* Add canvas */
    Layout *time_sig_area = layout_get_child_by_name_recursive(page->layout, "time_signature_area");
    Layout *canvas_lt = layout_get_child_by_name_recursive(time_sig_area, "time_sig_canvas");
    p.canvas_p.draw_fn = draw_time_sig;
    p.canvas_p.draw_arg1 = (void *)tt;
    p.canvas_p.draw_arg2 = (void *)&canvas_lt->rect;
    el = page_add_el(
	page,
	EL_CANVAS,
	p,
	"time_sig_canvas",
	"time_sig_canvas"
	);


    /* Add tempo */
    snprintf(tt->tempo_str, 5, "%d", s->cfg.bpm);
    p.textentry_p.font = main_win->bold_font;
    p.textentry_p.text_size = 16;
    p.textentry_p.value_handle = tt->tempo_str;
    p.textentry_p.buf_len = 5;
    p.textentry_p.validation = txt_integer_validation;
    p.textentry_p.completion = NULL;
    /* p.textentry_p.completion = num_beats_completion; */
    /* p.textentry_p.completion_target = (void *)s; */
    /* p.textbox_p.set_str = tt->num_beats_str; */
    /* p.textbox_p.font = main_win->std_font; */
    el = page_add_el(page, EL_TEXTENTRY, p, "tempo_value", "tempo_value");
    Layout *tempo_lt = el->layout;
    /* tempo_lt->w.value = 50; */
    layout_size_to_fit_children_v(tempo_lt, false, 1);
    /* layout_center_agnostic(tempo_lt, false, true); */
    /* /\* num_beats_lt->w.value = 50; *\/ */
    /* layout_size_to_fit_children_v(num_beats_lt, false, 2); */
    /* layout_center_agnostic(num_beats_lt, false, true); */
    textentry_reset(el->component);
    




    /* Add end-bound behavior radio */
    if (s->next) {
	char opt1[64];
	char opt2[64];
	char timestr[64];
	timecode_str_at(tt->tl, timestr, 64, s->end_pos);
	snprintf(opt1, 64, "Fixed end pos (%s)", timestr);

	if (s->cfg.dur_sframes * s->num_measures == s->end_pos - s->start_pos) {
	    snprintf(opt2, 64, "Fixed num measures (%d)", s->num_measures);
	} else {
	    snprintf(opt2, 64, "Fixed num measures (%f)", (float)(s->end_pos - s->start_pos)/s->cfg.dur_sframes);
	}
	char *options[] = {opt1, opt2};

	p.radio_p.ep = &tt->end_bound_behavior_ep;
	/* p.radio_p.action = tempo_rb_action; */
	/* p.radio_p.target = (void *)&tt->end_bound_behavior; */
	p.radio_p.num_items = 2;
	p.radio_p.text_size = 16;
	p.radio_p.text_color = &color_global_white;
	p.radio_p.item_names = (const char **)options;

	el = page_add_el(
	    page,
	    EL_RADIO,
	    p,
	    "tempo_segment_ebb_radio",
	    "ebb_radio"
	    );

	radio_button_reset_from_endpoint(el->component);
	layout_reset(el->layout);
	layout_size_to_fit_children_v(el->layout, true, 0);
	layout_force_reset(el->layout);
	/* te->tb->text->max_len = TEMPO_STRLEN; */
    }


    /* layout_force_reset(page->layout); */

    layout_size_to_fit_children_v(time_sig_area, true, 0);
    
    /* Add submit button */
    p.button_p.action = time_sig_submit_button_action;
    p.button_p.target = (void *)s;
    p.button_p.font = main_win->mono_font;
    p.button_p.text_color = &color_global_white;
    p.button_p.text_size = 16;
    p.button_p.background_color = &color_global_quickref_button_blue;
    p.button_p.win = main_win;
    p.button_p.set_str = "Submit";
    el = page_add_el(
	page,
	EL_BUTTON,
	p,
	"time_signature_submit_button",
	"time_signature_submit");
    textbox_reset_full(((Button *)el->component)->tb);
    page_reset(page);
	
}

void tempo_track_populate_settings_tabview(TempoTrack *tt, TabView *tv)
{
    TempoSegment *s = tempo_track_get_segment_at_pos(tt, tt->tl->play_pos_sframes);
    tempo_track_populate_settings_internal(s, tv, true);
}

void timeline_tempo_track_edit(Timeline *tl)
{
    TempoTrack *tt = timeline_selected_tempo_track(tl);
    if (!tt) return;

    TabView *tv = tabview_create("Track Settings", proj->layout, main_win);
    tempo_track_populate_settings_tabview(tt, tv);
    /* p.textbox_p.font = main_win->mono_bold_font; */
    /* p.textbox_p.text_size = LABEL_STD_FONT_SIZE; */
    /* p.textbox_p.set_str = "Bandwidth:"; */
    /* p.textbox_p.win = page->win; */
    /* PageEl *el = page_add_el(page, EL_TEXTBOX, p, "track_settings_filter_bandwidth_label", "bandwidth_label"); */

    /* Textbox *tb = el->component; */
    /* textbox_set_align(tb, CENTER_LEFT); */
    /* textbox_reset_full(tb); */

    /* p.textbox_p.set_str = "Cutoff / center frequency:"; */
    /* p.textbox_p.win = main_win; */
    /* el = page_add_el(page, EL_TEXTBOX, p, "track_settings_filter_cutoff_label",  "cutoff_label"); */
    /* tb = el->component; */
    /* textbox_set_align(tb, CENTER_LEFT); */
    /* textbox_reset_full(tb); */
    
    /* p.textbox_p.set_str = "Impulse response length (\"sharpness\")"; */
    /* el = page_add_el(page, EL_TEXTBOX, p, "track_settings_filter_irlen_label", "irlen_label"); */
    /* tb=el->component; */
    /* textbox_set_trunc(tb, false); */
    /* textbox_set_align(tb, CENTER_LEFT); */
    /* textbox_reset_full(tb); */

    /* p.textbox_p.set_str = "Enable FIR filter"; */
    /* el = page_add_el(page, EL_TEXTBOX, p, "track_settings_filter_toggle_label", "toggle_label"); */
    /* tb=el->component; */
    /* textbox_set_trunc(tb, false); */
    /* textbox_set_align(tb, CENTER_LEFT); */
    /* textbox_reset_full(tb); */

    /* p.toggle_p.value = &track->fir_filter_active; */
    /* p.toggle_p.target = NULL; */
    /* p.toggle_p.action = NULL; */
    /* page_add_el(page, EL_TOGGLE, p, "track_settings_filter_toggle", "toggle_filter_on"); */

    tabview_activate(tv);
    tl->needs_redraw = true;


    /* TempoSegment *s = tempo_track_get_segment_at_pos(tt, tl->play_pos_sframes); */
    /* TODO: CREATE TABVIEW */
    
}
/* static void timeline_set_tempo_track_config_at_cursor(Timeline *tl, int num_measures, int bpm, int num_beats, uint8_t *subdiv_lens) */
/* { */
/*     TempoTrack *tt = timeline_selected_tempo_track(tl); */
/*     if (!tt) return; */
/*     TempoSegment *s = tempo_track_get_segment_at_pos(tt, tl->play_pos_sframes); */
/*     tempo_segment_set_config(s, num_measures, bpm, num_beats, subdiv_lens); */
/*     tl->needs_redraw = true; */
/* } */

static void simple_tempo_segment_remove(TempoSegment *s)
{
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
    /* fprintf(stderr, "After:\n"); */
    /* tempo_track_fprint(stderr, s->track); */
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
    tempo_segment_set_config(s, s->num_measures, new_tempo, s->cfg.num_beats, subdiv_lens, tt->end_bound_behavior);
    tl->needs_redraw = true;
}

void timeline_goto_prox_beat(Timeline *tl, int direction, enum beat_prominence bp)
{
    TempoTrack *tt = timeline_selected_tempo_track(tl);
    if (!tt) return;
    TempoSegment *s;
    
    int bar, beat, subdiv;
    int32_t pos = tl->play_pos_sframes;
    int safety = 0;
    tempo_track_bar_beat_subdiv(tt, pos, &bar, &beat, &subdiv, &s, false);
    pos = get_beat_pos(s, bar, beat, subdiv);
    if (direction > 0) {
	do_increment(s, &bar, &beat, &subdiv);
    }
    if (direction < 0 && tl->play_pos_sframes == pos) {
	if (pos <= s->start_pos && s->prev) {
	    tempo_track_bar_beat_subdiv(tt, pos - 1, &bar, &beat, &subdiv, &s, false);
	}
	do_decrement(s, &bar, &beat, &subdiv);
    }
    pos = get_beat_pos(s, bar, beat, subdiv);
    enum beat_prominence bp_test;
    set_beat_prominence(s, &bp_test, bar, beat, subdiv);
    
    while (safety < 10000 && bp_test > bp) {
	if (direction > 0) {
	    do_increment(s, &bar, &beat, &subdiv);
	} else {
	    do_decrement(s, &bar, &beat, &subdiv);
	}
	pos = get_beat_pos(s, bar, beat, subdiv);
	set_beat_prominence(s, &bp_test, bar, beat, subdiv);
	/* if (bp_test <= bp) { */
	/*     timeline_set_play_position(tl, pos);  */
	/*     return; */
	/* } */
	/* if (direction < 0) { */
	/*     pos -= (s->cfg.atom_dur_approx + 20); */
	/* } */
	/* /\* if (direction < 0) { *\/ */
	/* /\*     pos -= s->cfg.atom_dur_approx; *\/ */
	/* /\* } *\/ */
	/* safety++; */
	/* if (safety > 10000) { */
	/*     fprintf(stderr, "SAFE LIMIT EXCEEDED\n"); */
	/*     break; */
	/* } */
    }
    if (safety > 9999) {
	fprintf(stderr, "SAFETY ISSUE\n");
    }
    if (pos < s->start_pos) {
	pos = s->start_pos;
    } else if (pos > s->end_pos && s->start_pos != s->end_pos) {
	pos = s->end_pos;
    }

    timeline_set_play_position(tl, pos);
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
    /* SDL_Rect cliprect = *audio_rect; */
    /* cliprect.w += cliprect.x; */
    /* cliprect.x = 0; */
    if (audio_rect->y + audio_rect->h > tt->tl->layout->rect.y) {
	SDL_RenderSetClipRect(main_win->rend, audio_rect);
	textbox_draw(tt->metronome_button);
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
    textbox_draw(tt->edit_button);	

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

int send_message_udp(char *message, int port);

void tempo_track_mix_metronome(TempoTrack *tt, float *mixdown_buf, int32_t mixdown_buf_len, int32_t tl_start_pos_sframes, int32_t tl_end_pos_sframes, float step)
{
    if (tt->muted) return;
    if (step < 0.0) return;
    /* if (step > 10.0) return; */
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
	
	/* UDP -> Pd testing */
	/* if (tick_start_in_chunk > 0) { */
	/*     char message[255]; */
	/*     snprintf(message, 255, "%d %d;", tick_start_in_chunk, bp); */
	/*     send_message_udp(message, 5400); */
	/* } */

	
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
    /* slider_edit_made(tt->metronome_vol_slider); */
    slider_reset(tt->metronome_vol_slider);
}


void tempo_track_decrement_vol(TempoTrack *tt)
{
    tt->metronome_vol -= TRACK_VOL_STEP;
    if (tt->metronome_vol < 0.0f) {
	tt->metronome_vol = 0.0f;
    }
    /* slider_edit_made(tt->metronome_vol_slider); */
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
	if (SDL_PointInRect(&main_win->mousep, &t->edit_button->layout->rect)) {
	    timeline_tempo_track_edit(proj->timelines[proj->active_tl_index]);
	    return true;
	}
	return true;
    }
    TempoSegment *final = NULL;
    TempoSegment *s = t->segments;
    const int xdst_init = 5 * main_win->dpi_scale_factor;
    int xdst = xdst_init;
    while (s) {
	int xdst_test = abs(main_win->mousep.x - timeline_get_draw_x(s->track->tl, s->start_pos));
	if (xdst_test < xdst) {
	    final = s;
	    xdst = xdst_test;
	} else if (final) {
	    break;
	}
	s = s->next;
    }
    if (final) {
	proj->dragged_component.component = final;
	proj->dragged_component.type = DRAG_TEMPO_SEG_BOUND;
	return true;
    }

    
    return false;
}

void tempo_track_mouse_motion(TempoSegment *s, Window *win)
{
    if (!s->prev) return;
    s = s->prev;
    int32_t tl_pos = timeline_get_abspos_sframes(s->track->tl, win->mousep.x);
    if (tl_pos < s->start_pos + proj->sample_rate / 100) {
	return;
    }
    tempo_segment_set_end_pos(s, tl_pos);
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
