/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

#include "endpoint_callbacks.h"
#include "text.h"
#include "thread_safety.h"

/* #include <stdarg.h> */
#include "assets.h"
#include "color.h"
#include "components.h"
#include "input.h"
#include "layout.h"
#include "layout_xml.h"
#include "modal.h"
#include "project.h"
#include "session.h"
#include "tempo.h"
#include "timeline.h"
#include "timeview.h"
#include "user_event.h"
#include "wav.h"

#define MAX_BPM 999.0f
#define MIN_BPM 1.0f
#define DEFAULT_BPM 120.0f

extern Window *main_win;

extern struct colors colors;

void session_init_metronomes(Session *session)
{
    Metronome *m = &session->metronomes[0];
    m->name = "standard";
    float *L, *R;
    char *real_path = asset_get_abs_path(METRONOME_STD_HIGH_PATH);
    int32_t buf_len = wav_load(real_path, &L, &R);
    free(real_path);
    if (buf_len == 0) {
	fprintf(stderr, "Error: unable to load metronome buffer at %s\n", METRONOME_STD_LOW_PATH);
	exit(1);
    }

    free(R);

    m->buffers[0] = L;
    m->buf_lens[0] = buf_len;
    real_path = asset_get_abs_path(METRONOME_STD_LOW_PATH);
    buf_len = wav_load(real_path, &L, &R);
    free(real_path);
    if (buf_len == 0) {
	fprintf(stderr, "Error: unable to load metronome buffer at %s\n", METRONOME_STD_LOW_PATH);
	exit(1);
    }
    free(R);

    m->buffers[1] = L;
    m->buf_lens[1] = buf_len;
}

void session_destroy_metronomes(Session *session)
{
    for (int i=0; i<PROJ_NUM_METRONOMES; i++) {
	Metronome *m = session->metronomes + i;
	if (m->buffers[0])
	    free(m->buffers[0]);
	if (m->buffers[1])
	    free(m->buffers[1]);
    }
}

