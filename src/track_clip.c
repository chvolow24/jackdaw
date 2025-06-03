/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

#include "color.h"
#include "geometry.h"
#include "project.h"
#include "session.h"
#include "timeline.h"
#include "track_clip.h"

#define CLIPREF_NAMELABEL_H 20
#define CLIPREF_NAMELABEL_H_PAD 8
#define CLIPREF_NAMELABEL_V_PAD 2

#define CR_RECT_V_PAD (4 * main_win->dpi_scale_factor)

extern Window *main_win;
extern struct colors colors;

TrackClip *tclip_add(
    Track *track,
    int32_t tl_pos,
    enum track_clip_type type,
    void *source_clip /* an audio or MIDI clip */
    )
{
    TrackClip *tc = calloc(1, sizeof(TrackClip));
    tc->track = track;
    tc->tl_pos = tl_pos;
    tc->type = type;
    tc->source_clip = source_clip;
    ClipRef *cr = NULL;
    MIDIClipRef *mcr = NULL;
    switch (type) {
    case TCLIP_AUDIO: {
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
	    snprintf(tc->name, MAX_NAMELENGTH, "%s ref %d", aclip->name, aclip->num_refs + 1);
	} else {
	    snprintf(tc->name, MAX_NAMELENGTH, "%s", aclip->name);
	}
	
    }
	break;
    case TCLIP_MIDI: {
	MIDIClip *mclip = source_clip;
	mcr = calloc(1, sizeof(MIDIClipRef));
	tc->obj = mcr;
	if (mclip->num_refs == mclip->refs_alloc_len) {
	    mclip->refs_alloc_len *= 2;
	    mclip->refs = realloc(mclip->refs, mclip->refs_alloc_len * sizeof(Clip *));
	}
	mclip->refs[mclip->num_refs] = mcr;
	mclip->num_refs++;

	if (mclip->num_refs > 0) {
	    snprintf(tc->name, MAX_NAMELENGTH, "%s ref %d", mclip->name, mclip->num_refs + 1);
	} else {
	    snprintf(tc->name, MAX_NAMELENGTH, "%s", mclip->name);
	}
    }
	break;
    }

    tc->layout = layout_add_child(track->inner_layout);
    tc->layout->h.type = SCALE;
    tc->layout->h.value = 0.96;
    /* pthread_mutex_init(&cr->lock, NULL); */
    /* cr->lock = SDL_CreateMutex(); */
    /* SDL_LockMutex(cr->lock); */
    /* pthread_mutex_lock(&cr->lock); */
    /* track->clips[track->num_clips] = cr; */
    /* track->num_clips++; */
    /* cr->clip = clip; */
    /* cr->track = track; */
    /* cr->home = home; */

    Layout *label_lt = layout_add_child(tc->layout);
    label_lt->x.value = CLIPREF_NAMELABEL_H_PAD;
    label_lt->y.value = CLIPREF_NAMELABEL_V_PAD;
    label_lt->w.type = SCALE;
    label_lt->w.value = 0.8f;
    label_lt->h.value = CLIPREF_NAMELABEL_H;
    tc->label = textbox_create_from_str(tc->name, label_lt, main_win->mono_bold_font, 12, main_win);
    tc->label->text->validation = txt_name_validation;
    tc->label->text->completion = project_obj_name_completion;

    textbox_set_align(tc->label, CENTER_LEFT);
    textbox_set_background_color(tc->label, NULL);
    textbox_set_text_color(tc->label, &colors.light_grey);
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
	track->clips = realloc(track->clips, track->clips_alloc_len * sizeof(TrackClip *));
    }
    track->clips[track->num_clips] = tc;

    return tc;
}

int32_t clipref_len(ClipRef *cr);
int32_t midi_clipref_len(MIDIClipRef *mcr);
void clipref_draw_waveform(ClipRef *cr);

int32_t tclip_len(TrackClip *tc)
{
    switch(tc->type) {
    case TCLIP_AUDIO:
	return clipref_len(tc->obj);
    case TCLIP_MIDI:
	return midi_clipref_len(tc->obj);
    }
}

