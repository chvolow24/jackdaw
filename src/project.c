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
    project.c

    * Create and destroy project objects, incl Project, Track, Timeline, and Clip
    * Retrieve audio data from the timeline
 *****************************************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "SDL_events.h"
#include "SDL_video.h"
#include "project.h"
#include "gui.h"
#include "theme.h"
#include "audio.h"
#include "draw.h"
#include "timeline.h"

#define DEFAULT_TRACK_HEIGHT (108 * scale_factor)
#define DEFAULT_SFPP 600 // sample frames per pixel

extern Project *proj;
extern uint8_t scale_factor;

extern JDAW_Color clear;
extern JDAW_Color white;
extern JDAW_Color lightergrey;
extern JDAW_Color black;

/* Alternating bright colors to easily distinguish tracks */
JDAW_Color trck_colors[7] = {
    {{180, 40, 40, 255}, {180, 40, 40, 255}},
    {{226, 151, 0, 255}, {226, 151, 0, 255}},
    {{15, 228, 0, 255}, {15, 228, 0, 255}},
    {{0, 226, 219, 255}, {0, 226, 219, 255}},
    {{0, 98, 226, 255}, {0, 98, 226, 255}},
    {{128, 0, 226, 255}, {128, 0, 226, 255}},
    {{226, 100, 226, 255}, {226, 100, 226, 255}}
};

int trck_color_index = 0;

void rename_track(Textbox *tb, void *track_v)
{
    Track *track = (Track *)track_v;
    track->name_box->bckgrnd_color = &white;
    track->name_box->show_cursor = true;
    track->name_box->cursor_countdown = CURSOR_COUNTDOWN;
    track->name_box->cursor_pos = strlen(track->name);
    edit_textbox(track->name_box);
    track->name_box->bckgrnd_color = &lightergrey;
    track->name_box->show_cursor = false;
}

void select_track_input(Textbox *tb, void *track_v)
{
    fprintf(stderr, "SELECT track input on %p. Clicked value: %s\n", track_v, tb->value);
    Track *track = (Track *)track_v;
    for (uint8_t i=0; i<proj->num_record_devices; i++) {
        AudioDevice *rd = proj->record_devices[i];
        if (strcmp(rd->name, tb->value) == 0) {
            track->input = rd;
            fprintf(stderr, "Resetting track input to %s\n", rd->name);
            reset_textbox_value(track->input_name_box, (char *) rd->name);
            return;
        }
    }
}

void select_track_input_menu(void *track_v)
{
    Track *track = (Track *)track_v;
    MenulistItem *device_mlitems[proj->num_record_devices];
    for (uint8_t i=0; i<proj->num_record_devices; i++) {
        device_mlitems[i] = malloc(sizeof(MenulistItem)); //TODO: Danger: memory leak
        fprintf(stderr, "About to do strcpy\n");
        strcpy(device_mlitems[i]->label, proj->record_devices[i]->name);
        fprintf(stderr, "strcpy done\n");
        device_mlitems[i]->available = proj->record_devices[i]->open;
    }
    TextboxList *tbl = create_menulist(
        proj->jwin,
        0,
        5,
        proj->jwin->fonts[2],
        device_mlitems,
        proj->num_record_devices,
        select_track_input,
        track
    );
    Textbox * tb = NULL;
    position_textbox_list(tbl, track->input_name_box->container.x, track->input_name_box->container.y);
}

/* Query track clips and return audio sample representing a given point in the timeline. */
int16_t get_track_sample(Track *track, Timeline *tl, uint32_t start_pos, uint32_t pos_in_chunk)
{
    if (track->muted) {
        return 0;
    }
    int16_t sample = 0;
    for (int i=0; i < track->num_clips; i++) {
        Clip *clip = (track->clips)[i];
        if (!(clip->done)) {
            break;
        }
        int32_t pos_in_clip = start_pos + pos_in_chunk - clip->absolute_position;
        if (pos_in_clip >= 0 && pos_in_clip < clip->length) {
            sample += (clip->post_proc)[pos_in_clip];
        }
    }

    return sample;
}

