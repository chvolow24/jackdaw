/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

#include "audio_clip.h"
#include "clipref.h"
#include "color.h"
#include "consts.h"
#include "endpoint.h"
#include "label.h"
#include "midi_clip.h"
/* #include "project.h" */
#include "session.h"
#include "timeline.h"

#define CLIPREF_NAMELABEL_H 20
#define CLIPREF_NAMELABEL_H_PAD 8
#define CLIPREF_NAMELABEL_V_PAD 2

#define CLIPREF_GAIN_ADJ_SCALAR 0.02

#define CR_RECT_V_PAD (4 * main_win->dpi_scale_factor)

extern Window *main_win;
extern struct colors colors;

void clipref_gain_gui_cb(Endpoint *ep)
{
    ClipRef *cr = ep->xarg1;
    clipref_reset(cr, false);
    cr->track->tl->needs_redraw = true;
    label_move(cr->gain_label, main_win->mousep.x, cr->layout->rect.y + cr->layout->rect.h / 2);
    label_reset(cr->gain_label, ep->current_write_val);
}

void clipref_gain_dsp_cb(Endpoint *ep)
{
    ClipRef *cr = ep->xarg1;
    int sign = cr->gain_ctrl / (fabs(cr->gain_ctrl));
    cr->gain = sign * pow(fabs(cr->gain_ctrl), VOL_EXP);
}

ClipRef *clipref_create(
    Track *track,
    int32_t tl_pos,
    enum clip_type type,
    void *source_clip /* an audio or MIDI clip */
    )
{
    ClipRef *cr = calloc(1, sizeof(ClipRef));
    cr->track = track;
    cr->tl_pos = tl_pos;
    cr->type = type;
    cr->source_clip = source_clip;
		  
    /* ClipRef *cr = NULL; */
    /* MIDIClipRef *mcr = NULL; */
    switch (type) {
    case CLIP_AUDIO: {
	Clip *aclip = source_clip;
	/* cr = calloc(1, sizeof(ClipRef)); */
	pthread_mutex_init(&cr->lock, NULL);
	pthread_mutex_lock(&cr->lock);

	if (aclip->num_refs == aclip->refs_alloc_len) {
	    aclip->refs_alloc_len *= 2;
	    aclip->refs = realloc(aclip->refs, aclip->refs_alloc_len * sizeof(Clip *));
	}
	aclip->refs[aclip->num_refs] = cr;
	aclip->num_refs++;
	if (aclip->num_refs == 1) cr->home =true;
	if (aclip->num_refs > 1) {
	    snprintf(cr->name, MAX_NAMELENGTH, "%s ref %d", aclip->name, aclip->num_refs + 1);
	} else {
	    snprintf(cr->name, MAX_NAMELENGTH, "%s", aclip->name);
	}
	cr->end_in_clip = aclip->len_sframes;
	
    }
	break;
    case CLIP_MIDI: {
	MIDIClip *mclip = source_clip;
	/* mcr = calloc(1, sizeof(MIDIClipRef)); */
	/* cr->obj = mcr; */
	if (mclip->num_refs == mclip->refs_alloc_len) {
	    mclip->refs_alloc_len *= 2;
	    mclip->refs = realloc(mclip->refs, mclip->refs_alloc_len * sizeof(Clip *));
	}
	mclip->refs[mclip->num_refs] = cr;
	mclip->num_refs++;

	if (mclip->num_refs > 1) {
	    snprintf(cr->name, MAX_NAMELENGTH, "%s ref %d", mclip->name, mclip->num_refs);
	} else {
	    snprintf(cr->name, MAX_NAMELENGTH, "%s", mclip->name);
	}
	cr->end_in_clip = mclip->len_sframes;
    }
	break;
    }

    cr->layout = layout_add_child(track->inner_layout);
    cr->layout->h.type = SCALE;
    cr->layout->h.value = 0.96;
    /* pthread_mutex_init(&cr->lock, NULL); */
    /* cr->lock = SDL_CreateMutex(); */
    /* SDL_LockMutex(cr->lock); */
    /* pthread_mutex_lock(&cr->lock); */
    /* track->clips[track->num_clips] = cr; */
    /* track->num_clips++; */
    /* cr->clip = clip; */
    /* cr->track = track; */
    /* cr->home = home; */

    cr->gain = 1.0f;
    cr->gain_ctrl = 1.0f;
    cr->gain_label = label_create(0, cr->layout, label_amp_pre_exp_to_dbstr, &cr->gain, JDAW_FLOAT, main_win);
    endpoint_init(
	&cr->gain_ep,
	&cr->gain_ctrl,
	JDAW_FLOAT,
	"gain",
	"Gain",
	JDAW_THREAD_DSP,
	clipref_gain_gui_cb, NULL, clipref_gain_dsp_cb,
	cr, NULL, NULL, NULL);
    endpoint_set_default_value(&cr->gain_ep, (Value){.float_v = 1.0f});
    endpoint_set_label_fn(&cr->gain_ep, label_amp_pre_exp_to_dbstr);


    Layout *label_lt = layout_add_child(cr->layout);
    label_lt->x.value = CLIPREF_NAMELABEL_H_PAD;
    label_lt->y.value = CLIPREF_NAMELABEL_V_PAD;
    label_lt->w.type = SCALE;
    label_lt->w.value = 0.8f;
    label_lt->h.value = CLIPREF_NAMELABEL_H;
    cr->label = textbox_create_from_str(
	cr->name,
	label_lt,
	main_win->mono_bold_font,
	12,
	main_win);
    cr->label->text->validation = txt_name_validation;
    cr->label->text->completion = project_obj_name_completion;

    textbox_set_align(cr->label, CENTER_LEFT);
    textbox_set_background_color(cr->label, NULL);
    if (type == CLIP_AUDIO) {
	textbox_set_text_color(cr->label, &colors.white);
	/* textbox_set_text_color(cr->label, &colors.light_grey); */
    } else if (type == CLIP_MIDI) {
	textbox_set_text_color(cr->label, &colors.dark_brown);
    }
    /* textbox_size_to_fit(cr->label, CLIPREF_NAMELABEL_H_PAD, CLIPREF_NAMELABEL_V_PAD); */
    /* fprintf(stdout, "Clip num refs: %d\n", clip->num_refs); */
    /* clip->refs[clip->num_refs] = cr; */
    /* clip->num_refs++; */
    /* pthread_mutex_unlock(&cr->lock); */
    if (track->num_clips == track->clips_alloc_len) {
	track->clips_alloc_len *= 2;
	track->clips = realloc(track->clips, track->clips_alloc_len * sizeof(ClipRef *));
    }
    track->clips[track->num_clips] = cr;
    track->num_clips++;
    if (cr->type == CLIP_AUDIO) {
	pthread_mutex_unlock(&cr->lock);
    }


    return cr;
}