void tclip_reset(TrackClip *tc, bool rescaled)
{
    tc->layout->rect.x = timeline_get_draw_x(tc->track->tl, tc->tl_pos);
    int32_t tc_len = tclip_len(tc);
    /* uint32_t tc_len = tc->in_mark_sframes >= tc->out_mark_sframes */
    /* 	? tc->clip->len_sframes */
    /* 	: tc->out_mark_sframes - tc->in_mark_sframes; */
    tc->layout->rect.w = timeline_get_draw_w(tc->track->tl, tc_len);
    tc->layout->rect.y = tc->track->inner_layout->rect.y + CR_RECT_V_PAD;
    tc->layout->rect.h = tc->track->inner_layout->rect.h - 2 * CR_RECT_V_PAD;
    layout_set_values_from_rect(tc->layout);
    layout_reset(tc->layout);
    textbox_reset_full(tc->label);
    /* if (rescaled) { */
    if (tc->type == TCLIP_AUDIO) {
	((ClipRef *)tc->obj)->waveform_redraw = true;
    }
}

void tclip_draw(TrackClip *tc)
{
    if (tc->deleted) {
	return;
    }
    Session *session = session_get();
    ClipRef *cr = NULL;
    MIDIClipRef *mcr = NULL;
    switch (tc->type) {
    case TCLIP_AUDIO:
	cr = tc->obj;
	break;
    case TCLIP_MIDI:
	mcr = tc->obj;
	break;
    }

    
    if (tc->grabbed && session->dragging) {
	tclip_reset(tc, false);
    }
    /* Only check horizontal out-of-bounds; track handles vertical */
    if (tc->layout->rect.x > main_win->w_pix || tc->layout->rect.x + tc->layout->rect.w < 0) {
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
	ClipRef *cr = tc->obj;
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

/* Moving tclips on the timeline */

static void tclip_remove_from_track(TrackClip *tc)
{
    bool displace = false;
    Track *track = tc->track;
    for (uint16_t i=0; i<track->num_clips; i++) {
	TrackClip *test = track->clips[i];
	if (test == tc) {
	    displace = true;
	} else if (displace && i > 0) {
	    track->clips[i - 1] = test;	    
	}
    }
    if (displace) track->num_clips--; /* else not found! */
}

static void tclip_displace(TrackClip *tc, int displace_by)
{
    Track *track = tc->track;
    Timeline *tl = track->tl;
    int new_rank = track->tl_rank + displace_by;
    if (new_rank >= 0 && new_rank < tl->num_tracks) {
	Track *new_track = tl->tracks[new_rank];
	tclip_remove_from_track(tc);
	/* clipref_remove_from_track(cr); */
	if (new_track->num_clips == new_track->clips_alloc_len) {
	    new_track->clips_alloc_len *= 2;
	    new_track->clips = realloc(new_track->clips, new_track->clips_alloc_len * sizeof(TrackClip *));
	}
	new_track->clips[new_track->num_clips] = tc;
	new_track->num_clips++;
	tc->track = new_track;
	/* fprintf(stdout, "ADD CLIP TO TRACK %s, which has %d clips now\n", new_track->name, new_track->num_clips); */
	
    }
    timeline_reset(tl, false);
}

static void tclip_insert_on_track(TrackClip *tc, Track *target)
{
    if (target->num_clips == target->clips_alloc_len) {
	target->clips_alloc_len *= 2;
	target->clips = realloc(target->clips, target->clips_alloc_len * sizeof(TrackClip *));	
    }
    target->clips[target->num_clips] = tc;
    target->num_clips++;
    tc->track = target;    
}

void tclip_move_to_track(TrackClip *tc, Track *target)
{
    Track *prev_track = tc->track;
    Timeline *tl = prev_track->tl;
    tclip_remove_from_track(tc);
    tclip_insert_on_track(tc, target);
    timeline_reset(tl, false);
}

void tclip_grab(TrackClip *tc)
{
    Timeline *tl = tc->track->tl;
    if (tl->num_grabbed_clips == MAX_GRABBED_CLIPS) {
	char errstr[MAX_STATUS_STRLEN];
	snprintf(errstr, MAX_STATUS_STRLEN, "Cannot grab >%d clips", MAX_GRABBED_CLIPS);
	status_set_errstr(errstr);
	return;
    }

    tl->grabbed_clips[tl->num_grabbed_clips] = tc;
    tl->num_grabbed_clips++;
    tc->grabbed = true;
}


void tclip_ungrab(TrackClip *tc)
{
    Timeline *tl = tc->track->tl;
    bool displace = false;
    for (uint8_t i=0; i<tl->num_grabbed_clips; i++) {
	if (displace) {
	    tl->grabbed_clips[i - 1] = tl->grabbed_clips[i];
	} else if (tc == tl->grabbed_clips[i]) {
	    displace = true;
	}
    }
    tc->grabbed = false;
    tl->num_grabbed_clips--;
    status_stat_drag();

}

void tclip_bring_to_front()
{
    Session *session = session_get();
    Timeline *tl = ACTIVE_TL;
    Track *track = timeline_selected_track(tl);
    if (!track) return;
    /* bool displace = false; */
    TrackClip *to_move = NULL;
    for (int i=0; i<track->num_clips; i++) {
	TrackClip *tc = track->clips[i];
	if (!to_move && tc->tl_pos <= tl->play_pos_sframes && tc->tl_pos + tclip_len(tc) >= tl->play_pos_sframes) {
	    to_move = tc;
	} else if (to_move) {
	    track->clips[i-1] = track->clips[i];
	    if (i == track->num_clips - 1) {
		track->clips[i] = to_move;
	    }
	}
    }
    tl->needs_redraw = true;
}

TrackClip *tclip_at_cursor()
{
    Session *session = session_get();
    Timeline *tl = ACTIVE_TL;
    Track *track = timeline_selected_track(tl);
    if (!track) return NULL;
    
    /* Reverse iter to ensure top-most clip is returned in case of overlap */
    for (int i=track->num_clips -1; i>=0; i--) {
	TrackClip *tc = track->clips[i];
	if (tc->tl_pos <= tl->play_pos_sframes && tc->tl_pos + tclip_len(tc) >= tl->play_pos_sframes) {
	    return tc;
	}
    }
    return NULL;
}


TrackClip *tclip_before_cursor(int32_t *pos_dst)
{
    Session *session = session_get();
    Timeline *tl = ACTIVE_TL;
    Track *track = timeline_selected_track(tl);
    if (!track) return NULL;
    if (track->num_clips == 0) return NULL;
    TrackClip *ret = NULL;
    int32_t end = INT32_MIN;
    for (int i=0; i<track->num_clips; i++) {
	TrackClip *tc = track->clips[i];
	int32_t tc_end = tc->tl_pos + tclip_len(tc);
	if (tc_end < tl->play_pos_sframes && tc_end >= end) {
	    ret = tc;
	    end = tc_end;
	}
    }
    *pos_dst = end;
    return ret;
}

TrackClip *tclip_after_cursor(int32_t *pos_dst)
{
    Session *session = session_get();
    Timeline *tl = ACTIVE_TL;
    Track *track = timeline_selected_track(tl);
    if (!track) return NULL;
    if (track->num_clips == 0) return NULL;
    TrackClip *ret = NULL;
    int32_t start = INT32_MAX;
    for (int i=0; i<track->num_clips; i++) {
	TrackClip *tc = track->clips[i];
	int32_t tc_start = tc->tl_pos;
	if (tc_start > tl->play_pos_sframes && tc_start <= start) {
	    start = tc_start;
	    *pos_dst = tc_start;
	    ret = tc;
	}
    }
    return ret;
}



static TrackClip *tclip_cut(TrackClip *tc, int32_t cut_pos_rel)
{
    /* ClipRef *new = tclip_add(cr->track, cr->clip, cr->pos_sframes + cut_pos_rel, false); */
    TrackClip *new = tclip_add(tc->track, tc->tl_pos + cut_pos_rel, tc->type, tc->source_clip);
    if (!new) {
	return NULL;
    }
    if (cut_pos_rel < 0) return NULL;
    new->in_mark_sframes = cr->in_mark_sframes + cut_pos_rel;
    new->out_mark_sframes = cr->out_mark_sframes == 0 ? clipref_len(cr) : cr->out_mark_sframes;
    Value orig_end_pos = {.int32_v = cr->out_mark_sframes};
    cr->out_mark_sframes = cr->out_mark_sframes == 0 ? cut_pos_rel : cr->out_mark_sframes - (clipref_len(cr) - cut_pos_rel);
    track_reset(cr->track, true);

    Value cut_pos = {.int32_v = cr->out_mark_sframes};
    user_event_push(
	undo_cut_clipref,
	redo_cut_clipref,
	NULL,
	cut_clip_dispose_forward,
	cr, new,
	cut_pos, orig_end_pos, cut_pos, orig_end_pos,
	0, 0, false, false);

    return new;
}
