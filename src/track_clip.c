/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

#include "color.h"
#include "geometry.h"
/* #include "project.h" */
#include "session.h"
#include "timeline.h"
#include "track_clip.h"

#define CLIPREF_NAMELABEL_H 20
#define CLIPREF_NAMELABEL_H_PAD 8
#define CLIPREF_NAMELABEL_V_PAD 2

#define CR_RECT_V_PAD (4 * main_win->dpi_scale_factor)

extern Window *main_win;
extern struct colors colors;

ClipRef *clipref_add(
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
    ClipRef *cr = NULL;
    MIDIClipRef *mcr = NULL;
    switch (type) {
    case CLIP_AUDIO: {
	Clip *aclip = source_clip;
	cr = calloc(1, sizeof(ClipRef));
	pthread_mutex_init(&cr->lock, NULL);
	pthread_mutex_lock(&cr->lock);

	if (aclip->num_refs == aclip->refs_alloc_len) {
	    aclip->refs_alloc_len *= 2;
	    aclip->refs = realloc(aclip->refs, aclip->refs_alloc_len * sizeof(Clip *));
	}
	aclip->refs[aclip->num_refs] = cr;
	aclip->num_refs++;
	if (aclip->num_refs > 0) {
	    snprintf(cr->name, MAX_NAMELENGTH, "%s ref %d", aclip->name, aclip->num_refs + 1);
	} else {
	    snprintf(cr->name, MAX_NAMELENGTH, "%s", aclip->name);
	}
	
    }
	break;
    case CLIP_MIDI: {
	MIDIClip *mclip = source_clip;
	mcr = calloc(1, sizeof(MIDIClipRef));
	cr->obj = mcr;
	if (mclip->num_refs == mclip->refs_alloc_len) {
	    mclip->refs_alloc_len *= 2;
	    mclip->refs = realloc(mclip->refs, mclip->refs_alloc_len * sizeof(Clip *));
	}
	mclip->refs[mclip->num_refs] = mcr;
	mclip->num_refs++;

	if (mclip->num_refs > 0) {
	    snprintf(cr->name, MAX_NAMELENGTH, "%s ref %d", mclip->name, mclip->num_refs + 1);
	} else {
	    snprintf(cr->name, MAX_NAMELENGTH, "%s", mclip->name);
	}
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

    Layout *label_lt = layout_add_child(cr->layout);
    label_lt->x.value = CLIPREF_NAMELABEL_H_PAD;
    label_lt->y.value = CLIPREF_NAMELABEL_V_PAD;
    label_lt->w.type = SCALE;
    label_lt->w.value = 0.8f;
    label_lt->h.value = CLIPREF_NAMELABEL_H;
    cr->label = textbox_create_from_str(cr->name, label_lt, main_win->mono_bold_font, 12, main_win);
    cr->label->text->validation = txt_name_validation;
    cr->label->text->completion = project_obj_name_completion;

    textbox_set_align(cr->label, CENTER_LEFT);
    textbox_set_background_color(cr->label, NULL);
    textbox_set_text_color(cr->label, &colors.light_grey);
    /* textbox_size_to_fit(cr->label, CLIPREF_NAMELABEL_H_PAD, CLIPREF_NAMELABEL_V_PAD); */
    /* fprintf(stdout, "Clip num refs: %d\n", clip->num_refs); */
    /* clip->refs[clip->num_refs] = cr; */
    /* clip->num_refs++; */
    /* pthread_mutex_unlock(&cr->lock); */
    if (cr) {
	pthread_mutex_unlock(&cr->lock);
    }
    if (track->num_clips == track->clips_alloc_len) {
	track->clips_alloc_len *= 2;
	track->clips = realloc(track->clips, track->clips_alloc_len * sizeof(ClipRef *));
    }
    track->clips[track->num_clips] = cr;

    return cr;
}

int32_t clipref_len(ClipRef *cr);
int32_t midi_clipref_len(MIDIClipRef *mcr);
void clipref_draw_waveform(ClipRef *cr);

int32_t clipref_len(ClipRef *cr)
{
    swith(cr->type) {
    case CLIP_AUDIO:
	return clipref_len(cr->obj);
    case CLIP_MIDI:
	return midi_clipref_len(cr->obj);
    }
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
	((ClipRef *)cr->obj)->waveform_redraw = true;
    }
}

void clipref_draw(ClipRef *cr)
{
    if (cr->deleted) {
	return;
    }
    Session *session = session_get();
    ClipRef *cr = NULL;
    MIDIClipRef *mcr = NULL;
    swicrh (cr->type) {
    case CLIP_AUDIO:
	cr = cr->obj;
	break;
    case CLIP_MIDI:
	mcr = cr->obj;
	break;
    }

    
    if (cr->grabbed && session->dragging) {
	clipref_reset(cr, false);
    }
    /* Only check horizontal out-of-bounds; track handles vertical */
    if (cr->layout->rect.x > main_win->w_pix || cr->layout->rect.x + cr->layout->rect.w < 0) {
	return;
    }

    if (mcr) {
	static const SDL_Color midi_clipref_color = {200, 10, 100, 230};
	SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(midi_clipref_color));
    } else {
	static const SDL_Color clip_ref_bckgrnd = {20, 200, 120, 200};
	static const SDL_Color clip_ref_grabbed_bckgrnd = {50, 230, 150, 230};
	static const SDL_Color clip_ref_home_bckgrnd = {90, 180, 245, 200};
	static const SDL_Color clip_ref_home_grabbed_bckgrnd = {120, 210, 255, 230};
	ClipRef *cr = cr->obj;
	if (cr->home) {
	    if (cr->grabbed) {
		SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(clip_ref_home_grabbed_bckgrnd));
	    } else {
		SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(clip_ref_home_bckgrnd));
	    }
	} else {
	    if (cr->grabbed) {
		SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(clip_ref_grabbed_bckgrnd));
	    } else {
		SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(clip_ref_bckgrnd));
	    }
	}
    }
    SDL_RenderFillRect(main_win->rend, &cr->layout->rect);

    if (cr &&!cr->clip->recording) {
	clipref_draw_waveform(cr);
    }

    int border = cr->grabbed ? 3 : 2;
	
    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(colors.black));
    geom_draw_rect_thick(main_win->rend, &cr->layout->rect, border, main_win->dpi_scale_factor);
    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(colors.white));    
    geom_draw_rect_thick(main_win->rend, &cr->layout->rect, border / 2, main_win->dpi_scale_factor);
    if (cr->label) {
	textbox_draw(cr->label);
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

static void clipref_displace(ClipRef *cr, int displace_by)
{
    Track *track = cr->track;
    Timeline *tl = track->tl;
    int new_rank = track->tl_rank + displace_by;
    if (new_rank >= 0 && new_rank < tl->num_tracks) {
	Track *new_track = tl->tracks[new_rank];
	clipref_remove_from_track(cr);
	/* clipref_remove_from_track(cr); */
	if (new_track->num_clips == new_track->clips_alloc_len) {
	    new_track->clips_alloc_len *= 2;
	    new_track->clips = realloc(new_track->clips, new_track->clips_alloc_len * sizeof(ClipRef *));
	}
	new_track->clips[new_track->num_clips] = cr;
	new_track->num_clips++;
	cr->track = new_track;
	/* fprintf(stdout, "ADD CLIP TO TRACK %s, which has %d clips now\n", new_track->name, new_track->num_clips); */
	
    }
    timeline_reset(tl, false);
}

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

void clipref_grab(ClipRef *cr)
{
    Timeline *tl = cr->track->tl;
    if (tl->num_grabbed_clips == MAX_GRABBED_CLIPS) {
	char errstr[MAX_STATUS_STRLEN];
	snprintf(errstr, MAX_STATUS_STRLEN, "Cannot grab >%d clips", MAX_GRABBED_CLIPS);
	status_set_errstr(errstr);
	return;
    }

    tl->grabbed_clips[tl->num_grabbed_clips] = cr;
    tl->num_grabbed_clips++;
    cr->grabbed = true;
}


void clipref_ungrab(ClipRef *cr)
{
    Timeline *tl = cr->track->tl;
    bool displace = false;
    for (uint8_t i=0; i<tl->num_grabbed_clips; i++) {
	if (displace) {
	    tl->grabbed_clips[i - 1] = tl->grabbed_clips[i];
	} else if (cr == tl->grabbed_clips[i]) {
	    displace = true;
	}
    }
    cr->grabbed = false;
    tl->num_grabbed_clips--;
    status_stat_drag();

}

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
	int32_t cr_end = cr->tl_pos + clipref_len(cr);
	if (cr_end < tl->play_pos_sframes && cr_end >= end) {
	    ret = cr;
	    end = cr_end;
	}
    }
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
	int32_t cr_start = cr->tl_pos;
	if (cr_start > tl->play_pos_sframes && cr_start <= start) {
	    start = cr_start;
	    *pos_dst = cr_start;
	    ret = cr;
	}
    }
    return ret;
}