int32_t clipref_len(ClipRef *cr)
{
    if (cr->end_in_clip == 0) {
	switch(cr->type) {
	case CLIP_AUDIO:
	    cr->end_in_clip = ((Clip *)cr->source_clip)->len_sframes;
	    /* return ((Clip *)(cr->source_clip))->len_sframes; */
	    break;
	case CLIP_MIDI:
	    cr->end_in_clip = ((MIDIClip *)cr->source_clip)->len_sframes;
	    /* return ((MIDIClip *)cr->source_clip)->len_sframes; */
	    break;
	}
    }
    return cr->end_in_clip - cr->start_in_clip;
}

void clipref_reset(ClipRef *cr, bool rescaled)
{
    cr->layout->rect.x = timeline_get_draw_x(cr->track->tl, cr->tl_pos);
    int32_t cr_len = clipref_len(cr);
    /* uint32_t cr_len = cr->in_mark_sframes >= cr->out_mark_sframes */
    /* 	? cr->clip->len_sframes */
    /* 	: cr->out_mark_sframes - cr->in_mark_sframes; */
    cr->layout->rect.w = timeline_get_draw_w(cr->track->tl, cr_len);
    cr->layout->rect.y = cr->track->inner_layout->rect.y + CR_RECT_V_PAD;
    cr->layout->rect.h = cr->track->inner_layout->rect.h - 2 * CR_RECT_V_PAD;
    layout_set_values_from_rect(cr->layout);
    layout_reset(cr->layout);
    textbox_reset_full(cr->label);
    /* if (rescaled) { */
    if (cr->type == CLIP_AUDIO) {
	cr->waveform_redraw = true;
    }
}

/* Moving cliprefs on the timeline */

