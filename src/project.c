/*****************************************************************************************************************
  Jackdaw | a stripped-down, keyboard-focused Digital Audio Workstation | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023 Charlie Volow
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software iso
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
    project.c

    * functions modifying project-related structures that do not belong elsewhere
    * timeline.c contains operations related to visualizing time onscreen
 *****************************************************************************************************************/

#include <string.h>

#include "audio_connection.h"
#include "layout.h"
#include "menu.h"
#include "project.h"
#include "slider.h"
#include "textbox.h"
#include "timeline.h"
#include "window.h"

#define DEFAULT_SFPP 600
#define CR_RECT_V_PAD (4 * main_win->dpi_scale_factor)
#define NUM_TRACK_COLORS 7

#define TRACK_NAME_H_PAD 3
#define TRACK_NAME_V_PAD 3
#define TRACK_CTRL_SLIDER_H_PAD 7
#define TRACK_CTRL_SLIDER_V_PAD 5

#define TRACK_VOL_STEP 0.03
#define TRACK_PAN_STEP 0.01
extern Window *main_win;
extern Project *proj;
extern SDL_Color color_global_black;
extern SDL_Color color_global_white;
extern SDL_Color color_global_clear;

SDL_Color timeline_label_txt_color = {0, 200, 100, 255};

/* Alternating bright colors to easily distinguish tracks */
uint8_t track_color_index = 0;
SDL_Color track_colors[7] = {
    {5, 100, 115, 255},
    {38, 125, 240, 255},
    {70, 30, 130, 255},
    {171, 38, 38, 255},
    {224, 142, 74, 255},
    {250, 110, 100, 255},
    {60, 200, 150, 255}
};

uint8_t project_add_timeline(Project *proj, char *name)
{
    if (proj->num_timelines == MAX_PROJ_TIMELINES) {
	fprintf(stdout, "Error: project has max num timlines\n:");
	return proj->active_tl_index;
    }
    Timeline *new_tl = calloc(1, sizeof(Timeline));
    if (!new_tl) {
	fprintf(stderr, "Error: unable to allocate space for timeline.\n");
	exit(1);
    }
    if (strlen(name) > MAX_NAMELENGTH - 1) {
	fprintf(stderr, "Error: timeline name exceeds max len (%d)\n", MAX_NAMELENGTH);
	exit(1);
    }
    strcpy(new_tl->name, name);
    new_tl->proj = proj;
    new_tl->layout = layout_get_child_by_name_recursive(proj->layout, "timeline");
    new_tl->sample_frames_per_pixel = DEFAULT_SFPP;
    strcpy(new_tl->timecode.str, "+00:00:00:00000");
    Layout *tc_lt = layout_get_child_by_name_recursive(proj->layout, "timecode");
    new_tl->timecode_tb = textbox_create_from_str(
	new_tl->timecode.str,
	tc_lt,
	main_win->std_font,
	18,
	main_win);
    textbox_set_background_color(new_tl->timecode_tb, &color_global_black);
    textbox_set_text_color(new_tl->timecode_tb, &color_global_white);
    textbox_set_trunc(new_tl->timecode_tb, false);
    /* textbox_size_to_fit(new_tl->timecode_tb, 0, 0); */
    
    /* new_tl->track_selector = 0; */
    
    proj->timelines[proj->num_timelines] = new_tl;
    proj->num_timelines++;
    return proj->num_timelines - 1; /* Return the new timeline index */
}

void project_reset_tl_label(Project *proj)
{
    Timeline *tl = proj->timelines[proj->active_tl_index];
    snprintf(proj->timeline_label_str, PROJ_TL_LABEL_BUFLEN, "Timeline %d: \"%s\"\n", proj->active_tl_index + 1, tl->name);
    textbox_reset_full(proj->timeline_label);
}