/* 
Sum track samples over a chunk of timeline and return an array of samples. from_mark_in indicates that samples
should be collected from the in mark rather than from the play head.
*/
int16_t *get_mixdown_chunk(Timeline* tl, uint32_t length, bool from_mark_in)
{
    uint32_t bytelen = sizeof(int16_t) * length;
    int16_t *mixdown = malloc(bytelen);
    memset(mixdown, '\0', bytelen);
    if (!mixdown) {
        fprintf(stderr, "\nError: could not allocate mixdown chunk.");
        exit(1);
    }

    int i=0;
    float j=0;
    uint32_t start_pos = from_mark_in ? proj->tl->in_mark : proj->tl->play_position;
    while (i < length) {
        for (int t=0; t<tl->num_tracks; t++) {
            mixdown[i] += get_track_sample((tl->tracks)[t], tl, start_pos, (int) j);
        }
        j += from_mark_in ? 1 : proj->play_speed;
        i++;
    }
    proj->tl->play_position += length * proj->play_speed;
    // }
    // fprintf(stderr, "\t->exit get_mixdown_chunk\n");
    return mixdown;
}

/* Reset the cached size of the window. Called whenever the window is resized (see event loop in main.c) */
// void reset_winrect(Project *proj) {
//     SDL_GL_GetDrawableSize(proj->win, &((proj->winrect).w), &((proj->winrect).h));
// }

/* An active project is required for jackdaw to run. This function creates a project based on various specifications,
and initalizes a variety of UI "globals." */


Project *create_empty_project() 
{
    /* Allocate space for project and set name */
    Project *proj = malloc(sizeof(Project));
    memset(proj->name, '\0', MAX_NAMELENGTH);

    /* Set audio settings */
    proj->channels = 0;
    proj->sample_rate = 0;
    proj->fmt = 0;
    proj->chunk_size = 0;

    /* Initialize play/record values */
    proj->play_speed = 0;
    proj->playing = false;
    proj->recording = false;
    proj->num_active_clips = 0;

    /* Initialize timeline struct */
    Timeline *tl = (Timeline *)malloc(sizeof(Timeline));
    tl->num_tracks = 0;
    tl->play_position = 0;
    tl->record_offset = 0;
    tl->play_offset = 0;
    tl->tempo = 0;
    tl->click_on = 0;
    tl->sample_frames_per_pixel = DEFAULT_SFPP;
    tl->offset = 0; // horizontal offset of timeline in samples
    tl->v_offset = 0;
    tl->num_active_tracks = 0;
    memset(tl->active_track_indices, '\0', MAX_ACTIVE_TRACKS);
    proj->tl = tl;
    tl->proj = proj;

    proj->jwin = NULL;
    // /* Create SDL_Window and accompanying SDL_Renderer */
    // char project_window_name[MAX_NAMELENGTH + 10];
    // sprintf(project_window_name, "Jackdaw | %s", name);
    // proj->jwin = create_jwin(project_window_name, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED-20, 900, 650);

    // tl->console_width = TRACK_CONSOLE_WIDTH;
    // tl->rect = get_rect((SDL_Rect){0, 0, proj->jwin->w, proj->jwin->h,}, TL_RECT);
    // tl->audio_rect = (SDL_Rect) {tl->rect.x + TRACK_CONSOLE_WIDTH + COLOR_BAR_W + PADDING, tl->rect.y, tl->rect.w, tl->rect.h}; // SET x in track

    return proj;
}