ClickSegment *click_track_get_segment_at_pos(ClickTrack *t, int32_t pos)
{
    ClickSegment *s = t->segments;
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
	    if (!s->next || pos < s->next->start_pos) {
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

static int32_t get_beat_pos(ClickSegment *s, int measure, int beat, int subdiv)
{
    
    int32_t pos = s->start_pos + (measure - s->first_measure_index) * s->cfg.dur_sframes;
    for (int i=0; i<beat; i++) {
	pos += s->cfg.dur_sframes * s->cfg.beat_subdiv_lens[i] / s->cfg.num_atoms;
    }
    pos += s->cfg.dur_sframes * subdiv / s->cfg.num_atoms;
    return pos;
}

static void do_increment(ClickSegment *s, int *measure, int *beat, int *subdiv)
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

static void do_decrement(ClickSegment *s, int *measure, int *beat, int *subdiv)
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


static void get_beat_prominence(ClickSegment *s, BeatProminence *bp, int measure, int beat, int subdiv)
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
static bool click_track_get_next_pos(ClickTrack *t, bool start, int32_t start_from, int32_t *pos, BeatProminence *bp, ClickSegment **seg_dst)
{
    static JDAW_THREAD_LOCAL ClickSegment *s;
    static JDAW_THREAD_LOCAL int beat = 0;
    static JDAW_THREAD_LOCAL int subdiv = 0;
    static JDAW_THREAD_LOCAL int measure = 0;
    if (start) {
	s = click_track_get_segment_at_pos(t, start_from);
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
	if (s->next && *pos >= s->next->start_pos) {
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
    if (seg_dst) {
	*seg_dst = s;
    }
    get_beat_prominence(s, bp, measure, beat, subdiv);
    return true;
}


void click_segment_set_start_pos(ClickSegment *s, int32_t new_end_pos);

void click_segment_set_config(ClickSegment *s, int num_measures, float bpm, uint8_t num_beats, uint8_t *subdivs, enum ts_end_bound_behavior ebb)
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
	/* s->end_pos = s->start_pos; */
	/* s->end_pos_internal = s->start_pos; */
	s->num_measures = -1;
    } else {
	if (ebb == SEGMENT_FIXED_NUM_MEASURES) {
	    double num_measures_f = (double)(s->next->start_pos - s->start_pos)/old_dur_sframes;
	    int32_t end_pos = s->start_pos + num_measures_f * s->cfg.dur_sframes;
	    click_segment_set_start_pos(s->next, end_pos);
	} else {
	    click_segment_set_start_pos(s->next, s->next->start_pos);
	}
    }
    /* if (s->track->end_bound_behavior == SEGMENT_FIXED_NUM_MEASURES) { */
	
    /* 	click_segment_set_end_pos(s, s->start_pos + measure_dur * s->num_measures); */
    /* } else { */
    /* 	if (num_measures > 0 && s->next) { */
    /* 	    s->end_pos = s->start_pos + num_measures * s->cfg.dur_sframes; */
    /* 	} else { */
    /* 	    s->end_pos = s->start_pos; */
    /* 	} */
    /* 	if (s->next) { */
    /* 	    click_segment_set_end_pos(s, s->next->start_pos); */
    /* 	} */
    /* } */
}

/* Calculates the new segment dur in measures and sets the first measure index of the next segment */
/* void click_segment_set_end_pos(ClickSegment *s, int32_t new_end_pos) */
/* { */
/*     int32_t segment_dur = new_end_pos - s->start_pos; */
/*     double num_measures = (double)segment_dur / s->cfg.dur_sframes; */
/*     s->num_measures = floor(0.9999999 + num_measures); */
/*     /\* s->end_pos = new_end_pos; *\/ */
/*     /\* s->end_pos_internal = new_end_pos; *\/ */
/*     if (s->next) { */
/* 	int32_t diff = new_end_pos - s->next->start_pos; */
/* 	s->next->start_pos = new_end_pos; */
/* 	click_segment_set_end_pos(s->next, s->next->start_pos + diff);  */
/*     } */
/* } */

static void bpm_proj_cb(Endpoint *ep)
{
    ClickSegment *s = ep->xarg1;
    click_segment_set_config(s, s->num_measures, ep->current_write_val.float_v, s->cfg.num_beats, s->cfg.beat_subdiv_lens, s->track->end_bound_behavior);
    if (session_get()->dragged_component.component == s) {
	label_move(s->bpm_label, main_win->mousep.x, s->track->layout->rect.y - 20);
    }
    label_reset(s->bpm_label, ep->current_write_val);
}

void click_segment_set_start_pos(ClickSegment *s, int32_t new_start_pos)
{
    int32_t diff = new_start_pos - s->start_pos;
    s->start_pos = new_start_pos;
    s->start_pos_internal = new_start_pos;
    if (s->prev) {
	int32_t prev_dur = new_start_pos - s->start_pos;
	double num_measures = (double)prev_dur / s->prev->cfg.dur_sframes;
	s->prev->num_measures = floor(0.9999999 + num_measures);
    }
    if (s->next) {
	click_segment_set_start_pos(s->next, s->next->start_pos + diff);
    }
}

ClickSegment *click_track_add_segment(ClickTrack *t, int32_t start_pos, int16_t num_measures, float bpm, uint8_t num_beats, uint8_t *subdiv_lens)
{
    ClickSegment *s = calloc(1, sizeof(ClickSegment));
    s->track = t;
    s->num_measures = num_measures;
    s->start_pos = start_pos;
    s->start_pos_internal = start_pos;
    endpoint_init(
	&s->start_pos_ep,
	&s->start_pos_internal,
	JDAW_INT32,
	"start_pos",
	"undo/redo move click segment boundary",
	JDAW_THREAD_MAIN,
	NULL, click_segment_bound_proj_cb, NULL,
	(void *)s, NULL, NULL, NULL);
    endpoint_init(
	&s->bpm_ep,
	&s->cfg.bpm,
	JDAW_FLOAT,
	"bpm",
	"BPM",
	JDAW_THREAD_MAIN,
	NULL, bpm_proj_cb, NULL,
	s, NULL, NULL, NULL);
    endpoint_set_default_value(&s->bpm_ep, (Value){.float_v = DEFAULT_BPM});
    endpoint_set_allowed_range(&s->bpm_ep, (Value){.float_v = MIN_BPM}, (Value){.float_v=MAX_BPM});
	

    s->bpm_label = label_create(BPM_STRLEN, t->layout, label_bpm, &s->cfg.bpm, JDAW_FLOAT, main_win);
    if (!t->segments) {
	t->segments = s;
	/* s->end_pos = s->start_pos; */
	s->first_measure_index = BARS_FOR_NOTHING * -1;
	goto set_config_and_exit;
    }
    ClickSegment *interrupt = click_track_get_segment_at_pos(t, start_pos);
    if (interrupt->start_pos == start_pos) {
	fprintf(stderr, "Error: cannot insert segment at existing segment loc\n");
	free(s);
	return NULL;
    }
    if (!interrupt->next) {
	interrupt->next = s;
	s->prev = interrupt;
	click_segment_set_start_pos(s, start_pos);
    } else {
	s->next = interrupt->next;
	interrupt->next->prev = s;
	interrupt->next = s;
	s->prev = interrupt;
	click_segment_set_config(s, num_measures, bpm, num_beats, subdiv_lens, SEGMENT_FIXED_END_POS);
	click_segment_set_start_pos(s, start_pos);
	return s;
    }
set_config_and_exit:
    click_segment_set_config(s, num_measures, bpm, num_beats, subdiv_lens, SEGMENT_FIXED_END_POS); 
    return s;
}

ClickSegment *click_track_add_segment_at_measure(ClickTrack *t, int16_t measure, int16_t num_measures, int bpm, uint8_t num_beats, uint8_t *subdiv_lens)
{
    int32_t start_pos = 0;
    ClickSegment *s = t->segments;
    if (s) {
	while (1) {

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
    s = click_track_add_segment(t, start_pos, num_measures, bpm, num_beats, subdiv_lens);
    return s;
}

static void click_track_remove(ClickTrack *tt)
{
    Timeline *tl = tt->tl;
    for (int i=tt->index; i<tl->num_click_tracks - 1; i++) {
	tl->click_tracks[i] = tl->click_tracks[i+1];
    }
    tl->num_click_tracks--;
    layout_remove_child(tt->layout);
    timeline_rectify_track_indices(tl);
    timeline_rectify_track_area(tl);

}

static void click_track_reinsert(ClickTrack *tt)
{
    Timeline *tl = tt->tl;
    for (int i=tl->num_click_tracks; i>tt->index; i--) {
	tl->click_tracks[i] = tl->click_tracks[i-1];
    }
    tl->click_tracks[tt->index] = tt;
    tl->num_click_tracks++;
    layout_insert_child_at(tt->layout, tl->track_area, tt->layout->index);
    tl->layout_selector = tt->layout->index;
    timeline_rectify_track_indices(tl);
    timeline_rectify_track_area(tl);
}

NEW_EVENT_FN(undo_add_click_track, "undo add tempo track")
    ClickTrack *tt = obj1;
    click_track_remove(tt);

}
NEW_EVENT_FN(redo_add_click_track, "redo add tempo track")
    ClickTrack *tt = obj1;
    click_track_reinsert(tt);
}
 
NEW_EVENT_FN(dispose_forward_add_click_track, "")
    click_track_destroy((ClickTrack *)obj1);
}

ClickTrack *timeline_add_click_track(Timeline *tl)
{
    if (tl->num_click_tracks == MAX_CLICK_TRACKS) return NULL;
    ClickTrack *t = calloc(1, sizeof(ClickTrack));
    snprintf(t->name, MAX_NAMELENGTH, "click_track_%d", tl->num_click_tracks + 1);
    t->tl = tl;
    Session *session = session_get();
    t->metronome = &session->metronomes[0];
    snprintf(t->num_beats_str, 3, "4");
    for (int i=0; i<MAX_BEATS_PER_BAR; i++) {
	snprintf(t->subdiv_len_strs[i], 2, "4");
    }    

    endpoint_init(
	&t->metronome_vol_ep,
	&t->metronome_vol,
	JDAW_FLOAT,
	"metro_vol",
	"undo/redo adj metronome vol",
	JDAW_THREAD_MAIN,
	track_slider_cb, NULL, NULL,
	(void *)&t->metronome_vol_slider, (void *)tl, NULL, NULL);
    endpoint_set_allowed_range(
	&t->metronome_vol_ep,
	(Value){.float_v = 0.0f},
	(Value){.float_v = TRACK_VOL_MAX});

    endpoint_init(
	&t->end_bound_behavior_ep,
	&t->end_bound_behavior,
	JDAW_INT,
	"segment_end_bound_behavior",
	"click track segment end-bound behavior",
	JDAW_THREAD_MAIN,
        click_track_ebb_gui_cb, NULL, NULL,
	NULL, NULL, NULL, NULL);
    
    endpoint_set_allowed_range(
	&t->end_bound_behavior_ep,
	(Value){.int_v = 0},
	(Value){.int_v = 1});

    
    /* Layout *click_tracks_area = layout_get_child_by_name_recursive(tl->layout, "tracks_area"); */
    Layout *click_tracks_area = tl->track_area;
    if (!click_tracks_area) {
	fprintf(stderr, "Error: no tempo tracks area\n");
	exit(1);
    }
    /* Layout *lt = layout_read_xml_to_lt(click_tracks_area, CLICK_TRACK_LT_PATH); */
    Layout *lt = layout_read_from_xml(CLICK_TRACK_LT_PATH);
    if (tl->layout_selector >= 0) {
	layout_insert_child_at(lt, click_tracks_area, tl->layout_selector);
    } else {
	layout_insert_child_at(lt, click_tracks_area, 0);
    }

    t->layout = lt;
    layout_size_to_fit_children_v(click_tracks_area, true, 0);
    layout_reset(tl->track_area);
    tl->needs_redraw = true;

    layout_force_reset(lt);

    Layout *tb_lt = layout_get_child_by_name_recursive(t->layout, "display");
    snprintf(t->pos_str, CLICK_POS_STR_LEN, "0.0.0:00000");
    t->readout = textbox_create_from_str(
	t->pos_str,
	tb_lt,
	main_win->mono_bold_font,
	14,
	main_win);
    textbox_set_align(t->readout, CENTER_LEFT);
    textbox_set_background_color(t->readout, &colors.black);
    textbox_set_text_color(t->readout, &colors.white);
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
    textbox_set_text_color(t->edit_button, &colors.white);
    textbox_set_background_color(t->edit_button, &colors.quickref_button_blue);
    /* textbox_set_border(t->edit_button, &colors.black, 1); */
    textbox_set_border(t->edit_button, &colors.white, 1, BUTTON_CORNER_RADIUS);
    
    Layout *metro_button_lt = layout_get_child_by_name_recursive(t->layout, "metronome_button");
    t->metronome_button = textbox_create_from_str(
	"M",
	metro_button_lt,
	main_win->bold_font,
	14,
	main_win);
    textbox_set_background_color(t->metronome_button, &colors.play_green);
    textbox_set_border(t->metronome_button, &colors.black, 1, MUTE_SOLO_BUTTON_CORNER_RADIUS);
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
	NULL,
	/* NULL, */
	/* /\* &slider_label_amp_to_dbstr, *\/ */
	/* NULL, */
	/* NULL, */
	/* NULL, */
	&session->dragged_component);
    slider_reset(t->metronome_vol_slider);
    /* Value min, max; */
    /* min.float_v = 0.0f; */
    /* max.float_v = TRACK_VOL_MAX; */
    /* slider_set_range(t->metronome_vol_slider, min, max); */
    /* slider_reset(t->metronome_vol_slider); */

    uint8_t subdivs[] = {4, 4, 4, 4};
    click_track_add_segment(t, 0, -1, DEFAULT_BPM, 4, subdivs);
    
    tl->click_tracks[tl->num_click_tracks] = t;
    t->index = tl->num_click_tracks;
    tl->num_click_tracks++;
    snprintf(t->name, MAX_NAMELENGTH, "Click track %d\n", t->index + 1);

    timeline_rectify_track_indices(tl);
    timeline_rectify_track_area(tl);
    
    /* timeline_reset(tl, false); */
    /* layout_force_reset(tl->layout); */
    /* layout_reset(lt); */
    Value nullval = {.int_v = 0};
    user_event_push(
	undo_add_click_track,
	redo_add_click_track,
	NULL,
	dispose_forward_add_click_track,
	(void *)t, NULL,
	nullval, nullval,
	nullval, nullval,
	0, 0, false, false);


    /* timeline_reset(tl, false); */
    return t;
}