static void clipref_remove_from_track(ClipRef *cr)
{
    bool displace = false;
    Track *track = cr->track;
    for (uint16_t i=0; i<track->num_clips; i++) {
	ClipRef *test = track->clips[i];
	if (test == cr) {
	    displace = true;
	} else if (displace && i > 0) {
	    track->clips[i - 1] = test;	    
	}
    }
    if (displace) track->num_clips--; /* else not found! */
}

/* static void clipref_displace(ClipRef *cr, int displace_by) */
/* { */
/*     Track *track = cr->track; */
/*     Timeline *tl = track->tl; */
/*     int new_rank = track->tl_rank + displace_by; */
/*     if (new_rank >= 0 && new_rank < tl->num_tracks) { */
/* 	Track *new_track = tl->tracks[new_rank]; */
/* 	clipref_remove_from_track(cr); */
/* 	/\* clipref_remove_from_track(cr); *\/ */
/* 	if (new_track->num_clips == new_track->clips_alloc_len) { */
/* 	    new_track->clips_alloc_len *= 2; */
/* 	    new_track->clips = realloc(new_track->clips, new_track->clips_alloc_len * sizeof(ClipRef *)); */
/* 	} */
/* 	new_track->clips[new_track->num_clips] = cr; */
/* 	new_track->num_clips++; */
/* 	cr->track = new_track; */
/* 	/\* fprintf(stdout, "ADD CLIP TO TRACK %s, which has %d clips now\n", new_track->name, new_track->num_clips); *\/ */
	
/*     } */
/*     timeline_reset(tl, false); */
/* } */

static void clipref_insert_on_track(ClipRef *cr, Track *target)
{
    if (target->num_clips == target->clips_alloc_len) {
	target->clips_alloc_len *= 2;
	target->clips = realloc(target->clips, target->clips_alloc_len * sizeof(ClipRef *));	
    }
    target->clips[target->num_clips] = cr;
    target->num_clips++;
    cr->track = target;    
}


void clipref_move_to_track(ClipRef *cr, Track *target)
{
    Track *prev_track = cr->track;
    Timeline *tl = prev_track->tl;
    clipref_remove_from_track(cr);
    clipref_insert_on_track(cr, target);
    timeline_reset(tl, false);
}

/* void clipref_grab(ClipRef *cr) */
/* { */
/*     Timeline *tl = cr->track->tl; */
/*     if (tl->num_grabbed_clips == MAX_GRABBED_CLIPS) { */
/* 	char errstr[MAX_STATUS_STRLEN]; */
/* 	snprintf(errstr, MAX_STATUS_STRLEN, "Cannot grab >%d clips", MAX_GRABBED_CLIPS); */
/* 	status_set_errstr(errstr); */
/* 	return; */
/*     } */

/*     tl->grabbed_clips[tl->num_grabbed_clips] = cr; */
/*     tl->num_grabbed_clips++; */
/*     cr->grabbed = true; */
/* } */


/* void clipref_ungrab(ClipRef *cr) */
/* { */
/*     Timeline *tl = cr->track->tl; */
/*     bool displace = false; */
/*     for (uint8_t i=0; i<tl->num_grabbed_clips; i++) { */
/* 	if (displace) { */
/* 	    tl->grabbed_clips[i - 1] = tl->grabbed_clips[i]; */
/* 	} else if (cr == tl->grabbed_clips[i]) { */
/* 	    displace = true; */
/* 	} */
/*     } */
/*     cr->grabbed = false; */
/*     cr->grabbed_edge = CLIPREF_EDGE_NONE; */
/*     tl->num_grabbed_clips--; */
/*     status_stat_drag(); */
/* } */

/* void clipref_grab_left(ClipRef *cr) */
/* { */
/*     if (!cr->grabbed) clipref_grab(cr); */
/*     cr->grabbed_edge = CLIPREF_EDGE_LEFT; */
/* } */
/* void clipref_grab_right(ClipRef *cr) */
/* { */
/*     if (!cr->grabbed) clipref_grab(cr); */
/*     cr->grabbed_edge = CLIPREF_EDGE_RIGHT; */
/* } */