Project *create_project(const char* name, uint8_t channels, int sample_rate, SDL_AudioFormat fmt, uint16_t chunk_size)
{
    /* Allocate space for project and set name */
    Project *proj = malloc(sizeof(Project));
    strcpy(proj->name, name);

    /* Set audio settings */
    proj->channels = channels;
    proj->sample_rate = sample_rate;
    proj->fmt = fmt;
    proj->chunk_size = chunk_size;

    /* Initialize play/record values */
    proj->play_speed = 0;
    proj->playing = false;
    proj->recording = false;
    proj->num_active_clips = 0;

    /* Initialize timeline struct */
    Timeline *tl = (Timeline *)malloc(sizeof(Timeline));
    tl->num_tracks = 0;
    tl->play_position = 0;
    tl->record_offset = 0;
    tl->play_offset = 0;
    tl->tempo = 120;
    tl->click_on = false;
    tl->sample_frames_per_pixel = DEFAULT_SFPP; // how "zoomed in" the timeline is. Affects display only
    tl->offset = 0; // horizontal offset of timeline in samples
    tl->v_offset = 0;
    tl->num_active_tracks = 0;
    memset(tl->active_track_indices, '\0', MAX_ACTIVE_TRACKS);
    proj->tl = tl;
    tl->proj = proj;


    /* Create SDL_Window and accompanying SDL_Renderer */
    char project_window_name[MAX_NAMELENGTH + 10];
    sprintf(project_window_name, "Jackdaw | %s", name);
    proj->jwin = create_jwin(project_window_name, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED-20, 900, 650);

    // tl->console_width = TRACK_CONSOLE_WIDTH;
    tl->rect = get_rect((SDL_Rect){0, 0, proj->jwin->w, proj->jwin->h,}, TL_RECT);
    int audio_rect_x = tl->rect.x + TRACK_CONSOLE_WIDTH + COLOR_BAR_W + PADDING;
    tl->audio_rect = (SDL_Rect) {audio_rect_x, tl->rect.y, proj->jwin->w - audio_rect_x, tl->rect.h}; // SET x in track

    return proj;
}

/* Create a new track, initialize some UI members */
Track *create_track(Timeline *tl, bool stereo)
{
    fprintf(stderr, "Enter create_track\n");
    Track *track = malloc(sizeof(Track));
    track->tl = tl;
    track->tl_rank = tl->num_tracks++;
    sprintf(track->name, "Track %d", track->tl_rank + 1);

    track->stereo = stereo;
    track->muted = false;
    track->solo = false;
    track->record = false;
    track->active = false;
    track->num_clips = 0;
    track->num_grabbed_clips = 0;
    track->rect.h = DEFAULT_TRACK_HEIGHT;
    track->rect.x = tl->rect.x + PADDING;

    if (track->tl_rank == 0) {
        track->rect.y = track->tl->rect.y + PADDING;
    } else {
        Track *last_track = track->tl->tracks[track->tl_rank - 1];
        track->rect.y = last_track->rect.y + last_track->rect.h + TRACK_SPACING;
    }
    track->rect.w = tl->proj->jwin->w - track->rect.x;


    (tl->tracks)[tl->num_tracks - 1] = track;
    trck_color_index++;
    trck_color_index %= 7;
    track->color = &(trck_colors[trck_color_index]);
    if (tl->proj && tl->proj->record_devices[0]) {
        fprintf(stderr, "Passed test\n");
        track->input = proj->record_devices[0];
    }


    track->vol_ctrl = malloc(sizeof(FSlider));
    track->vol_ctrl->max = 2;
    track->vol_ctrl->min = 0;
    track->vol_ctrl->value = 1;
    track->vol_ctrl->type = FILL;
    // track->vol_ctrl->orientation_vertical = false;
    track->pan_ctrl = malloc(sizeof(FSlider));
    track->pan_ctrl->max = 1;
    track->pan_ctrl->min = -1;
    track->pan_ctrl->value = 0;
    track->pan_ctrl->type = LINE;

    track->name_box = create_textbox(
        NAMEBOX_W * TRACK_CONSOLE_WIDTH / 100,
        // NAMEBOX_W * proj->tl->console_width / 100, 
        0, 
        2, 
        proj->jwin->bold_fonts[2],
        track->name,
        &black,
        NULL,
        NULL,
        rename_track,
        track,
        NULL,
        NULL,
        0,
        true
    );
    track->input_label_box = create_textbox(
        0, 
        0, 
        2, 
        proj->jwin->bold_fonts[1],
        "Input:",
        NULL,
        &clear,
        &clear,
        NULL,
        NULL,
        NULL,
        NULL,
        0,
        true
    );
    track->input_name_box = create_textbox(
        //TRACK_IN_W * proj->tl->console_width / 100, 
        TRACK_IN_W * TRACK_CONSOLE_WIDTH / 100,
        0, 
        4, 
        proj->jwin->fonts[1],
        (char *) track->input->name,
        NULL,
        NULL,
        NULL,
        select_track_input,
        track,
        NULL,
        NULL,
        5 * scale_factor,
        track->input->open
    );
    track->vol_label_box = create_textbox(
        0, 
        0, 
        2, 
        proj->jwin->bold_fonts[1],
        "Vol:",
        NULL,
        &clear,
        &clear,
        NULL,
        NULL,
        NULL,
        NULL,
        0,
        true
    );
    track->pan_label_box = create_textbox(
        0, 
        0, 
        2, 
        proj->jwin->bold_fonts[1],
        "Pan:",
        NULL,
        &clear,
        &clear,
        NULL,
        NULL,
        NULL,
        NULL,
        0,
        true
    );
    /* TEMPORARY */
    reset_track_internal_rects(track);
    /* END TEMPORARY */

    fprintf(stderr, "\t->exit create track\n");
    return track;
}