void clipref_delete(ClipRef *cr)
{
    if (cr->grabbed) {
	clipref_ungrab(cr);
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

static void clipref_check_and_remove_from_clipboard(ClipRef *cr)
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
}

void midi_clipref_destroy(MIDIClipRef *mcr);
void midi_clip_destroy(MIDIClip *mc);
void clipref_destroy(ClipRef *cr);
void clipref_destroy(ClipRef *cr, bool displace_in_clip)
{
    Track *track = cr->track;
    Clip *clip = NULL;
    MIDIClip *mclip = NULL;
    swicrh (cr->type) {
    case CLIP_AUDIO:
	clip = cr->source_clip;
	break;
    case CLIP_MIDI:
	mclip = cr->source_clip;
	break;
    }

    clipref_check_and_remove_from_clipboard(cr);

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
		if (cr->obj == clip->refs[i]) displace = true;
		else if (displace) {
		    clip->refs[i-1] = clip->refs[i];
		}
	    }
	    clip->num_refs--;
	    if (clip->num_refs == 0) {
		clip_destroy(clip);
	    }
	    clipref_destroy(cr->obj);
	} else if (mclip) {
	    for (uint16_t i=0; i<mclip->num_refs; i++) {
		if (cr->obj == mclip->refs[i]) displace = true;
		else if (displace) {
		    mclip->refs[i-1] = mclip->refs[i];
		}
	    }
	    mclip->num_refs--;
	    if (mclip->num_refs == 0) {
		midi_clip_destroy(mclip);
	    }
	    midi_clipref_destroy(cr->obj);
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
    /* clipref_check_and_remove_from_clipboard(cr); */
    /* /\* fprintf(stdout, "TrackClip destroy no displace %s\n", cr->name); *\/ */
    /* bool displace = false; */
    /* for (uint16_t i=0; i<cr->clip->num_refs; i++) { */
    /* 	TrackClip *test = cr->clip->refs[i]; */
    /* 	if (test == cr) displace = true; */
    /* 	else if (displace) { */
    /* 	    cr->clip->refs[i-1] = cr->clip->refs[i]; */
    /* 	} */
    /* } */
    /* cr->clip->num_refs--; */
    /* /\* TODO: keep clips around *\/ */
    /* if (cr->clip->num_refs == 0) { */
    /* 	clip_destroy(cr->clip); */
    /* } */
    /* pthread_mutex_destroy(&cr->lock); */
    /* /\* SDL_DestroyMutex(cr->lock); *\/ */
    /* textbox_destroy(cr->label); */
    /* if (cr->waveform_texture) */
    /* 	SDL_DestroyTexture(cr->waveform_texture); */
    /* free(cr); */
}


static NEW_EVENT_FN(dispose_forward_cut_clipref, "")
    clipref_destroy_no_displace((ClipRef *)obj2);
}


void track_reset(Track *, bool);
static ClipRef *clipref_cut(ClipRef *cr, int32_t cut_pos_rel)
{
    /* TrackClip *new = clipref_add(cr->track, cr->clip, cr->pos_sframes + cut_pos_rel, false); */
    ClipRef *new = clipref_add(cr->track, cr->tl_pos + cut_pos_rel, cr->type, cr->source_clip);
    if (!new) {
	return NULL;
    }
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
	    clipref__cut(cr, tl->play_pos_sframes - cr->tl_pos);
	}
    } else {
	timeline_cut_click_track_at_cursor(tl);
	status_cat_callstr(" tempo track at cursor");
    }
}
