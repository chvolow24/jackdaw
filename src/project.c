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
#include "textbox.h"
#include "timeline.h"
#include "window.h"

#define DEFAULT_SFPP 600

extern Window *main_win;
extern Project *proj;
extern SDL_Color color_global_black;
extern SDL_Color color_global_white;

void project_add_timeline(Project *proj, char *name)
{
    if (proj->num_timelines == MAX_PROJ_TIMELINES) {
	fprintf(stdout, "Error: project has max num timlines\n:");
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
    
    /* new_tl->track_selector = 0; */
    
    proj->timelines[proj->num_timelines] = new_tl;
    proj->num_timelines++;
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
    project_add_timeline(proj, "Main");

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

void timeline_add_track(Timeline *tl)
{
    Track *track = calloc(1, sizeof(Track));
    tl->tracks[tl->num_tracks] = track;
    track->tl_rank = tl->num_tracks++;
    track->tl = tl;
    sprintf(track->name, "Track %d", track->tl_rank + 1);

    track->channels = tl->proj->channels;

    track->input = tl->proj->record_devices[0];
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

    name = layout_get_child_by_name_recursive(track->layout, "track_name");
    mute = layout_get_child_by_name_recursive(track->layout, "mute");
    solo = layout_get_child_by_name_recursive(track->layout, "solo");
    vol_label = layout_get_child_by_name_recursive(track->layout, "vol_label");
    pan_label = layout_get_child_by_name_recursive(track->layout, "pan_label");
    in_label = layout_get_child_by_name_recursive(track->layout, "in_label");
    in_value = layout_get_child_by_name_recursive(track->layout, "in_value");

    /* textbox_create_from_str(char *set_str, Layout *lt, Font *font, uint8_t text_size, Window *win) -> Textbox *
     */

    track->tb_name = textbox_create_from_str(
	track->name,
	name,
	main_win->std_font,
	16,
	main_win);

    track->tb_input_label = textbox_create_from_str(
	"In: ",
	in_label,
	main_win->std_font,
	12,
	main_win);

    track->tb_mute_button = textbox_create_from_str(
	"M",
	mute,
	main_win->std_font,
	16,
	main_win);

    track->tb_solo_button = textbox_create_from_str(
	"S",
	solo,
	main_win->std_font,
	16,
	main_win);

    track->console_rect = &(layout_get_child_by_name_recursive(track->layout, "track_console")->rect);

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

void clipref_reset(ClipRef *cr)
{
    cr->rect.x = timeline_get_draw_x(cr->pos_sframes);
    uint32_t cr_len = cr->out_mark_sframes >= cr->in_mark_sframes
	? cr->clip->len_sframes
	: cr->out_mark_sframes - cr->in_mark_sframes;
    cr->rect.w = timeline_get_draw_w(cr_len);
    cr->rect.y = cr->track->layout->rect.y;
    cr->rect.h = cr->track->layout->rect.h;

}


static void track_reset(Track *track)
{
    textbox_reset(track->tb_name);
    textbox_reset(track->tb_input_label);
    textbox_reset(track->tb_mute_button);
    textbox_reset(track->tb_solo_button);
    for (uint8_t i=0; i<track->num_clips; i++) {
	clipref_reset(track->clips[i]);
    }
}

ClipRef *track_create_clip_ref(Track *track, Clip *clip, int32_t record_from_sframes, bool home)
{
    ClipRef *cr = calloc(1, sizeof(ClipRef));
    sprintf(cr->name, "%s ref%d", clip->name, track->num_clips);
    track->clips[track->num_clips] = cr;
    track->num_clips++;
    cr->pos_sframes = record_from_sframes;
    cr->clip = clip;
    cr->track = track;
    cr->home = home;
    clip->refs[clip->num_refs] = cr;
    clip->num_refs++;
    return cr;
}

void timeline_reset(Timeline *tl)
{
    for (int i=0; i<tl->num_tracks; i++) {
	track_reset(tl->tracks[i]);
    }
}
