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


#include "audio_device.h"
#include "layout.h"
#include "project.h"
#include "window.h"

extern Window *main_win;

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
    project_add_timeline(proj, "Main");

    project_init_audio_devices(proj);
    
    return proj;
}


void project_init_audio_devices(Project *proj)
{
    proj->num_playback_devices = query_audio_devices(proj, 0);
    proj->num_record_devices = query_audio_devices(proj, 1);
}

void timeline_add_track(Timeline *tl)
{
    Track *track = calloc(1, sizeof(Track));
    tl->tracks[tl->num_tracks] = track;
    track->tl_rank = tl->num_tracks++;
    track->tl = tl;
    sprintf(track->name, "Track %d", track->tl_rank + 1);

    track->channels = tl->proj->channels;
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



void track_reset(Track *track)
{
    textbox_reset(track->tb_name);
    textbox_reset(track->tb_input_label);
    textbox_reset(track->tb_mute_button);
    textbox_reset(track->tb_solo_button);
}

void timeline_reset(Timeline *tl)
{
    for (int i=0; i<tl->num_tracks; i++) {
	track_reset(tl->tracks[i]);
    }
}