/* Destroy functions */

void click_segment_destroy(ClickSegment *s)
{
    label_destroy(s->bpm_label);
    free(s);
}

void click_track_destroy(ClickTrack *tt)
{
    textbox_destroy(tt->metronome_button);
    textbox_destroy(tt->edit_button);
    slider_destroy(tt->metronome_vol_slider);
    
    ClickSegment *s = tt->segments;
    while (s) {
	ClickSegment *next = s->next;
	click_segment_destroy(s);
	s = next;
    }
    layout_destroy(tt->layout);
}



/* utility functions */
/* static void click_segment_set_tempo(ClickSegment *s, unsigned int new_tempo, bool from_undo); */
/* static void click_segment_set_tempo(ClickSegment *s, double new_tempo, enum ts_end_bound_behavior ebb,  bool from_undo); */

/* NEW_EVENT_FN(undo_redo_set_tempo, "undo/redo set tempo") */
/*     ClickSegment *s = (ClickSegment *)obj1; */
/*     double tempo = val1.double_v; */
/*     enum ts_end_bound_behavior ebb = (enum ts_end_bound_behavior)val2.int_v; */
/*     click_segment_set_tempo(s, tempo, ebb, true); */
/* } */


/* static void click_segment_set_tempo(ClickSegment *s, double new_tempo, enum ts_end_bound_behavior ebb,  bool from_undo) */
/* { */
/*     if (new_tempo < MIN_BPM) { */
/* 	char errstr[64]; */
/* 	snprintf(errstr, 64, "Tempo cannot be < %.1f bpm", MIN_BPM); */
/* 	status_set_errstr(errstr); */
/* 	return; */
/*     } */
/*     if (new_tempo > MAX_BPM) { */
/* 	char errstr[64]; */
/* 	snprintf(errstr, 64, "Tempo cannot be > %.1f bpm", MAX_BPM); */
/* 	status_set_errstr(errstr); */
/* 	return; */
/*     } */
/*     double old_tempo = s->cfg.bpm; */
/*     /\* float num_measures; *\/ */
/*     /\* if (s->next && ebb == SEGMENT_FIXED_NUM_MEASURES) { *\/ */
/*     /\* 	if (s->cfg.dur_sframes * s->num_measures == s->end_pos - s->start_pos) { *\/ */
/*     /\* 	    num_measures = s->num_measures; *\/ */
/*     /\* 	} else { *\/ */
/*     /\* 	    num_measures = ((float)s->end_pos - s->start_pos) / s->cfg.dur_sframes; *\/ */
/*     /\* 	} *\/ */

/*     /\* } *\/ */
/*     uint8_t subdiv_lens[s->cfg.num_beats]; */
/*     memcpy(subdiv_lens, s->cfg.beat_subdiv_lens, s->cfg.num_beats * sizeof(uint8_t)); */
/*     click_segment_set_config(s, s->num_measures, new_tempo, s->cfg.num_beats, subdiv_lens, ebb); */

/*     /\* if (s->next && ebb == SEGMENT_FIXED_NUM_MEASURES) { *\/ */
/*     /\* 	click_segment_set_end_pos(s, s->start_pos + num_measures * s->cfg.dur_sframes); *\/ */
/*     /\* } *\/ */
    
/*     s->track->tl->needs_redraw = true; */
/*     if (!from_undo) { */
/* 	Value old_val = {.double_v = old_tempo}; */
/* 	Value new_val = {.double_v = new_tempo}; */
/* 	Value ebb = {.int_v = (int)(s->track->end_bound_behavior)}; */
/* 	user_event_push( */
/* 	    undo_redo_set_tempo, */
/* 	    undo_redo_set_tempo, */
/* 	    NULL, NULL, */
/* 	    (void *)s, NULL, */
/* 	    old_val, ebb, */
/* 	    new_val, ebb, */
/* 	    0, 0, false, false); */
/*     } */
/* } */

	

/* MID-LEVEL INTERFACE */