void project_init_audio_conns(Project *proj);
Project *project_create(
    char *name,
    uint8_t channels,
    uint32_t sample_rate,
    SDL_AudioFormat fmt,
    uint16_t chunk_size_sframes
    )
{
    Project *proj = calloc(1, sizeof(Project));
    if (!proj) {
	fprintf(stderr, "Error: unable to allocate space for project.\n");
	exit(1);
    }
    if (strlen(name) > MAX_NAMELENGTH - 1) {
	fprintf(stderr, "Error: project name exceeds max len (%d)\n", MAX_NAMELENGTH);
	exit(1);
    }
    
    strcpy(proj->name, name);
    proj->channels = channels;
    proj->sample_rate = sample_rate;
    proj->fmt = fmt;
    proj->chunk_size_sframes = chunk_size_sframes;
    proj->layout = main_win->layout;
    proj->audio_rect = &(layout_get_child_by_name_recursive(proj->layout, "audio_rect")->rect);
    proj->control_bar_rect = &(layout_get_child_by_name_recursive(proj->layout, "control_bar")->rect);
    proj->ruler_rect = &(layout_get_child_by_name_recursive(proj->layout, "ruler")->rect);
    Layout *source_lt = layout_get_child_by_name_recursive(proj->layout, "source_area");
    proj->source_rect = &source_lt->rect;
    proj->source_clip_rect = &(layout_get_child_by_name_recursive(source_lt, "source_clip")->rect);
    Layout *src_name_lt = layout_get_child_by_name_recursive(source_lt, "source_clip_name");
    proj->source_name_tb = textbox_create_from_str(
	"(no source clip)",
	src_name_lt,
	main_win->std_font,
	14,
	main_win
	);

    textbox_set_align(proj->source_name_tb, CENTER_LEFT);
    textbox_set_background_color(proj->source_name_tb, &color_global_clear);
    textbox_set_text_color(proj->source_name_tb, &color_global_white);

    project_add_timeline(proj, "Main");
    Layout *timeline_label_lt = layout_get_child_by_name_recursive(proj->layout, "timeline_label");
    strcpy(proj->timeline_label_str, "Timeline 1: \"Main\"");
    proj->timeline_label = textbox_create_from_str(
	proj->timeline_label_str,
	timeline_label_lt,
	main_win->bold_font,
	12,
	main_win
	);
    textbox_set_text_color(proj->timeline_label, &timeline_label_txt_color);
    textbox_set_background_color(proj->timeline_label, &color_global_clear);
    textbox_set_align(proj->timeline_label, CENTER_LEFT);
    project_reset_tl_label(proj);
    /* Initialize output */
    proj->output_len = chunk_size_sframes;
    proj->output_L = malloc(sizeof(float) * chunk_size_sframes);
    proj->output_R = malloc(sizeof(float) * chunk_size_sframes);
    memset(proj->output_L, '\0', sizeof(float) * chunk_size_sframes);
    memset(proj->output_R, '\0', sizeof(float) * chunk_size_sframes);


    project_init_audio_conns(proj);

    Layout *out_label_lt, *out_value_lt;
    out_label_lt = layout_get_child_by_name_recursive(proj->layout, "default_out_label");
    out_value_lt = layout_get_child_by_name_recursive(proj->layout, "default_out_value");
    proj->tb_out_label = textbox_create_from_str(
	"Default Out:",
	out_label_lt,
	main_win->bold_font,
	12,
	main_win);
    textbox_set_align(proj->tb_out_label, CENTER_LEFT);
    textbox_set_background_color(proj->tb_out_label, &color_global_clear);
    textbox_set_text_color(proj->tb_out_label, &color_global_white);
    
    proj->tb_out_value = textbox_create_from_str(
	(char *)proj->playback_conn->name,
	out_value_lt,
	main_win->std_font,
	12,
	main_win);
    textbox_set_align(proj->tb_out_value, CENTER_LEFT);
    proj->tb_out_value->corner_radius = 6;
    /* textbox_set_align(proj->tb_out_value, CENTER); */
    int saved_w = proj->tb_out_value->layout->rect.w / main_win->dpi_scale_factor;
    textbox_size_to_fit(proj->tb_out_value, 2, 1);
    textbox_set_fixed_w(proj->tb_out_value, saved_w - 10);
    textbox_set_border(proj->tb_out_value, &color_global_black, 1);

    Layout *output_l_lt, *output_r_lt;
    output_l_lt = layout_get_child_by_name_recursive(proj->layout, "out_waveform_left");
    output_r_lt = layout_get_child_by_name_recursive(proj->layout, "out_waveform_right");

    proj->outwav_l_rect = &output_l_lt->rect;
    proj->outwav_r_rect = &output_r_lt->rect;
	
    return proj;
}


