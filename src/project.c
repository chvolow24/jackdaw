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
#include <string.h>

#include "audio_device.h"
#include "layout.h"
#include "project.h"
#include "slider.h"
#include "textbox.h"
#include "timeline.h"
#include "window.h"

#define DEFAULT_SFPP 600
#define CR_RECT_V_PAD (8 * main_win->dpi_scale_factor)
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
    {38, 125, 255, 255},
    {250, 190, 50, 255},
    {171, 38, 38, 255},
    {250, 132, 222, 255},
    {224, 142, 74, 255},
    {5, 100, 115, 255},
    {245, 240, 206, 255}
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

void project_init_audio_devices(Project *proj);
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


    project_init_audio_devices(proj);
    
    return proj;
}


int device_open(Project *proj, AudioDevice *dev);
void project_init_audio_devices(Project *proj)
{
    proj->num_playback_devices = query_audio_devices(proj, 0);
    proj->num_record_devices = query_audio_devices(proj, 1);
    proj->playback_device = proj->playback_devices[0];
    if (device_open(proj, proj->playback_device) != 0) {
	fprintf(stderr, "Error: failed to open default audio device \"%s\". More info: %s\n", proj->playback_device->name, SDL_GetError());
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


void timeline_add_track(Timeline *tl)
{
    Track *track = calloc(1, sizeof(Track));
    tl->tracks[tl->num_tracks] = track;
    track->tl_rank = tl->num_tracks++;
    track->tl = tl;
    sprintf(track->name, "Track %d", track->tl_rank + 1);

    track->channels = tl->proj->channels;

    track->input = tl->proj->record_devices[0];

    /* track->color.r = rand() % 255; */
    /* track->color.g = rand() % 255; */
    /* track->color.b = rand() % 255; */
    /* track->color.a = 255; */
    track->color = track_colors[track_color_index];
    if (track_color_index < NUM_TRACK_COLORS -1) {
	track_color_index++;
    } else {
	track_color_index = 0;
    }
    
    /* track->muted = false; */
    /* track->solo = false; */
    /* track->solo_muted = false; */
    /* track->active = false; */
    /* track->num_clips = 0; */
    /* track->num_grabbed_clips = 0; */
    /* track-> */

    Layout *track_template = layout_get_child_by_name_recursive(tl->layout, "track_container");
    if (!track_template) {
	fprintf(stderr, "Error: unable to find layout with name \"track_container\"\n");
	exit(1);
    }

    Layout *iteration = layout_add_iter(track_template, VERTICAL, true);
    track->layout = track_template->iterator->iterations[track->tl_rank];


    Layout *name, *mute, *solo, *vol_label, *pan_label, *in_label, *in_value;

    Layout *track_name_row = layout_get_child_by_name_recursive(track->layout, "track_name");
    name = layout_add_child(track_name_row);
    layout_pad(name, TRACK_NAME_H_PAD, TRACK_NAME_V_PAD);
    mute = layout_get_child_by_name_recursive(track->layout, "mute");
    solo = layout_get_child_by_name_recursive(track->layout, "solo");
    vol_label = layout_get_child_by_name_recursive(track->layout, "vol_label");
    pan_label = layout_get_child_by_name_recursive(track->layout, "pan_label");
    in_label = layout_get_child_by_name_recursive(track->layout, "in_label");
    in_value = layout_get_child_by_name_recursive(track->layout, "in_value");

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
}

void project_clear_active_clips()
{
    proj->active_clip_index = proj->num_clips;
}

Clip *project_add_clip(AudioDevice *dev, Track *target)
{
    if (proj->num_clips == MAX_PROJ_CLIPS) {
	return NULL;
    }
    Clip *clip = calloc(1, sizeof(Clip));

    if (dev) {
	clip->recorded_from = dev;
    }
    if (target) {
	clip->target = target;
    }
    
    if (!target && dev) {
	sprintf(clip->name, "%s_rec_%d", dev->name, proj->num_clips); /* TODO: Fix this */
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