static int set_tempo_submit_form(void *mod_v, void *target)
{
    Modal *mod = (Modal *)mod_v;
    ClickSegment *s = (ClickSegment *)mod->stashed_obj;
    for (uint8_t i=0; i<mod->num_els; i++) {
	ModalEl *el = mod->els[i];
	if (el->type == MODAL_EL_TEXTENTRY) {
	    char *value = ((TextEntry *)el->obj)->tb->text->value_handle;
	    /* TODO: FIX THIS */
	    /* float tempo = atoi(value); */
	    float tempo = txt_float_from_str(value);
	    endpoint_write(&s->bpm_ep, (Value){.float_v = tempo}, true, true, true, true);
	    /* click_segment_set_tempo(s, tempo, s->track->end_bound_behavior, false); */
	    break;
	}
    }
    window_pop_modal(main_win);
    Session *session = session_get();
    ACTIVE_TL->needs_redraw = true;
    return 0;
}


#define TEMPO_STRLEN 8
void timeline_click_track_set_tempo_at_cursor(Timeline *tl)
{
    ClickTrack *tt = timeline_selected_click_track(tl);
    if (!tt) {
	status_set_errstr("No tempo track selected");
	return;
    }
    ClickSegment *s = click_track_get_segment_at_pos(tt, tl->play_pos_sframes);
    Layout *mod_lt = layout_add_child(main_win->layout);
    layout_set_default_dims(mod_lt);
    Modal *mod = modal_create(mod_lt);
    static char tempo_str[TEMPO_STRLEN];
    snprintf(tempo_str, TEMPO_STRLEN, "%f", s->cfg.bpm);
    modal_add_header(mod, "Set tempo:", &colors.light_grey, 4);
    ModalEl *el = modal_add_textentry(
	mod,
	tempo_str,
	TEMPO_STRLEN,
	txt_float_validation,
	/* txt_integer_validation, */
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
	timecode_str_at(tt->tl, timestr, 64, s->next->start_pos);
	snprintf(opt1, 64, "Fixed end pos (%s)", timestr);

	if (s->cfg.dur_sframes * s->num_measures == s->next->start_pos - s->start_pos) {
	    snprintf(opt2, 64, "Fixed num measures (%d)", s->num_measures);
	} else {
	    snprintf(opt2, 64, "Fixed num measures (%f)", (float)(s->next->start_pos - s->start_pos)/s->cfg.dur_sframes);
	}
	char *options[] = {opt1, opt2};

	
	/* TODO: FIX THIS */
	el = modal_add_radio(
	    mod,
	    &colors.white,
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
    ((Button *)el->obj)->target = s;

    layout_reset(mod->layout);
    layout_center_agnostic(el->layout, true, false);

    mod->stashed_obj = (void *)s;
    mod->submit_form = set_tempo_submit_form;
    window_push_modal(main_win, mod);

    modal_reset(mod);
    modal_move_onto(mod);
    tl->needs_redraw = true;
 }

static void click_track_delete(ClickTrack *tt, bool from_undo);

NEW_EVENT_FN(undo_delete_click_track, "undo delete tempo track")
    ClickTrack *tt = (ClickTrack *)obj1;
    click_track_reinsert(tt);
}

NEW_EVENT_FN(redo_delete_click_track, "redo delete tempo track")
    ClickTrack *tt = (ClickTrack *)obj1;
    click_track_delete(tt, true);
}

NEW_EVENT_FN(dispose_delete_click_track, "")
    ClickTrack *tt = (ClickTrack *)obj1;
    click_track_destroy(tt);
}

static void click_track_delete(ClickTrack *tt, bool from_undo)
{
    click_track_remove(tt);
    Value nullval = {.int_v = 0};
    if (!from_undo) {
	user_event_push(
	    undo_delete_click_track,
	    redo_delete_click_track,
	    dispose_delete_click_track,
	    NULL,
	    (void *)tt, NULL,
	    nullval, nullval, nullval, nullval,
	    0, 0, false, false);	    
    }
}

bool timeline_click_track_delete(Timeline *tl)
{
    ClickTrack *tt = timeline_selected_click_track(tl);
    if (!tt) return false;
    if (tt == tl->click_tracks[0] && tl->click_track_frozen) {
	check_unfreeze_click_track(tl);
    }
    click_track_delete(tt, false);
    timeline_reset(tl, false);
    return true;
}


/* static void timeline_set_click_track_config_at_cursor(Timeline *tl, int num_measures, int bpm, int num_beats, uint8_t *subdiv_lens) */
/* { */
/*     ClickTrack *tt = timeline_selected_click_track(tl); */
/*     if (!tt) return; */
/*     ClickSegment *s = click_track_get_segment_at_pos(tt, tl->play_pos_sframes); */
/*     click_segment_set_config(s, num_measures, bpm, num_beats, subdiv_lens); */
/*     tl->needs_redraw = true; */
/* } */

/* static void simple_click_segment_reinsert(ClickSegment *s, int32_t at) */
/* { */


/*     ClickSegment *insertion = click_track_get_segment_at_pos(s->track, at); */
/*     insertion->next = s; */
    
/* } */

static void simple_click_segment_remove(ClickSegment *s)
{
    if (s == s->track->segments && !s->next) return;
    ClickTrack *tt = s->track;
    if (s->prev) {
	s->prev->next = s->next;
    } else {
	tt->segments = s->next;
    }
    if (s->next) {
	s->next->prev = s->prev;
	click_segment_set_start_pos(s->next, s->next->start_pos);
    }
}

static void simple_click_segment_reinsert(ClickSegment *s, int32_t segment_dur)
{
    if (!s->prev) {
	s->track->segments = s;
    } else {
	s->prev->next = s;
    }
    if (s->next) {
	s->next->prev = s;
	click_segment_set_start_pos(s->next, s->start_pos + segment_dur);
    }
    /* if ( */
    click_segment_set_start_pos(s, s->start_pos);
}



ClickSegment *click_track_cut_at(ClickTrack *tt, int32_t at)
{
    ClickSegment *s = click_track_get_segment_at_pos(tt, at);
    uint8_t subdiv_lens[s->cfg.num_beats];
    memcpy(subdiv_lens, s->cfg.beat_subdiv_lens, s->cfg.num_beats * sizeof(uint8_t));
    return click_track_add_segment(tt, at, -1, s->cfg.bpm, s->cfg.num_beats, subdiv_lens);
}


NEW_EVENT_FN(undo_cut_click_track, "undo cut click track")
    ClickTrack *tt = (ClickTrack *)obj1;
    ClickSegment *s = (ClickSegment *)obj2;
    /* int32_t at = val1.int32_v; */
    /* s = click_track_get_segment_at_pos(tt, at); */
    simple_click_segment_remove(s);
    tt->tl->needs_redraw = true;
}

NEW_EVENT_FN(redo_cut_click_track, "redo cut click track")
    ClickTrack *tt = (ClickTrack *)obj1;
    /* int32_t at = val1.int32_v; */
    simple_click_segment_reinsert(obj2, val2.int32_v);
    tt->tl->needs_redraw = true;
    /* ClickSegment *s = click_track_cut_at(tt, at); */
    /* self->obj2 = (void *)s; */
}