int audioconn_open(Project *proj, AudioConn *dev);
void project_init_audio_conns(Project *proj)
{
    proj->num_playback_conns = query_audio_connections(proj, 0);
    proj->num_record_conns = query_audio_connections(proj, 1);
    proj->playback_conn = proj->playback_conns[0];
    if (audioconn_open(proj, proj->playback_conn) != 0) {
	fprintf(stderr, "Error: failed to open default audio conn \"%s\". More info: %s\n", proj->playback_conn->name, SDL_GetError());
    }
}


static float amp_to_db(float amp)
{
    return (20.0f * log10(amp));
}

static void slider_label_amp_to_dbstr(char *dst, size_t dstsize, float amp)
{
    int max_float_chars = dstsize - 2;
    if (max_float_chars < 1) {
	fprintf(stderr, "Error: no room for dbstr\n");
	dst[0] = '\0';
	return;
    }
    snprintf(dst, max_float_chars, "%f", amp_to_db(amp));
    strcat(dst, " dB");
}

static void slider_label_plain_str(char *dst, size_t dstsize, float value)
{
    snprintf(dst, dstsize, "%f", value);

}


Track *timeline_add_track(Timeline *tl)
{
    Track *track = calloc(1, sizeof(Track));
    tl->tracks[tl->num_tracks] = track;
    track->tl_rank = tl->num_tracks++;
    track->tl = tl;
    sprintf(track->name, "Track %d", track->tl_rank + 1);

    track->channels = tl->proj->channels;

    track->input = tl->proj->record_conns[0];
    track->color = track_colors[track_color_index];
    if (track_color_index < NUM_TRACK_COLORS -1) {
	track_color_index++;
    } else {
	track_color_index = 0;
    }
    
    Layout *track_template = layout_get_child_by_name_recursive(tl->layout, "track_container");
    if (!track_template) {
	fprintf(stderr, "Error: unable to find layout with name \"track_container\"\n");
	exit(1);
    }

    Layout *iteration = layout_add_iter(track_template, VERTICAL, true);
    track->layout = track_template->iterator->iterations[track->tl_rank];

    Layout *name, *mute, *solo, *vol_label, *pan_label, *in_label, *in_value;

    Layout *track_name_row = layout_get_child_by_name_recursive(track->layout, "name_value");
    name = layout_add_child(track_name_row);
    layout_pad(name, TRACK_NAME_H_PAD, TRACK_NAME_V_PAD);
    mute = layout_get_child_by_name_recursive(track->layout, "mute_button");
    solo = layout_get_child_by_name_recursive(track->layout, "solo_button");
    vol_label = layout_get_child_by_name_recursive(track->layout, "vol_label");
    pan_label = layout_get_child_by_name_recursive(track->layout, "pan_label");
    in_label = layout_get_child_by_name_recursive(track->layout, "in_label");
    in_value = layout_get_child_by_name_recursive(track->layout, "in_value");

    /* fprintf(stdout, "Addrs: %p, %p, %p, %p, %p, %p, %p\n", name, mute, solo, vol_label, pan_label, in_label, in_value); */
    /* textbox_create_from_str(char *set_str, Layout *lt, Font *font, uint8_t text_size, Window *win) -> Textbox *
     */

    track->tb_vol_label = textbox_create_from_str(
	"Vol:",
	vol_label,
	main_win->bold_font,
	12,
	main_win);
    textbox_set_background_color(track->tb_vol_label, &color_global_clear);

    track->tb_pan_label = textbox_create_from_str(
	"Pan:",
	pan_label,
	main_win->bold_font,
	12,
	main_win);
    textbox_set_background_color(track->tb_pan_label, &color_global_clear);

    track->tb_name = textbox_create_from_str(
	track->name,
	name,
	main_win->bold_font,
	14,
	main_win);

    textbox_set_align(track->tb_name, CENTER_LEFT);
    textbox_set_pad(track->tb_name, 4, 0);
    textbox_set_border(track->tb_name, &color_global_black, 1);

    track->tb_input_label = textbox_create_from_str(
	"In: ",
	in_label,
	main_win->bold_font,
	12,
	main_win);
    textbox_set_background_color(track->tb_input_label, &color_global_clear);
    

    track->tb_input_name = textbox_create_from_str(
	(char *)track->input->name,
	in_value,
	main_win->std_font,
	12,
	main_win);
    track->tb_input_name->corner_radius = 6;
    /* textbox_set_align(track->tb_input_name, CENTER); */
    int saved_w = track->tb_input_name->layout->rect.w / main_win->dpi_scale_factor;
    textbox_size_to_fit(track->tb_input_name, 2, 1);
    textbox_set_fixed_w(track->tb_input_name, saved_w - 10);
    textbox_set_border(track->tb_input_name, &color_global_black, 1);

    
    track->tb_mute_button = textbox_create_from_str(
	"M",
	mute,
	main_win->bold_font,
	14,
	main_win);
    track->tb_mute_button->corner_radius = 4;
    textbox_set_border(track->tb_mute_button, &color_global_black, 1);

    track->tb_solo_button = textbox_create_from_str(
	"S",
	solo,
	main_win->bold_font,
	14,
	main_win);
    track->tb_solo_button->corner_radius = 4;
    textbox_set_border(track->tb_solo_button, &color_global_black, 1);


    Layout *vol_ctrl_row = layout_get_child_by_name_recursive(track->layout, "vol_slider");
    Layout *vol_ctrl_lt = layout_add_child(vol_ctrl_row);

    layout_pad(vol_ctrl_lt, TRACK_CTRL_SLIDER_H_PAD, TRACK_CTRL_SLIDER_V_PAD);
    /* vol_ctrl_lt->x.value.intval = TRACK_CTRL_SLIDER_H_PAD; */
    /* vol_ctrl_lt->y.value.intval = TRACK_CTRL_SLIDER_V_PAD; */
    /* vol_ctrl_lt->w.value.intval = vol_ctrl_row->w.value.intval - TRACK_CTRL_SLIDER_H_PAD * 2; */
    /* vol_ctrl_lt->h.value.intval = vol_ctrl_row->h.value.intval - TRACK_CTRL_SLIDER_V_PAD * 2; */
    /* layout_set_values_from_rect(vol_ctrl_lt); */
    
    track->vol = 1.0f;
    track->vol_ctrl = fslider_create(
	&track->vol,
	vol_ctrl_lt,
	SLIDER_HORIZONTAL,
	SLIDER_FILL,
	slider_label_amp_to_dbstr);
    fslider_set_range(track->vol_ctrl, 0.0f, 3.0f);

    Layout *pan_ctrl_row = layout_get_child_by_name_recursive(track->layout, "pan_slider");
    Layout *pan_ctrl_lt = layout_add_child(pan_ctrl_row);

    layout_pad(pan_ctrl_lt, TRACK_CTRL_SLIDER_H_PAD, TRACK_CTRL_SLIDER_V_PAD);
    /* pan_ctrl_lt->x.value.intval = TRACK_CTRL_SLIDER_H_PAD; */
    /* pan_ctrl_lt->y.value.intval = TRACK_CTRL_SLIDER_V_PAD; */
    /* pan_ctrl_lt->w.value.intval = pan_ctrl_row->w.value.intval - TRACK_CTRL_SLIDER_H_PAD * 2; */
    /* pan_ctrl_lt->h.value.intval = pan_ctrl_row->h.value.intval - TRACK_CTRL_SLIDER_V_PAD * 2; */
    /* /\* layout_set_values_from_rect(pan_ctrl_lt); *\/ */
    track->pan = 0.5f;
    track->pan_ctrl = fslider_create(
	&track->pan,
	pan_ctrl_lt,
	SLIDER_HORIZONTAL,
	SLIDER_TICK,
	slider_label_plain_str);

    fslider_reset(track->vol_ctrl);
    fslider_reset(track->pan_ctrl);
    

    track->console_rect = &(layout_get_child_by_name_recursive(track->layout, "track_console")->rect);
    track->colorbar = &(layout_get_child_by_name_recursive(track->layout, "colorbar")->rect);
    return track;
}