void clipref_bring_to_front()
{
    Session *session = session_get();
    Timeline *tl = ACTIVE_TL;
    Track *track = timeline_selected_track(tl);
    if (!track) return;
    /* bool displace = false; */
    ClipRef *to_move = NULL;
    for (int i=0; i<track->num_clips; i++) {
	ClipRef *cr = track->clips[i];
	if (!to_move && cr->tl_pos <= tl->play_pos_sframes && cr->tl_pos + clipref_len(cr) >= tl->play_pos_sframes) {
	    to_move = cr;
	} else if (to_move) {
	    track->clips[i-1] = track->clips[i];
	    if (i == track->num_clips - 1) {
		track->clips[i] = to_move;
	    }
	}
    }
    tl->needs_redraw = true;
}
ClipRef *clipref_at_cursor_not_dragging()
{
    Session *session = session_get();
    Timeline *tl = ACTIVE_TL;
    Track *track = timeline_selected_track(tl);
    if (!track) return NULL;
    
    /* Reverse iter to ensure top-most clip is returned in case of overlap */
    for (int i=track->num_clips -1; i>=0; i--) {
	ClipRef *cr = track->clips[i];
	if (session->dragging && cr->grabbed) continue;
	if (cr->tl_pos <= tl->play_pos_sframes && cr->tl_pos + clipref_len(cr) >= tl->play_pos_sframes) {
	    return cr;
	}
    }
    return NULL;

}

ClipRef *clipref_at_cursor()
{
    Session *session = session_get();
    Timeline *tl = ACTIVE_TL;
    Track *track = timeline_selected_track(tl);
    if (!track) return NULL;
    
    /* Reverse iter to ensure top-most clip is returned in case of overlap */
    for (int i=track->num_clips -1; i>=0; i--) {
	ClipRef *cr = track->clips[i];
	if (cr->tl_pos <= tl->play_pos_sframes && cr->tl_pos + clipref_len(cr) >= tl->play_pos_sframes) {
	    return cr;
	}
    }
    return NULL;
}

ClipRef *clipref_at_cursor_in_track(Track *track)
{
    for (int i=track->num_clips-1; i>=0; i--) {
	ClipRef *cr = track->clips[i];
	if (cr->tl_pos <= track->tl->play_pos_sframes && cr->tl_pos + clipref_len(cr) >= track->tl->play_pos_sframes) {
	    return cr;
	}
    }
    return NULL;
}

ClipRef *clipref_before_cursor(int32_t *pos_dst)
{
    Session *session = session_get();
    Timeline *tl = ACTIVE_TL;
    Track *track = timeline_selected_track(tl);
    if (!track) return NULL;
    if (track->num_clips == 0) return NULL;
    ClipRef *ret = NULL;
    int32_t end = INT32_MIN;
    for (int i=0; i<track->num_clips; i++) {
	ClipRef *cr = track->clips[i];
	if (cr->grabbed && session->dragging) continue;
	int32_t cr_end = cr->tl_pos + clipref_len(cr);
	if (cr_end <= tl->play_pos_sframes && cr_end >= end) {
	    ret = cr;
	    if (cr_end == tl->play_pos_sframes) {
		end = cr->tl_pos;
	    } else {
		end = cr_end;
	    }
	}
    }
    if (pos_dst)
	*pos_dst = end;
    return ret;
}

ClipRef *clipref_after_cursor(int32_t *pos_dst)
{
    Session *session = session_get();
    Timeline *tl = ACTIVE_TL;
    Track *track = timeline_selected_track(tl);
    if (!track) return NULL;
    if (track->num_clips == 0) return NULL;
    ClipRef *ret = NULL;
    int32_t start = INT32_MAX;
    for (int i=0; i<track->num_clips; i++) {
	ClipRef *cr = track->clips[i];
	if (cr->grabbed && session->dragging) continue;
	int32_t cr_start = cr->tl_pos;
	if (cr_start > tl->play_pos_sframes && cr_start <= start) {
	    start = cr_start;
	    if (pos_dst)
		*pos_dst = cr_start;
	    ret = cr;
	    break;
	}
    }
    return ret;
}

void clipref_delete(ClipRef *cr)
{
    if (cr->grabbed) {
	timeline_clipref_ungrab(cr);
    }
    cr->track->tl->needs_redraw = true;
    cr->deleted = true;
    clipref_remove_from_track(cr);
}

void clipref_undelete(ClipRef *cr)
{
    cr->track->tl->needs_redraw = true;
    cr->deleted = false;
    clipref_insert_on_track(cr, cr->track);
}


static NEW_EVENT_FN(undo_cut_clipref, "undo cut clip")
    ClipRef *cr1 = (ClipRef *)obj1;
    ClipRef *cr2 = (ClipRef *)obj2;
    cr1->end_in_clip = val2.int32_v;
    clipref_reset(cr1, true);
    clipref_delete(cr2);
}