NEW_EVENT_FN(dispose_forward_cut_click_track, "")
    click_segment_destroy((ClickSegment *)obj2);
}


void timeline_cut_click_track_at_cursor(Timeline *tl)
{
    if (tl->play_pos_sframes < 0) return;
    ClickTrack *tt = timeline_selected_click_track(tl);
    if (!tt) return;
    ClickSegment *s = click_track_cut_at(tt, tl->play_pos_sframes);
    if (!s) {
	status_set_errstr("Error: cannot cut at existing segment boundary");
	return;
    }
    tl->needs_redraw = true;
    Value cut_pos = {.int32_v = tl->play_pos_sframes};
    Value new_seg_duration = {.int32_v = s->next ? s->next->start_pos - s->start_pos : -1};
    user_event_push(
	undo_cut_click_track, redo_cut_click_track,
	NULL, dispose_forward_cut_click_track,
	(void *)tt, (void *)s,
	cut_pos, new_seg_duration,
	cut_pos, new_seg_duration,
	0, 0, false, false);
}

void timeline_increment_click_at_cursor(Timeline *tl, int inc_by)
{
    ClickTrack *tt = timeline_selected_click_track(tl);
    if (!tt) return;
    ClickSegment *s = click_track_get_segment_at_pos(tt, tl->play_pos_sframes);
    int new_tempo = s->cfg.bpm + inc_by;
    uint8_t subdiv_lens[s->cfg.num_beats];
    memcpy(subdiv_lens, s->cfg.beat_subdiv_lens, s->cfg.num_beats * sizeof(uint8_t));
    click_segment_set_config(s, s->num_measures, new_tempo, s->cfg.num_beats, subdiv_lens, tt->end_bound_behavior);
    tl->needs_redraw = true;
}

void click_track_get_prox_beats(ClickTrack *ct, int32_t pos, BeatProminence bp, int32_t *prev_pos_dst, int32_t *next_pos_dst)
{
    ClickSegment *s;    
    int bar, beat, subdiv;
    BeatProminence bp_test = bp + 1;
    click_track_bar_beat_subdiv(ct, pos, &bar, &beat, &subdiv, &s, false);
    do {
	get_beat_prominence(s, &bp_test, bar, beat, subdiv);
	do_decrement(s, &bar, &beat, &subdiv);
    } while (bp_test > bp);
    do_increment(s, &bar, &beat, &subdiv);
    
    int32_t prev_pos = get_beat_pos(s, bar, beat, subdiv);
    do_increment(s, &bar, &beat, &subdiv);
    do {
	get_beat_prominence(s, &bp_test, bar, beat, subdiv);
	do_increment(s, &bar, &beat, &subdiv);
    } while (bp_test > bp);
    do_decrement(s, &bar, &beat, &subdiv);

    int32_t next_pos = get_beat_pos(s, bar, beat, subdiv);
    *prev_pos_dst = prev_pos;
    *next_pos_dst = next_pos;
    return;
}

void click_track_goto_prox_beat(ClickTrack *tt, int direction, BeatProminence bp)
{

    Timeline *tl = tt->tl;
    ClickSegment *s;
    
    int bar, beat, subdiv;
    int32_t pos = tl->play_pos_sframes;
    int safety = 0;
    click_track_bar_beat_subdiv(tt, pos, &bar, &beat, &subdiv, &s, false);
    pos = get_beat_pos(s, bar, beat, subdiv);
    if (direction > 0) {
	do_increment(s, &bar, &beat, &subdiv);
    }
    if (direction < 0 && tl->play_pos_sframes == pos) {
	if (pos <= s->start_pos && s->prev) {
	    click_track_bar_beat_subdiv(tt, pos - 1, &bar, &beat, &subdiv, &s, false);
	}
	do_decrement(s, &bar, &beat, &subdiv);
    }
    pos = get_beat_pos(s, bar, beat, subdiv);
    BeatProminence bp_test;
    get_beat_prominence(s, &bp_test, bar, beat, subdiv);
    
    while (safety < 10000 && bp_test > bp) {
	if (direction > 0) {
	    do_increment(s, &bar, &beat, &subdiv);
	} else {
	    do_decrement(s, &bar, &beat, &subdiv);
	}
	pos = get_beat_pos(s, bar, beat, subdiv);
	get_beat_prominence(s, &bp_test, bar, beat, subdiv);
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
    } else if (s->next && pos > s->next->start_pos) {
	pos = s->next->start_pos;
    }

    timeline_set_play_position(tl, pos, true);
}

/* static BeatProminence timeline_get_beat_pos_range(Timeline *tl, int32_t *pos) */
/* { */
/*     ClickTrack *tt = timeline_selected_click_track(tl); */
/*     if (!tt) return BP_NONE; */
/*     ClickSegment *s = click_track_get_segment_at_pos(tt, tl->play_pos_sframes); */
/*     int32_t pos_n = s->start_pos; */
/*     int32_t pos_m; */
/*     while ((pos_m = pos_n + s->cfg.dur_sframes) >= *pos) { */
/* 	pos_n = pos_m; */
/*     } */
/*     BeatProminence bp; */
/*     int bar, beat, subdiv; */
/*     bar = beat = subdiv = 0; */
/*     set_beat_prominence(s, &bp, bar, beat, subdiv); */
    
/*     return bp; */
    
/*     /\* timeline_move_play_position(tl, pos_m); *\/ */
/*     /\* /\\* tl->needs_redraw = true; *\\/ *\/ */
/* } */

/* void click_track_fill_metronome_buffer(ClickTrack *tt, float *L, float *R, int32_t start_from) */
/* { */
/*     static ClickSegment *s = NULL; */
    
/*     if (!s) { */
/* 	s = click_track_get_segment_at_pos(tt, start_from); */
/*     } else { */
/* 	while (s->next && start_from < s->start_pos) { */
/* 	    s = s->next; */
/* 	} */
/*     } */
/* } */

static void click_track_deferred_draw(void *click_track_v)
{
    ClickTrack *tt = (ClickTrack *)click_track_v;
    Session *session = session_get();
    SDL_Rect *audio_rect = session->gui.audio_rect;
    /* SDL_Rect cliprect = *audio_rect; */
    /* cliprect.w += cliprect.x; */
    /* cliprect.x = 0; */
    if (audio_rect->y + audio_rect->h > tt->tl->track_area->rect.y) {
	/* SDL_RenderSetClipRect(main_win->rend, audio_rect); */
	textbox_draw(tt->metronome_button);
	slider_draw(tt->metronome_vol_slider);
	/* SDL_RenderSetClipRect(main_win->rend, NULL); */
    }
}