void reset_track_internal_rects(Track *track)
{
    track->console_rect = get_rect(track->rect, TRACK_CONSOLE_RECT);
    track->name_row_rect = get_rect(track->console_rect, TRACK_NAME_ROW);
    track->input_row_rect = get_rect(track->console_rect, TRACK_IN_ROW);
    track->vol_row_rect = get_rect(track->console_rect, TRACK_VOL_ROW);
    track->pan_row_rect = get_rect(track->console_rect, TRACK_PAN_ROW);
    position_textbox(track->name_box, track->name_row_rect.x, track->name_row_rect.y);
    position_textbox(track->input_label_box, track->input_row_rect.x, track->input_row_rect.y);
    position_textbox(track->input_name_box, track->input_row_rect.x + track->input_label_box->container.w + PADDING, track->input_row_rect.y);
    position_textbox(track->vol_label_box, track->vol_row_rect.x, track->vol_row_rect.y);
    position_textbox(track->pan_label_box, track->pan_row_rect.x, track->pan_row_rect.y);
    SDL_Rect volslider_rect = (SDL_Rect) {track->vol_label_box->container.x + track->vol_label_box->container.w + PADDING, track->vol_label_box->container.y + PADDING, track->vol_row_rect.w - track->vol_label_box->container.w - (PADDING * 4), track->vol_row_rect.h - (PADDING * 2)};
    set_fslider_rect(track->vol_ctrl, &volslider_rect, 4);
    reset_fslider(track->vol_ctrl); 

    SDL_Rect panslider_rect = (SDL_Rect) {track->pan_label_box->container.x + track->pan_label_box->container.w + PADDING, track->pan_label_box->container.y + PADDING, track->pan_row_rect.w - track->pan_label_box->container.w - (PADDING * 4), track->pan_row_rect.h - (PADDING * 2)};
    set_fslider_rect(track->pan_ctrl, &panslider_rect, 4);
    reset_fslider(track->pan_ctrl);
}

/* Reset the clip's container rect for redrawing */
void reset_cliprect(Clip *clip) 
{
    if (!(clip->track)) {
        fprintf(stderr, "Fatal Error: need track to create clip. \n"); //TODO allow loose clip
        exit(1);
    }
    clip->rect.x = get_tl_draw_x(clip->absolute_position);
    clip->rect.y = clip->track->rect.y + 4;
    clip->rect.w = get_tl_draw_w(clip->length);
    clip->rect.h = clip->track->rect.h - 8;
}