static NEW_EVENT_FN(redo_cut_clipref, "redo cut clip")
    ClipRef *cr1 = (ClipRef *)obj1;
    ClipRef *cr2 = (ClipRef *)obj2;
    clipref_undelete(cr2);
    cr1->end_in_clip = val1.int32_v;
    clipref_reset(cr1, true);
    /* clipref_reset(cr2); */
}


ClipRef *piano_roll_get_clipref();
void piano_roll_deactivate();

static void clipref_check_and_remove_from_clipboard_and_piano_roll(ClipRef *cr)
{
    /* fprintf(stderr, "handling destruction CHECK AND REMOVE\n"); */
    Timeline *tl = cr->track->tl;
    bool displace = false;
    for (int i=0; i<tl->num_clips_in_clipboard; i++) {
	if (tl->clipboard[i] == cr) {
	    displace = true;
	    /* fprintf(stderr, "Found %p==%p at index %d\n", cr, tl->clipboard[i], i); */
	} else if (displace) {
	    /* fprintf(stderr, "\tMoving index %d->%d\n", i, i-1); */
	    tl->clipboard[i-1] = tl->clipboard[i];
	}
    }
    if (displace) {
	tl->num_clips_in_clipboard--;
	/* fprintf(stderr, "removed!\n"); */
    }
    Session *session = session_get();
    if (session->piano_roll && cr == piano_roll_get_clipref()) {
	piano_roll_deactivate();
    }
}

/* void midi_clipref_destroy(MIDIClipRef *mcr); */
void midi_clip_destroy(MIDIClip *mc);
/* void clipref_destroy(ClipRef *cr); */
void clipref_destroy(ClipRef *cr, bool displace_in_clip)
{
    Track *track = cr->track;
    Clip *clip = NULL;
    MIDIClip *mclip = NULL;
    switch (cr->type) {
    case CLIP_AUDIO:
	clip = cr->source_clip;
	break;
    case CLIP_MIDI:
	mclip = cr->source_clip;
	break;
    }

    clipref_check_and_remove_from_clipboard_and_piano_roll(cr);
    label_destroy(cr->gain_label);
    
    bool displace = false;
    for (uint16_t i=0; i<track->num_clips; i++) {
	if (track->clips[i] == cr) {
	    displace = true;
	} else if (displace && i>0) {
	    track->clips[i-1] = track->clips[i];
	}
    }
    if (displace) {
	track->num_clips--; /* else not found! */
    }
    /* fprintf(stdout, "\t->Track %d now has %d clips\n", track->tl_rank, track->num_clips); */

    if (displace_in_clip) {
	displace = false;
	if (clip) {
	    for (uint16_t i=0; i<clip->num_refs; i++) {
		if (cr == clip->refs[i]) displace = true;
		else if (displace) {
		    clip->refs[i-1] = clip->refs[i];
		}
	    }
	    clip->num_refs--;
	    if (clip->num_refs == 0) {
		clip_destroy(clip);
	    }
	    /* clipref_destroy(cr, true); */
	} else if (mclip) {
	    for (uint16_t i=0; i<mclip->num_refs; i++) {
		if (cr == mclip->refs[i]) displace = true;
		else if (displace) {
		    mclip->refs[i-1] = mclip->refs[i];
		}
	    }
	    mclip->num_refs--;
	    if (mclip->num_refs == 0) {
		midi_clip_destroy(mclip);
	    }
	    /* midi_clipref_destroy(cr->obj); */
	}
	/* fprintf(stdout, "\t->Clip at %p now has %d refs\n", clip, clip->num_refs); */
    }


    textbox_destroy(cr->label);
    /* pthread_mutex_destroy(&cr->lock); */
    /* if (cr->waveform_texture) */
	/* SDL_DestroyTexture(cr->waveform_texture); */
    free(cr);
}
void clipref_destroy_no_displace(ClipRef *cr)
{
    clipref_check_and_remove_from_clipboard_and_piano_roll(cr);
    Clip *clip = NULL;
    MIDIClip *mclip = NULL;
    switch (cr->type) {
    case CLIP_AUDIO:
	clip = cr->source_clip;
	break;
    case CLIP_MIDI:
	mclip = cr->source_clip;
	break;
    }
    /* fprintf(stdout, "TrackClip destroy no displace %s\n", cr->name); */
    bool displace = false;
    if (clip) {
	for (uint16_t i=0; i<clip->num_refs; i++) {
	    ClipRef *test = clip->refs[i];
	    if (test == cr) displace = true;
	    else if (displace) {
		clip->refs[i-1] = clip->refs[i];
	    }
	}
	clip->num_refs--;
	
	/* TODO: reconsider this; prompt user? */
	if (clip->num_refs == 0) {
	    clip_destroy(clip);
	}

    } else if (mclip) {
	for (uint16_t i=0; i<mclip->num_refs; i++) {
	    ClipRef *test = mclip->refs[i];
	    if (test == cr) displace = true;
	    else if (displace) {
		mclip->refs[i-1] = mclip->refs[i];
	    }
	}
	mclip->num_refs--;
	if (mclip->num_refs == 0) {
	    midi_clip_destroy(mclip);
	}

	
    }
    /* TODO: keep clips around */
    pthread_mutex_destroy(&cr->lock);
    /* SDL_DestroyMutex(cr->lock); */
    textbox_destroy(cr->label);
    if (cr->waveform_texture)
	SDL_DestroyTexture(cr->waveform_texture);
    free(cr);
}