/* not-DRY code for piano roll */
void click_track_draw_segments(ClickTrack *tt, TimeView *tv, SDL_Rect draw_rect)
{

    static const SDL_Color line_colors[] =  {
	{255, 250, 125, 40},
	{255, 255, 255, 40},
	{170, 170, 170, 40},
	{130, 130, 130, 40},
	{100, 100, 100, 40}
    };

    Timeline *tl = tt->tl;
    int32_t pos = tv->offset_left_sframes;
    BeatProminence bp;
    click_track_get_next_pos(tt, true, pos, &pos, &bp, NULL);
    int x;
    int top_y = draw_rect.y;
    int h = draw_rect.h;
    int bottom_y = top_y + h;
    int prev_draw_x = -100;
    const int subdiv_draw_thresh = 10;
    const int beat_draw_thresh = 4;
    int max_bp = BP_NONE;
    
    while (1) {

	if (bp == BP_SEGMENT) prev_draw_x = -100;
	/* int prev_x = x; */
	x = timeline_get_draw_x(tl, pos);
	if (x > draw_rect.x + draw_rect.w) break;
	if (bp >= max_bp) {
	    goto reset_pos_and_bp;
	}
	    
	if (max_bp > BP_BEAT) {
	    /* fprintf(stderr, "DIFF: %d\n", x - prev_draw_x); */
	    if (x - prev_draw_x < beat_draw_thresh) {
		max_bp = BP_BEAT;
	    } else if (x - prev_draw_x < subdiv_draw_thresh) {
		max_bp = BP_SUBDIV;
	    }
	    /* fprintf(stderr, "x %d->%d (max bp %d)\n", prev_draw_x, x, max_bp); */
	    prev_draw_x = x;
	    if (bp >= max_bp) {
		goto reset_pos_and_bp;
	    }
	}
	/* int x_diff; */
	/* if (draw_beats && (x_diff = x - prev_x) <= draw_beat_thresh) { */
	/*     draw_beats = false; */
	/*     draw_subdivs = false; */
	/* } else if (draw_subdivs && x_diff <= draw_subdiv_thresh) { */
	/*     draw_subdivs = false; */
	/* } */
	/* if (!draw_subdivs && (bp >= BP_SUBDIV)) continue; */
	/* if (!draw_beats && (bp >= BP_BEAT)) continue; */
	/* top_y = main_top_y + h * (int)bp / 5; */
	/* top_y = main_top_y + h / (1 + (int)bp); */
	SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(line_colors[(int)bp]));
	double dpi = main_win->dpi_scale_factor;
	if (bp == BP_SEGMENT) {
	    SDL_Rect seg = {x - dpi, top_y, dpi * 2, h};
	    SDL_RenderFillRect(main_win->rend, &seg);

	    /* double dpi = main_win->dpi_scale_factor; */
	    /* SDL_RenderDrawLine(main_win->rend, x-dpi, top_y, x-dpi, bttm_y); */
	    /* SDL_RenderDrawLine(main_win->rend, x+dpi, top_y, x+dpi, bttm_y); */
	} else {
	    SDL_RenderDrawLine(main_win->rend, x, top_y, x, bottom_y);
	}
    reset_pos_and_bp:
	click_track_get_next_pos(tt, false, tv->offset_left_sframes, &pos, &bp, NULL);
    }

}

void click_track_draw(ClickTrack *tt)
{
    Timeline *tl = tt->tl;
    SDL_Rect ttr = tt->layout->rect;
    
    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(colors.click_track));
    SDL_RenderFillRect(main_win->rend, &ttr);

    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(colors.black));
    SDL_RenderFillRect(main_win->rend, tt->console_rect);
    
    static SDL_Color line_colors[] =  {
	{255, 250, 125, 255},
	{255, 255, 255, 255},
	{170, 170, 170, 255},
	{130, 130, 130, 255},
	{100, 100, 100, 255}
    };

    int32_t pos = tt->tl->timeview.offset_left_sframes;
    BeatProminence bp;
    ClickSegment *s;
    const static int MAX_BPM_LABELS_TO_DRAW = 64;
    Label *bpm_labels_to_draw[128];
    int num_bpm_labels_to_draw = 0;

    click_track_get_next_pos(tt, true, pos, &pos, &bp, &s);
    bpm_labels_to_draw[num_bpm_labels_to_draw] = s->bpm_label;
    num_bpm_labels_to_draw++;

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
	click_track_get_next_pos(tt, false, 0, &pos, &bp, &s);
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
	    if (num_bpm_labels_to_draw < MAX_BPM_LABELS_TO_DRAW) {
		bpm_labels_to_draw[num_bpm_labels_to_draw] = s->bpm_label;
		num_bpm_labels_to_draw++;
	    }

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
    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(colors.control_bar_background_grey));
    SDL_RenderFillRect(main_win->rend, tt->right_console_rect);

    /* Draw right console elements */
    click_track_deferred_draw(tt);
    /* window_defer_draw(main_win, click_track_deferred_draw, tt); */

    /* Draw outline */
    SDL_SetRenderDrawColor(main_win->rend, 100, 100, 100, 255);
    SDL_RenderDrawRect(main_win->rend, &ttr);
    SDL_RenderFillRect(main_win->rend, tt->colorbar_rect);
    SDL_RenderFillRect(main_win->rend, tt->right_colorbar_rect);

    for (int i=0; i<num_bpm_labels_to_draw; i++) {
	label_draw(bpm_labels_to_draw[i]);
    }

}

/* Simple stateless version of click_track_bar_beat_subdiv() */
ClickTrackPos click_track_get_pos(ClickTrack *ct, int32_t tl_pos)
{
    ClickSegment *seg = click_track_get_segment_at_pos(ct, tl_pos);
    int32_t pos_in_seg = tl_pos - seg->start_pos;
    int measure = (pos_in_seg / seg->cfg.dur_sframes) + seg->first_measure_index;
    int beat = 0;
    int subdiv = 0;
    int32_t beat_pos;
    do {
	beat_pos = get_beat_pos(seg, measure, beat, subdiv);
	do_increment(seg, &measure, &beat, &subdiv);
    } while (beat_pos < tl_pos);
    do_decrement(seg, &measure, &beat, &subdiv);
    if (beat_pos > tl_pos) 
	do_decrement(seg, &measure, &beat, &subdiv);
    ClickTrackPos ret;
    ret.seg = seg;
    ret.measure = measure;
    ret.beat = beat;
    ret.subdiv = subdiv;
    ret.remainder = get_beat_pos(seg, measure, beat, subdiv) - tl_pos;
    return ret;
}