/* Create a new clip on a specified track. Clips are usually created empty and filled after recording is done. */
Clip *create_clip(Track *track, uint32_t length, uint32_t absolute_position) {
    if (!track) {
        fprintf(stderr, "Fatal Error: need track to create clip. \n"); //TODO allow loose clip
        exit(1);
    }
    Clip *clip = malloc(sizeof(Clip));
    if (!clip) {
        fprintf(stderr, "Error: unable to allocate space for clip\n");
    }
    clip->input = track->input;
    sprintf(clip->name, "%s, take %d", track->name, track->num_clips + 1);

    clip->length = length;
    clip->absolute_position = absolute_position;
    clip->track = track;
    reset_cliprect(clip);
    // clip->rect.x = get_tl_draw_x(clip->absolute_position);
    // clip->rect.y = track->rect.y + 4; //TODO: fix this
    // clip->rect.w = 0;
    // clip->rect.h = track->rect.h - 8;
    fprintf(stderr, "In create clip, absolute position is %d", clip->absolute_position);
    // clip->samples = malloc(sizeof(int16_t) * length);
    if (track) {
        clip->track_rank = track->num_clips;
        track->clips[track->num_clips] = clip;
        track->num_clips++;
        // clip->track = track;
    }
    clip->done = false;
    clip->grabbed = false;
    fprintf(stderr, "\t->exit create_clip\n");
    return clip;

}

void destroy_clip(Clip *clip)
{
    fprintf(stderr, "Enter destroy clip.\n");
    if (clip->pre_proc) {
        free(clip->pre_proc);
        clip->pre_proc = NULL;
    }
    if (clip->post_proc) {
        free(clip->post_proc);
        clip->post_proc = NULL;
    }
    free(clip);
    clip = NULL;
    fprintf(stderr, "\t->done destroy clip.\n");

}

void destroy_track(Track *track)
{
    fprintf(stderr, "Enter destroy track. Destroy @ %p\n", track);

    /* Destroy track clips */
    while (track->num_clips > 0) {
        if ((track->clips)[track->num_clips - 1]) {
            destroy_clip(*(track->clips + track->num_clips - 1));
        }
        track->num_clips--;
    }

    if (track->active) {
        track->tl->num_active_tracks--;
    }
    track->tl->tracks[track->tl->num_tracks - 1] = NULL;
    (track->tl->num_tracks)--;

    destroy_textbox(track->input_label_box);
    destroy_textbox(track->input_name_box);
    destroy_textbox(track->vol_label_box);
    destroy_textbox(track->pan_label_box);
    free(track->vol_ctrl);
    free(track->pan_ctrl);
    free(track);

    fprintf(stderr, "\t->Exit destroy track\n");
}


void grab_clip(Clip *clip) 
{
    clip->grabbed = true;
    clip->track->num_grabbed_clips++;
}

void ungrab_clip(Clip *clip) 
{
    clip->grabbed = false;
    clip->track->num_grabbed_clips--;
}

void grab_ungrab_clip(Clip *clip) {
    if (clip->grabbed) {
        ungrab_clip(clip);
    } else {
        grab_clip(clip);
    }
}

void grab_clips()
{
    for (uint8_t i=0; i<proj->tl->num_active_tracks; i++) {
        Track *track = proj->tl->tracks[proj->tl->active_track_indices[i]];
        for (uint8_t j=0; j<track->num_clips; j++) {
            Clip *clip = track->clips[j];
            if (
                clip->done 
                && proj->tl->play_position >= clip->absolute_position 
                && proj->tl->play_position <= clip->absolute_position + clip->length
            ) {
                grab_clip(clip);
            }
        }
    }
}