void project_clear_active_clips()
{
    proj->active_clip_index = proj->num_clips;
}

Clip *project_add_clip(AudioConn *conn, Track *target)
{
    if (proj->num_clips == MAX_PROJ_CLIPS) {
	return NULL;
    }
    Clip *clip = calloc(1, sizeof(Clip));

    if (conn) {
	clip->recorded_from = conn;
    }
    if (target) {
	clip->target = target;
    }
    
    if (!target && conn) {
	sprintf(clip->name, "%s_rec_%d", conn->name, proj->num_clips); /* TODO: Fix this */
    } else if (target) {
	sprintf(clip->name, "%s take %d", target->name, target->num_takes);
	target->num_takes++;
    } else {
	sprintf(clip->name, "anonymous");
    }
    clip->channels = proj->channels;
    proj->clips[proj->num_clips] = clip;
    proj->num_clips++;
    return clip;
}

int32_t clip_ref_len(ClipRef *cr)
{
    if (cr->out_mark_sframes <= cr->in_mark_sframes) {
	return cr->clip->len_sframes;
    } else {
	return cr->out_mark_sframes - cr->in_mark_sframes;
    }
}


void clipref_reset(ClipRef *cr)
{
    cr->rect.x = timeline_get_draw_x(cr->pos_sframes);
    uint32_t cr_len = cr->in_mark_sframes >= cr->out_mark_sframes
	? cr->clip->len_sframes
	: cr->out_mark_sframes - cr->in_mark_sframes;
    cr->rect.w = timeline_get_draw_w(cr_len);
    cr->rect.y = cr->track->layout->rect.y + CR_RECT_V_PAD;
    cr->rect.h = cr->track->layout->rect.h - 2 * CR_RECT_V_PAD;
}