static NEW_EVENT_FN(dispose_forward_cut_clipref, "")
    clipref_destroy_no_displace((ClipRef *)obj2);
}


void track_reset(Track *, bool);
static ClipRef *clipref_cut(ClipRef *cr, int32_t cut_pos_rel)
{
    /* TrackClip *new = clipref_add(cr->track, cr->clip, cr->tl_pos + cut_pos_rel, false); */
    ClipRef *new = clipref_create(cr->track, cr->tl_pos + cut_pos_rel, cr->type, cr->source_clip);
    if (!new) {
	return NULL;
    }
    new->gain = cr->gain;
    if (cut_pos_rel < 0) return NULL;
    new->start_in_clip = cr->start_in_clip + cut_pos_rel;
    new->end_in_clip = cr->end_in_clip == 0 ? clipref_len(cr) : cr->end_in_clip;
    Value orig_end_pos = {.int32_v = cr->end_in_clip};
    cr->end_in_clip = cr->end_in_clip == 0 ? cut_pos_rel : cr->end_in_clip - (clipref_len(cr) - cut_pos_rel);
    track_reset(cr->track, true);

    Value cut_pos = {.int32_v = cr->end_in_clip};
    user_event_push(
	undo_cut_clipref,
	redo_cut_clipref,
	NULL,
	dispose_forward_cut_clipref,
	cr, new,
	cut_pos, orig_end_pos, cut_pos, orig_end_pos,
	0, 0, false, false);
    return new;
}

void timeline_cut_at_cursor(Timeline *tl)
{
    Track *track = timeline_selected_track(tl);
    if (track) {
	status_cat_callstr(" clipref at cursor");
	/* TrackClip *cr = clipref_at_cursor(); */
	ClipRef *cr = clipref_at_cursor();
	if (!cr) {
	    status_set_errstr("Error: no clip at cursor");
	    return;
	}
	if (tl->play_pos_sframes > cr->tl_pos && tl->play_pos_sframes < cr->tl_pos + clipref_len(cr)) {
	    clipref_cut(cr, tl->play_pos_sframes - cr->tl_pos);
	}
    } else {
	timeline_cut_click_track_at_cursor(tl);
	status_cat_callstr(" tempo track at cursor");
    }
}


void timeline_cut_at_cursor_and_grab_edges(Timeline *tl)
{
    Track *track = timeline_selected_track(tl);
    if (!track) return;
    if (track) {
	status_cat_callstr(" clipref at cursor");
	/* TrackClip *cr = clipref_at_cursor(); */
	ClipRef *cr = clipref_at_cursor();
	if (!cr) {
	    status_set_errstr("Error: no clip at cursor");
	    return;
	}
	if (tl->play_pos_sframes > cr->tl_pos && tl->play_pos_sframes < cr->tl_pos + clipref_len(cr)) {
	    ClipRef *new = clipref_cut(cr, tl->play_pos_sframes - cr->tl_pos);
	    if (new) {
		timeline_clipref_grab(new, CLIPREF_EDGE_LEFT);
		timeline_clipref_grab(cr, CLIPREF_EDGE_RIGHT);
	    }
	}
    }
}