void ungrab_clips()
{
    fprintf(stderr, "Ungrab clips\n");
    for (uint8_t i=0; i<proj->tl->num_tracks; i++) {
        Track *track = proj->tl->tracks[i];
        fprintf(stderr, "\t->track %p\n", track);
        for (uint8_t j=0; j<track->num_clips; j++) {
            Clip *clip = track->clips[j];
            if (clip->grabbed) {
                clip->grabbed = false;
            }
        }
        track->num_grabbed_clips = 0;
    } 
    fprintf(stderr, "\t->Exit ungrab clips\n");

}

void reset_tl_rect(Timeline *tl)
{
    tl->rect = get_rect((SDL_Rect){0, 0, proj->jwin->w, proj->jwin->h,}, TL_RECT);
    int audio_rect_x = tl->rect.x + TRACK_CONSOLE_WIDTH + COLOR_BAR_W + PADDING;
    tl->audio_rect = (SDL_Rect) {audio_rect_x, tl->rect.y, tl->proj->jwin->w - audio_rect_x, tl->rect.h}; // SET x in track
    Track *track = NULL;

    for (int t=0; t<tl->num_tracks; t++) {
        track = tl->tracks[t];
        track->rect.w = tl->rect.w;
    }

}

void activate_or_deactivate_track(uint8_t track_index)
{
    fprintf(stderr, "Call to activate or deactivate track. Num active: %d\n", proj->tl->num_active_tracks);
    if (!proj) {
        fprintf(stderr, "Error: no project!\n");
        return;
    }
    Track *track = NULL;
    if ((track = proj->tl->tracks[track_index])) {
        if (track->active) {
            uint8_t i=0;
            bool reposition = false;
            while (i < proj->tl->num_active_tracks - 1) {
                if (track_index == proj->tl->active_track_indices[i]) {
                    reposition = true;
                }
                if (reposition) {
                    proj->tl->active_track_indices[i] = proj->tl->active_track_indices[i + 1];
                }
                i++;
            }
            proj->tl->num_active_tracks--;
            track->active = false;
        } else {
            if (proj->tl->num_active_tracks < MAX_ACTIVE_TRACKS) {
                proj->tl->active_track_indices[proj->tl->num_active_tracks] = track_index;
                proj->tl->num_active_tracks++;
                track->active = true;
            } else {
                fprintf(stderr, "Reached maximum number of active tracks (%d)\n", MAX_ACTIVE_TRACKS);
                return;
            }
        }
    } else {
        fprintf(stderr, "Error: attempting to activate/deactivate null track at index %d.\n", track_index);
    }
}


/* Move grabbed clips forward or back on timeline */
void translate_grabbed_clips(int32_t translate_by)
{
    if (!proj) {
        fprintf(stderr, "Error: request to translate clips with no project.\n");
        return;
    }
    for (uint8_t i=0; i<proj->tl->num_tracks; i++) {
        Track *track = proj->tl->tracks[i];
        for (uint8_t j=0; j<track->num_clips; j++) {
            Clip *clip = track->clips[j];
            if (clip->grabbed) {
                clip->absolute_position += translate_by;
                reset_cliprect(clip);
            }
        }
    }
}

/* Get count of currently grabbed clips */
uint8_t proj_grabbed_clips()
{
    uint8_t ret=0;
    for (uint8_t i=0; i<proj->tl->num_tracks; i++) {
        ret += proj->tl->tracks[i]->num_grabbed_clips;
        fprintf(stderr, "Trackm %d has %d grabbed clips\n", i, proj->tl->tracks[i]->num_grabbed_clips);
    }
    return ret;
}

/* Remove a selected clip from a track, either to delete or move */
void remove_clip_from_track(Clip *clip)
{  
    if (clip->track) {
        if (clip->grabbed) {
            clip->track->num_grabbed_clips--;
        }
        for (uint8_t i=clip->track_rank + 1; i<clip->track->num_clips; i++) {
            clip->track->clips[i]->track_rank--;
            clip->track->clips[i-1] = clip->track->clips[i];
        }
        clip->track->num_clips--;
    }
}

void delete_clip(Clip *clip)
{
    remove_clip_from_track(clip);
    destroy_clip(clip);
    clip = NULL;
}