static void clipref_remove_from_track(ClipRef *cr)
{
    bool displace = false;
    Track *track = cr->track;
    for (uint8_t i=0; i<track->num_clips; i++) {
	ClipRef *test = track->clips[i];
	if (test == cr) {
	    displace = true;
	} else if (displace && i > 0) {
	    track->clips[i - 1] = test;
	    
	}
    }
    track->num_clips--;
}

void clipref_displace(ClipRef *cr, int displace_by)
{
    Track *track = cr->track;
    Timeline *tl = track->tl;
    int new_rank = track->tl_rank + displace_by;
    if (new_rank >= 0 && new_rank < tl->num_tracks) {
	Track *new_track = tl->tracks[new_rank];
	clipref_remove_from_track(cr);
	new_track->clips[new_track->num_clips] = cr;
	new_track->num_clips++;
	cr->track = new_track;
	fprintf(stdout, "ADD CLIP TO TRACK %s, which has %d clips now\n", new_track->name, new_track->num_clips);
	
    }
}

static void track_reset_full(Track *track)
{
    textbox_reset_full(track->tb_name);
    textbox_reset_full(track->tb_input_label);
    textbox_reset_full(track->tb_mute_button);
    textbox_reset_full(track->tb_solo_button);
    textbox_reset_full(track->tb_vol_label);
    textbox_reset_full(track->tb_pan_label);
    textbox_reset_full(track->tb_input_label);
    textbox_reset_full(track->tb_input_name);
    for (uint8_t i=0; i<track->num_clips; i++) {
	clipref_reset(track->clips[i]);
    }
    fslider_reset(track->vol_ctrl);
    fslider_reset(track->pan_ctrl);
}
    

static void track_reset(Track *track)
{

    textbox_reset(track->tb_name);
    textbox_reset(track->tb_input_label);
    textbox_reset(track->tb_mute_button);
    textbox_reset(track->tb_solo_button);
    textbox_reset(track->tb_vol_label);
    textbox_reset(track->tb_pan_label);
    textbox_reset(track->tb_input_label);
    textbox_reset(track->tb_input_name);
    for (uint8_t i=0; i<track->num_clips; i++) {
	clipref_reset(track->clips[i]);
    }
}