/* sets bar, beat, and pos; returns remainder in sframes */
int32_t click_track_bar_beat_subdiv(ClickTrack *tt, int32_t pos, int *bar_p, int *beat_p, int *subdiv_p, ClickSegment **segment_p, bool set_readout)
{
    /* bool debug = false; */
    /* if (strcmp(get_thread_name(), "main") == 0) { */
    /* 	debug = true; */
    /* } */
    /* fprintf(stderr, "CALL TO bar beat subdiv in thread %s\n", get_thread_name()); */
    static JDAW_THREAD_LOCAL int measures[MAX_CLICK_TRACKS] = {0};
    static JDAW_THREAD_LOCAL int beats[MAX_CLICK_TRACKS] = {0};
    static JDAW_THREAD_LOCAL int subdivs[MAX_CLICK_TRACKS] = {0};
    /* static JDAW_THREAD_LOCAL int32_t prev_positions[MAX_CLICK_TRACKS]; */
    
    int measure = measures[tt->index];
    int beat = beats[tt->index];
    int subdiv = subdivs[tt->index];

    ClickSegment *s = click_track_get_segment_at_pos(tt, pos);
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
    
    /* #ifdef TESTBUILD */
    int ops = 0;
    /* #endif */
    
    int32_t remainder = 0;
    /* if (debug) { */
    /* 	/\* fprintf(stderr, "\t\tSTART loop\n"); *\/ */
    /* } */
    while (1) {
	
	
	beat_pos = get_beat_pos(s, measure, beat, subdiv);
	remainder = pos - beat_pos;

	/* #ifdef TESTBUILD */
	ops++;
	if (ops > 1000000 - 5) {
	    if (ops > 1000000 - 3) breakfn();
	    fprintf(stderr, "ABORTING soon... ops = %d\n", ops);
	    fprintf(stderr, "Segment start: %d\n", s->start_pos);
	    fprintf(stderr, "Beat pos: %d\n", beat_pos);
	    fprintf(stderr, "Pos: %d\n", pos);
	    if (ops > 1000000) {
		fprintf(stderr, "ABORT\n");
		click_track_fprint(stderr, tt);
		goto set_dst_values;
		/* return 0; */
		/* exit(1); */
	    }
	}
	/* #endif */
	
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
    /* #ifdef TESTBUILD */
set_dst_values:
    /* #endif */
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
	snprintf(tt->pos_str, CLICK_POS_STR_LEN, "%d.%d.%d:%s", measure, beat, subdiv, sample_str);
	textbox_reset_full(tt->readout);
    /* fprintf(stderr, "%s\n", tt->pos_str); */
    }
    return remainder;
}

/* int send_message_udp(char *message, int port); */

void click_track_mix_metronome(ClickTrack *tt, float *mixdown_buf, int32_t mixdown_buf_len, int32_t tl_start_pos_sframes, int32_t tl_end_pos_sframes, float step)
{
    if (tt->muted) return;
    if (step < 0.0) return;
    /* if (step > 10.0) return; */
    /* fprintf(stderr, "TEMPO TRACK MIX METRONOME\n"); */
    /* clock_t start = clock(); */
    int bar, beat, subdiv;
    ClickSegment *s;
    float *buf = NULL;
    int32_t buf_len;
    int32_t tl_chunk_len = tl_end_pos_sframes - tl_start_pos_sframes;
    BeatProminence bp;
    /* clock_t c; */
    /* c = clock(); */
    /* struct timespec ts_start, ts_end; */
    /* static int runs = 0; */
    /* static double dur_old = 0.0, dur_new = 0.0; */
    /* clock_gettime(CLOCK_MONOTONIC, &ts_start); */
    int32_t remainder = click_track_bar_beat_subdiv(tt, tl_end_pos_sframes, &bar, &beat, &subdiv, &s, false);
    /* clock_gettime(CLOCK_MONOTONIC, &ts_end); */
    /* dur_old += timespec_diff_msec(&ts_start, &ts_end); */

    /* clock_gettime(CLOCK_MONOTONIC, &ts_start); */
    /* ClickTrackPos checkpos = click_track_get_pos(tt, tl_end_pos_sframes); */
    /* clock_gettime(CLOCK_MONOTONIC, &ts_end); */
    /* dur_new += timespec_diff_msec(&ts_start, &ts_end); */
    /* runs++; */
    /* if (runs == 100) { */
    /* 	fprintf(stderr, "\n%d %d %d + %d\n%d %d %d + %d\n", bar, beat, subdiv, remainder, checkpos.measure, checkpos.beat, checkpos.subdiv, checkpos.remainder); */
    /* 	fprintf(stderr, "DURS: %f, %f, %s advantage %f\n", dur_old, dur_new, dur_old < dur_new ? "OLD" : "NEW", dur_old < dur_new ? dur_new - dur_old : dur_old - dur_new); */
    /* 	dur_new = dur_old = 0.0; */
    /* 	runs = 0; */
    /* } */
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
	get_beat_prominence(s, &bp, bar, beat, subdiv);
	
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

void click_track_mute_unmute(ClickTrack *t)
{
    t->muted = !t->muted;
    if (t->muted) {
	textbox_set_background_color(t->metronome_button, &colors.mute_red);
    } else {
	textbox_set_background_color(t->metronome_button, &colors.play_green);
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

void click_track_increment_vol(ClickTrack *tt)
{
    tt->metronome_vol += TRACK_VOL_STEP;
    if (tt->metronome_vol > tt->metronome_vol_slider->max.float_v) {
	tt->metronome_vol = tt->metronome_vol_slider->max.float_v;
    }
    /* slider_edit_made(tt->metronome_vol_slider); */
    slider_reset(tt->metronome_vol_slider);
}


void click_track_decrement_vol(ClickTrack *tt)
{
    tt->metronome_vol -= TRACK_VOL_STEP;
    if (tt->metronome_vol < 0.0f) {
	tt->metronome_vol = 0.0f;
    }
    /* slider_edit_made(tt->metronome_vol_slider); */
    slider_reset(tt->metronome_vol_slider);
}

static void click_track_delete_segment_at(ClickTrack *ct, int32_t at, bool from_undo);

NEW_EVENT_FN(undo_delete_click_segment, "undo delete click track segment")
    ClickSegment *to_cpy = (ClickSegment *)obj1;
    simple_click_segment_reinsert(to_cpy, val2.int32_v);
    /* int32_t at = val1.int32_v; */
    /* uint8_t subdiv_lens[to_cpy->cfg.num_beats]; */
    /* memcpy(subdiv_lens, to_cpy->cfg.beat_subdiv_lens, to_cpy->cfg.num_beats * sizeof(uint8_t)); */
    /* click_track_add_segment(to_cpy->track, at, -1, to_cpy->cfg.bpm, to_cpy->cfg.num_beats, subdiv_lens); */
}

NEW_EVENT_FN(redo_delete_click_segment, "redo delete click track segment")
    ClickSegment *deleted = (ClickSegment *)obj1;
    click_track_delete_segment_at(deleted->track, val1.int32_v, true);
}

NEW_EVENT_FN(dispose_delete_click_segment, "")
    click_segment_destroy(obj1);
}

static void click_track_delete_segment_at(ClickTrack *ct, int32_t at, bool from_undo)
{
    ClickSegment *s = click_track_get_segment_at_pos(ct, at);
    int32_t start_pos = s->start_pos;
    int32_t segment_dur = s->next ? s->next->start_pos - s->start_pos : -1;
    simple_click_segment_remove(s);
    

    if (!from_undo) {
	user_event_push(
	    undo_delete_click_segment,
	    redo_delete_click_segment,
	    dispose_delete_click_segment, NULL,
	    (void *)s,
	    NULL,
	    (Value){.int32_v = start_pos}, (Value){.int32_v = segment_dur},
	    (Value){.int32_v = start_pos}, (Value){.int32_v = segment_dur},
	    0, 0,
	    false, false);
    }

}