/* Delete all currently grabbed clips. */
void delete_grabbed_clips()
{
    if (!proj) {
        fprintf(stderr, "Error: request to delete clips with no project.\n");
        return;
    }
    fprintf(stderr, "Enter delete grabbed clips\n");
    for (uint8_t i=0; i<proj->tl->num_tracks; i++) {
        Track *track = proj->tl->tracks[i];
        for (uint8_t j=0; j<track->num_clips; j++) {
            Clip *clip = track->clips[j];
            if (clip->grabbed) {
                fprintf(stderr, "Deleting grabbed clip at track %d, clip %d\n", i, j);
                delete_clip(clip);
                j--;
            }
        }
    }
}

void reposition_clip(Clip *clip, uint32_t new_pos)
{
    clip->absolute_position = new_pos;
}

void add_active_clip(Clip *clip)
{
    fprintf(stderr, "Add active clip\n");
    if (!proj) {
        fprintf(stderr, "Error: no project!\n");
        return;
    }
    if (proj->num_active_clips < MAX_ACTIVE_CLIPS) {
        proj->active_clips[proj->num_active_clips] = clip;
        proj->num_active_clips++;
    } else {
        fprintf(stderr, "Error: reached maximum number of active clips: %d\n", MAX_ACTIVE_CLIPS);
    }
}

void clear_active_clips()
{
    proj->num_active_clips = 0;
    fprintf(stderr, "\t->Exit clear_active_clips\n");
}

void activate_audio_devices(Project *proj)
{
    proj->num_playback_devices = query_audio_devices(&(proj->playback_devices), 0);
    proj->num_record_devices = query_audio_devices(&(proj->record_devices), 1);
    AudioDevice *dev = proj->playback_devices[0];
    if (open_audio_device(dev, proj->channels) == 0) {
        fprintf(stderr, "Opened audio device: %s\n\tchannels: %d\n\tformat: %s\n", dev->name, dev->spec.channels, get_audio_fmt_str(dev->spec.format));

    } else {
        fprintf(stderr, "Error: failed to open device %s\n", dev->name);
    }

    // for (int i=0; i<proj->num_playback_devices; i++) {
    //     fprintf(stderr, "device index %d, pointer: %p\n", i, proj->playback_devices[i]);
    //     AudioDevice *dev = proj->playback_devices[i];
    //     if (open_audio_device(dev, proj->channels) == 0) {
    //         fprintf(stderr, "Opened audio device: %s\n\tchannels: %d\n\tformat: %s\n", dev->name, dev->spec.channels, get_audio_fmt_str(dev->spec.format));

    //     } else {
    //         fprintf(stderr, "Error: failed to open device %s\n", dev->name);
    //     }
    // }
    dev = proj->record_devices[0];
    if (open_audio_device(dev, proj->channels) == 0) { //TODO: stop forcing mono
        fprintf(stderr, "Opened record device: %s\n\tchannels: %u\n\tformat: %s\n", dev->name, dev->spec.channels, get_audio_fmt_str(dev->spec.format));

    } else {
        fprintf(stderr, "Error: failed to open record device %s\n", dev->name);
    }
    // for (int i=0; i<proj->num_record_devices; i++) {
    //     AudioDevice *dev = proj->record_devices[i];
    //     if (open_audio_device(dev, proj->channels) == 0) { //TODO: stop forcing mono
    //         fprintf(stderr, "Opened record device: %s\n\tchannels: %u\n\tformat: %s\n", dev->name, dev->spec.channels, get_audio_fmt_str(dev->spec.format));

    //     } else {
    //         fprintf(stderr, "Error: failed to open record device %s\n", dev->name);
    //     }
    // }
    proj->playback_device = proj->playback_devices[0];

}

/* Returns true if change was made */
bool adjust_track_vol(Track *track, float change_by)
{
    return adjust_fslider(track->vol_ctrl, change_by);
}