NEW_EVENT_FN(undo_split_cr, "undo split stereo clip to mono")
    ClipRef **crs = (ClipRef **)obj1;
    ClipRef *orig = crs[0];
    ClipRef *new_L = crs[1];
    ClipRef *new_R = crs[2];
    clipref_delete(new_L);
    clipref_delete(new_R);
    clipref_undelete(orig);
}

NEW_EVENT_FN(redo_split_cr, "redo split stereo clip to mono")
    ClipRef **crs = (ClipRef **)obj1;
    ClipRef *orig = crs[0];
    ClipRef *new_L = crs[1];
    ClipRef *new_R = crs[2];
    clipref_delete(orig);
    clipref_undelete(new_L);
    clipref_undelete(new_R);
}

NEW_EVENT_FN(dispose_split_cr, "")
    ClipRef **crs = (ClipRef **)obj1;
    ClipRef *orig = crs[0];
    clipref_destroy(orig, true);
}

NEW_EVENT_FN(dispose_forward_split_cr, "")
    ClipRef **crs = (ClipRef **)obj1;
    ClipRef *new_L = crs[1];
    ClipRef *new_R = crs[2];
    clipref_destroy(new_L, true);
    clipref_destroy(new_R, true);
}

int clipref_split_stereo_to_mono(ClipRef *cr, ClipRef **new_L_dst, ClipRef **new_R_dst)
{
    if (cr->type == CLIP_MIDI) {
	fprintf(stderr, "Error: cannot split midi clip\n");
	return -1;
    }
    Clip *clip = cr->source_clip;
    if (clip->channels != 2) {
	fprintf(stderr, "Error: clip must have two channels to be split\n");
	return 0;
    }
    Track *t = cr->track;
    if (t->tl_rank == t->tl->num_tracks - 1) {
	fprintf(stderr, "Error: No room to split clip reference. Create a new track below.\n");
	return 0;
    }
    
    Clip *clip_L, *clip_R;
    clip_split_stereo_to_mono(clip, &clip_L, &clip_R);

    Track *next_track = t->tl->tracks[t->tl_rank + 1];

    ClipRef *new_L, *new_R;
    new_L = clipref_create(t, cr->tl_pos, CLIP_AUDIO, clip_L);
    new_R = clipref_create(next_track, cr->tl_pos, CLIP_AUDIO, clip_R);
    /* new_L = track_add_clipref(t, clip_L, cr->tl_pos, true); */
    /* new_R = track_add_clipref(next_track, clip_R, cr->tl_pos, true); */
    if (new_L_dst) *new_L_dst = new_L;
    if (new_R_dst) *new_R_dst = new_R;
    snprintf(new_L->name, MAX_NAMELENGTH, "%s L", cr->name);
    snprintf(new_R->name, MAX_NAMELENGTH, "%s R", cr->name);

    new_L->end_in_clip = clip_L->len_sframes;
    new_R->end_in_clip = clip_R->len_sframes;
    clipref_delete(cr);


    ClipRef **crs = malloc(3 * sizeof(ClipRef *));
    crs[0] = cr;
    crs[1] = new_L;
    crs[2] = new_R;

    Clip **clips = malloc(2* sizeof(Clip *));
    clips[0] = clip_L;
    clips[1] = clip_R;
    
    user_event_push(
	undo_split_cr,
	redo_split_cr,
	dispose_split_cr, dispose_forward_split_cr,
	crs, clips,
	(Value){0}, (Value){0},
	(Value){0}, (Value){0},
	0, 0,
	true, true);
    timeline_reset_full(t->tl);
    return 1;
}

bool clipref_marked(Timeline *tl, ClipRef *cr)
{
    if (tl->in_mark_sframes >= tl->out_mark_sframes) return false;
    int32_t cr_end = cr->tl_pos + clipref_len(cr);
    if (cr_end >= tl->in_mark_sframes && cr->tl_pos <= tl->out_mark_sframes) return true;
    return false;
}

void project_draw();
void clipref_rename(ClipRef *cr)
{
    /* TODO: replace with TextEntry */
    txt_edit(cr->label->text, project_draw);
    main_win->i_state = 0;
}


void clipref_gain_drag(ClipRef *cr, Window *win)
{
    float new_gain = endpoint_safe_read(&cr->gain_ep, NULL).float_v - (float)win->current_event->motion.yrel * CLIPREF_GAIN_ADJ_SCALAR;
    endpoint_write(&cr->gain_ep, (Value){.float_v = new_gain}, true, true, true, false);
}