ClipRef *track_create_clip_ref(Track *track, Clip *clip, int32_t record_from_sframes, bool home)
{
    ClipRef *cr = calloc(1, sizeof(ClipRef));
    
    sprintf(cr->name, "%s ref%d", clip->name, track->num_clips);
    cr->lock = SDL_CreateMutex();
    SDL_LockMutex(cr->lock);
    track->clips[track->num_clips] = cr;
    track->num_clips++;
    cr->pos_sframes = record_from_sframes;
    cr->clip = clip;
    cr->track = track;
    cr->home = home;
    clip->refs[clip->num_refs] = cr;
    clip->num_refs++;
    SDL_UnlockMutex(cr->lock);
    return cr;
}

void timeline_reset_full(Timeline *tl)
{
    for (int i=0; i<tl->num_tracks; i++) {
	track_reset_full(tl->tracks[i]);
    }
}

void timeline_reset(Timeline *tl)
{
    for (int i=0; i<tl->num_tracks; i++) {
	track_reset(tl->tracks[i]);
    }
}

void track_increment_vol(Track *track)
{
    track->vol += TRACK_VOL_STEP;
    if (track->vol > track->vol_ctrl->max) {
	track->vol = track->vol_ctrl->max;
    }
    fslider_reset(track->vol_ctrl);
}
void track_decrement_vol(Track *track)
{
    track->vol -= TRACK_VOL_STEP;
    if (track->vol < 0.0f) {
	track->vol = 0.0f;
    }
    fslider_reset(track->vol_ctrl);
}

void track_increment_pan(Track *track)
{
    track->pan += TRACK_PAN_STEP;
    if (track->pan > 1.0f) {
	track->pan = 1.0f;
    }
    fslider_reset(track->pan_ctrl);
}

void track_decrement_pan(Track *track)
{
    track->pan -= TRACK_PAN_STEP;
    if (track->pan < 0.0f) {
	track->pan = 0.0f;
    }
    fslider_reset(track->pan_ctrl);
}

/* SDL_Color mute_red = {255, 0, 0, 100}; */
/* SDL_Color solo_yellow = {255, 200, 0, 130}; */

SDL_Color mute_red = {255, 0, 0, 180};
SDL_Color solo_yellow = {255, 200, 0, 180};
extern SDL_Color textbox_default_bckgrnd_clr;

bool track_mute(Track *track)
{
    track->muted = !track->muted;
    if (track->muted) {
	textbox_set_background_color(track->tb_mute_button, &mute_red);
    } else {
	textbox_set_background_color(track->tb_mute_button, &textbox_default_bckgrnd_clr);
    }
    return track->muted;
}

bool track_solo(Track *track)
{
    track->solo = !track->solo;
    if (track->solo) {
	if (track->muted) {
	    track->muted = false;
	    textbox_set_background_color(track->tb_mute_button, &textbox_default_bckgrnd_clr);
	}
	track->solo_muted = false;
	textbox_set_background_color(track->tb_solo_button, &solo_yellow);
    } else {
	textbox_set_background_color(track->tb_solo_button, &textbox_default_bckgrnd_clr);

    }
    return track->solo;
}

void track_solomute(Track *track)
{
    track->solo_muted = true;
    textbox_set_background_color(track->tb_solo_button, &mute_red);
}
void track_unsolomute(Track *track)
{
    track->solo_muted = false;
    textbox_set_background_color(track->tb_solo_button, &textbox_default_bckgrnd_clr);
}

void track_or_tracks_solo(Timeline *tl, Track *opt_track)
{
    if (tl->num_tracks == 0) return;
    bool has_active_track = false;
    bool all_solo = true;
    int solo_count = 0;
    Track *track;
    for (uint8_t i=0; i<tl->num_tracks; i++) {
	track = tl->tracks[i];
	if (track->solo) {
	    solo_count++;
	}
	if (track->active) {
	    has_active_track = true;
	    if (!opt_track && !track->solo) {
		has_active_track = true;
		all_solo = false;
		track_solo(track);
		solo_count++;
	    }
	}
    }
    if (!has_active_track || opt_track) {
	track = opt_track ? opt_track : tl->tracks[tl->track_selector];
	if (track_solo(track)) {
	    all_solo = false;
	    solo_count++;
	} else {
	    solo_count--;
	}
    } else if (all_solo) {
	for (uint8_t i=0; i<tl->num_tracks; i++) {
	    track = tl->tracks[i];
	    if (track->active) {
		track_solo(track); /* unsolo */
		solo_count--;
		
	    }
	}
    }
    if (solo_count > 0) {
	for (uint8_t i=0; i<tl->num_tracks; i++) {
	    track = tl->tracks[i];
	    if (!track->solo) {
		track_solomute(track);
	    }
	}
    } else {
	for (uint8_t i=0; i<tl->num_tracks; i++) {
	    track = tl->tracks[i];
	    if (!track->solo) {
		track_unsolomute(track);
	    }
	}
    }

}