/* Returns true if change was made */
bool adjust_track_pan(Track *track, float change_by)
{
    return adjust_fslider(track->pan_ctrl, change_by);
}

// typedef struct project {
//     char name[MAX_NAMELENGTH];
//     // bool dark_mode;
//     Timeline *tl;
//     Clip *loose_clips[100];
//     uint8_t num_loose_clips;
//     JDAWWindow *jwin;
//     Clip *active_clips[MAX_ACTIVE_CLIPS];
//     uint8_t num_active_clips;
//     float play_speed;
//     bool playing;
//     bool recording;
//     AudioDevice **record_devices;
//     AudioDevice **playback_devices;
//     int num_record_devices;
//     int num_playback_devices;

//     /* Audio settings */
//     AudioDevice *playback_device;
//     uint8_t channels;
//     int sample_rate; //samples per second
//     SDL_AudioFormat fmt;
//     uint16_t chunk_size; //sample_frames

// } Project;
/* Debug segaults */
void log_project_state(FILE *f) {
    if (!proj) {
        fprintf(f, "Project is null. Exiting.\n");
        return;
    }
    fprintf(f, "\n\n======== Project: %p ========\n\n", proj);
    fprintf(f, "AUDIO DEVICES\n");
    fprintf(f, "\tRecord devices (%d):\n", proj->num_record_devices);
    for (uint8_t i=0; i<proj->num_record_devices; i++) {
        AudioDevice *dev = proj->record_devices[i];
        fprintf(f, "\t\tDevice at %p:\n", dev);
        fprintf(f, "\t\t\tName: %s:\n", dev->name);
        fprintf(f, "\t\t\tId: %d:\n", dev->id);
        fprintf(f, "\t\t\tOpen: %d:\n", dev->open);
        fprintf(f, "\t\t\tChannels: %d:\n", dev->spec.channels);
        fprintf(f, "\t\t\tWrite buffpos: %d:\n", dev->write_buffpos);
    }
    fprintf(f, "\tPlayback devices (%d):\n", proj->num_playback_devices);
    for (uint8_t i=0; i<proj->num_playback_devices; i++) {
        AudioDevice *dev = proj->playback_devices[i];
        fprintf(f, "\t\tDevice at %p:\n", dev);
        fprintf(f, "\t\t\tName: %s:\n", dev->name);
        fprintf(f, "\t\t\tId: %d:\n", dev->id);
        fprintf(f, "\t\t\tOpen: %d:\n", dev->open);
        fprintf(f, "\t\t\tChannels: %d:\n", dev->spec.channels);
        fprintf(f, "\t\t\tWrite buffpos: %d:\n", dev->write_buffpos);
    }
    fprintf(f, "TRACKS (%d):\n", proj->tl->num_tracks);
    Clip *grabbed_clip = NULL;
    for (uint8_t i=0; i<proj->tl->num_tracks; i++) {
        Track *track = proj->tl->tracks[i];
        fprintf(f, "\tTrack at %p\n", track);
        fprintf(f, "\t\tName: %s\n", track->name);
        fprintf(f, "\t\tActive: %d\n", track->active);
        fprintf(f, "\t\tInput: %s\n", track->input->name);
        fprintf(f, "\t\tVol: %f\n", track->vol_ctrl->value);
        fprintf(f, "\t\tPan: %f\n", track->pan_ctrl->value);
        fprintf(f, "\t\tGrabbed clips: %d\n", track->num_grabbed_clips);
        if (track->num_grabbed_clips > 0) {
            for (uint8_t j=0; j<track->num_clips; j++) {
                if (track->clips[j]->grabbed) {
                    grabbed_clip = track->clips[j];
                    break;
                }
            }
        }
    }
    if (grabbed_clip) {
        fprintf(f, "GRABBED CLIP at %p. Samples in post_proc:\n\n", grabbed_clip);
        for (uint32_t i=0; i<grabbed_clip->length; i++) {
            fprintf(f, "%d ", grabbed_clip->post_proc[i]);
        }


    }


    fclose(f);
}