void click_track_delete_segment_at_cursor(ClickTrack *ct)
{
    click_track_delete_segment_at(ct, ct->tl->play_pos_sframes, false);
}

static void timeline_select_click_track(Timeline *tl, ClickTrack *ct)
{
    tl->layout_selector = ct->layout->index;
    timeline_rectify_track_indices(tl);
}

/* Mouse */

static ClickTrackPos dragging_pos;
static float original_bpm;
static int32_t clicked_tl_pos;
void click_track_drag_pos(ClickSegment *s, Window *win)
{
    Session *session = session_get();
    int32_t tl_pos = timeview_get_pos_sframes(&ACTIVE_TL->timeview, win->mousep.x);
    int32_t og_dur = clicked_tl_pos - s->start_pos;
    int32_t new_dur = tl_pos - s->start_pos;
    double prop = (double)og_dur / new_dur;
    float new_tempo = original_bpm * prop;
    endpoint_write(&s->bpm_ep, (Value){.float_v=new_tempo}, true, true, true, false);
}

bool click_track_triage_click(uint8_t button, ClickTrack *t)
{
    Session *session = session_get();
    if (!SDL_PointInRect(&main_win->mousep, &t->layout->rect)) {
	return false;
    }
    if (SDL_PointInRect(&main_win->mousep, t->right_console_rect)) {
	if (slider_mouse_click(t->metronome_vol_slider, main_win)) {
	    return true;
	}
	if (SDL_PointInRect(&main_win->mousep, &t->metronome_button->layout->rect)) {
	    click_track_mute_unmute(t);
	    return true;
	}
	return true;
    }
    if (SDL_PointInRect(&main_win->mousep, t->console_rect)) {
	if (SDL_PointInRect(&main_win->mousep, &t->edit_button->layout->rect)) {
	    Timeline *tl = ACTIVE_TL;
	    timeline_select_click_track(tl, t);
	    timeline_click_track_edit(tl);
	    return true;
	}
	return true;
    }
    if (main_win->i_state & I_STATE_CMDCTRL) {
	Session *session = session_get();
	int32_t tl_pos = timeview_get_pos_sframes(&ACTIVE_TL->timeview, main_win->mousep.x);
	clicked_tl_pos = tl_pos;
	dragging_pos = click_track_get_pos(t, tl_pos);
	original_bpm = dragging_pos.seg->cfg.bpm;
	session->dragged_component.component = dragging_pos.seg;
	session->dragged_component.type = DRAG_CLICK_TRACK_POS;
	Value cur_val = endpoint_safe_read(&dragging_pos.seg->bpm_ep, NULL);
	endpoint_start_continuous_change(&dragging_pos.seg->bpm_ep, false, (Value){0}, JDAW_THREAD_MAIN, cur_val);
	label_move(dragging_pos.seg->bpm_label, main_win->mousep.x, dragging_pos.seg->track->layout->rect.y - 20);
	label_reset(dragging_pos.seg->bpm_label, cur_val);
	/* textbox_reset(dragging_pos.seg->bpm_label->tb); */
	return true;
    } else {

	ClickSegment *final = NULL;
	ClickSegment *s = t->segments;
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
	    session->dragged_component.component = final;
	    session->dragged_component.type = DRAG_CLICK_SEG_BOUND;
	    Value current_val = endpoint_safe_read(&final->start_pos_ep, NULL);
	    endpoint_start_continuous_change(&final->start_pos_ep, false, (Value)0, JDAW_THREAD_MAIN, current_val);
	    return true;
	}
    }

    
    return false;
}

static int32_t click_segment_get_nearest_beat_pos(ClickTrack *ct, int32_t start_pos)
{
    int bar, beat, subdiv;
    ClickSegment *s;
    int32_t remainder = click_track_bar_beat_subdiv(ct, start_pos, &bar, &beat, &subdiv, &s, false);
    int32_t init = get_beat_pos(s, bar, beat, subdiv);
    do_increment(s, &bar, &beat, &subdiv);
    int32_t next = get_beat_pos(s, bar, beat, subdiv);
    if (next - start_pos < remainder) {
	return next;
    } else {
	return init;
    }
    
}

void click_track_mouse_motion(ClickSegment *s, Window *win)
{
    int32_t tl_pos = timeline_get_abspos_sframes(s->track->tl, win->mousep.x);
    if (!(main_win->i_state & I_STATE_SHIFT)) {
	tl_pos = click_segment_get_nearest_beat_pos(s->track, tl_pos);
    }
    if (tl_pos < 0 || (s->prev && tl_pos < s->prev->start_pos + session_get_sample_rate() / 100)) {
	return;
    }
    endpoint_write(&s->start_pos_ep, (Value){.int32_v = tl_pos}, true, true, true, false);
}

void click_segment_fprint(FILE *f, ClickSegment *s)
{
    fprintf(f, "\nSegment at %p start: %d (%fs)\n", s, s->start_pos, (float)s->start_pos / s->track->tl->proj->sample_rate);
    fprintf(f, "Segment tempo (bpm): %f\n", s->cfg.bpm);
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

void click_segment_at_cursor_fprint(FILE *f, Timeline *tl)
{
    ClickTrack *tt = timeline_selected_click_track(tl);
    if (tt) {
	ClickSegment *s = click_track_get_segment_at_pos(tt, tl->play_pos_sframes);
	click_segment_fprint(f, s);
    }
}

void click_track_fprint(FILE *f, ClickTrack *tt)
{
    fprintf(f, "\nTEMPO TRACK\n");
    ClickSegment *s = tt->segments;
    while (s) {
	fprintf(f, "SEGMENT:\n");
	click_segment_fprint(f, s);
	s = s->next;
    }
}

ClickTrack *click_track_active_at_cursor(Timeline *tl)
{
    ClickTrack *ct = NULL;
    for (int i=tl->num_click_tracks - 1; i>=0; i--) {
	if (tl->click_tracks[i]->layout->index <= tl->layout_selector) {
	    ct = tl->click_tracks[i];
	    break;
	}
    }
    if (!ct && tl->click_track_frozen) {
	ct = tl->click_tracks[0];
    }
    return ct;
}

ClickSegment *click_segment_active_at_cursor(Timeline *tl)
{
    ClickTrack *ct = click_track_active_at_cursor(tl);
    if (!ct) return NULL;
    for (int i=tl->num_click_tracks - 1; i>=0; i--) {
	if (tl->click_tracks[i]->layout->index <= tl->layout_selector) {
	    ct = tl->click_tracks[i];
	    break;
	}
    }
    ClickSegment *ret;
    click_track_bar_beat_subdiv(ct, tl->play_pos_sframes, NULL, NULL, NULL, &ret, false);
    return ret;
}