void track_or_tracks_mute(Timeline *tl, Track *track)
{

}

struct track_in_arg {
    Track *track;
    AudioConn *conn;
};
static void track_set_in_onclick(void *void_arg)
{
    /* struct select_dev_onclick_arg *carg = (struct select_dev_onclick_arg *)arg; */
    /* Track *track = carg->track; */
    /* AudioConn *dev = carg->new_in; */
    struct track_in_arg *arg = (struct track_in_arg *)void_arg;
    arg->track->input = arg->conn;
    textbox_set_value_handle(arg->track->tb_input_name, arg->track->input->name);

    window_pop_menu(main_win);
    window_pop_mode(main_win);
}

void track_set_input(Track *track)
{
    SDL_Rect *rect = &(track->tb_input_name->layout->rect);
    int y = rect->y;
    Menu *menu = menu_create_at_point(rect->x, rect->y);
    MenuColumn *c = menu_column_add(menu, "");
    MenuSection *sc = menu_section_add(c, "");
    
    for (int i=0; i<proj->num_record_conns; i++) {
	struct track_in_arg *arg = malloc(sizeof(struct track_in_arg));
	arg->track = track;
	arg->conn = proj->record_conns[i];
	MenuItem *item = menu_item_add(
	    sc,
	    arg->conn->name,
	    " ",
	    track_set_in_onclick,
	    arg);
	item->free_target_on_destroy = true;
    }
    menu_add_header(menu,"", "Select audio input for track.\n\n'n' to select next item; 'p' to select previous item.");
    /* menu_reset_layout(menu); */
    window_push_mode(main_win, MENU_NAV);
    window_add_menu(main_win, menu);
    int move_by_y = 0;
    if ((move_by_y = y + menu->layout->rect.h - main_win->layout->rect.h) > 0) {
	menu_translate(menu, 0, -1 * move_by_y / main_win->dpi_scale_factor);
    }
}

void project_draw();
void track_rename(Track *track)
{
    txt_edit(track->tb_name->text, project_draw);
    main_win->i_state = 0;
}

static int32_t clipref_len(ClipRef *cr)
{
    if (cr->out_mark_sframes <= cr->in_mark_sframes) {
	return cr->clip->len_sframes;
    } else {
	return cr->out_mark_sframes - cr->in_mark_sframes;
    }
}

ClipRef *clipref_at_point_in_track(Track *track)
{
    for (int i=track->num_clips-1; i>=0; i--) {
	ClipRef *cr = track->clips[i];
	if (cr->pos_sframes <= track->tl->play_pos_sframes && cr->pos_sframes + clipref_len(cr) >= track->tl->play_pos_sframes) {
	    return cr;
	}
    }
    return NULL;
}

ClipRef *clipref_at_point()
{
    Timeline *tl = proj->timelines[proj->active_tl_index];
    Track *track = tl->tracks[tl->track_selector];
    if (!track) return NULL;
    for (int i=track->num_clips -1; i>=0; i--) {
	ClipRef *cr = track->clips[i];
	if (cr->pos_sframes <= tl->play_pos_sframes && cr->pos_sframes + clipref_len (cr) >= tl->play_pos_sframes) {
	    return cr;
	}
    }
    return NULL;
}

void clipref_grab(ClipRef *cr)
{
    Timeline *tl = cr->track->tl;
    tl->grabbed_clips[tl->num_grabbed_clips] = cr;
    tl->num_grabbed_clips++;
    cr->grabbed = true;
}

void timeline_ungrab_all_cliprefs(Timeline *tl)
{
    for (uint8_t i=0; i<tl->num_grabbed_clips; i++) {
	tl->grabbed_clips[i]->grabbed = false;
    }
    tl->num_grabbed_clips = 0;

}

