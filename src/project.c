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
#include "dsp.h"
#include "gui.h"
#include "project.h"
#include "theme.h"
#include "audio.h"
#include "draw.h"
#include "timeline.h"
#include "transition.h"

#define DEFAULT_TRACK_HEIGHT (108 * scale_factor)
#define DEFAULT_SFPP 600 // sample frames per pixel

extern Project *proj;
extern uint8_t scale_factor;

extern JDAW_Color clear;
extern JDAW_Color white;
extern JDAW_Color lightergrey;
extern JDAW_Color black;
extern JDAW_Color grey;
extern JDAW_Color tl_bckgrnd;
extern JDAW_Color muted_bckgrnd;
extern JDAW_Color unmuted_bckgrnd;
extern JDAW_Color solo_bckgrnd;

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
    edit_textbox(track->name_box, draw_project, proj);
    track->name_box->bckgrnd_color = &lightergrey;
    track->name_box->show_cursor = false;
}

static void select_track_input(Textbox *tb, void *track_v)
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

static void select_track_input_menu(Textbox *tb, void *track_v)
{
    Track *track = (Track *)track_v;
    MenulistItem *device_mlitems[proj->num_record_devices];
    for (uint8_t i=0; i<proj->num_record_devices; i++) {
        device_mlitems[i] = malloc(sizeof(MenulistItem)); //TODO: Danger: memory leak
        strcpy(device_mlitems[i]->label, proj->record_devices[i]->name);
        // device_mlitems[i]->available = proj->record_devices[i]->open;
        device_mlitems[i]->available = true;
    }
    TextboxList *tbl = create_menulist(
        proj->jwin,
        0,
        7,
        proj->jwin->fonts[1],
        device_mlitems,
        proj->num_record_devices,
        select_track_input,
        track
    );
    position_textbox_list(tbl, track->input_name_box->container.x, track->input_name_box->container.y);
}


static void menu_activate_or_deactivate_track(Textbox *tb, void *track_v)
{
    Track *track = (Track *)track_v;
    activate_or_deactivate_track(track->tl_rank);
}

void delete_track(Track *track);
static void menu_delete_track(Textbox *tb, void *track_v)
{
    Track *track = (Track *)track_v;
    delete_track(track);
}
static void menu_grab_clip(Textbox *tb, void *clip_v)
{
    Clip *clip = (Clip *)clip_v;
    grab_ungrab_clip(clip);
}
static void menu_delete_clip(Textbox *tb, void *clip_v)
{
    Clip *clip = (Clip *)clip_v;
    delete_clip(clip);
}
static void menu_adjust_clip_start_ramp(Textbox *tb, void *clip_v)
{
    Clip *clip = (Clip *)clip_v;
    add_clip_transition(clip, true);
    
}
static void menu_adjust_clip_end_ramp(Textbox *tb, void *clip_v)
{
    Clip *clip = (Clip *)clip_v;
    add_clip_transition(clip, false);
    
}
void track_actions_menu(Track *track, SDL_Point *mouse_p)
{
    static const int num_track_actions = 2;
    static const int num_clip_actions = 4;
    int num_actions = num_track_actions;

    Clip *clip = NULL;
    for (uint8_t i=0; i<track->num_clips; i++) {
        clip = track->clips[i];
        if (SDL_PointInRect(mouse_p, &(clip->rect))) {
            break;
        }
        clip = NULL;
    }
    if (clip) {
        num_actions += num_clip_actions;
    }
    // Track *track = (Track *)track_v;
    MenulistItem *mlitems[num_actions];
    for (uint8_t i=0; i<num_actions; i++) {
        mlitems[i] = malloc(sizeof(MenulistItem));
        mlitems[i]->available = true;
        if (i < num_track_actions) {
            mlitems[i]->target = track;
        } else {
            mlitems[i]->target = clip;
        }
    }

    /* Action 0: Activate or deactivate track */
    mlitems[0]->onclick = menu_activate_or_deactivate_track;
    if (track->active) {
        strcpy(mlitems[0]->label, "Deactivate track");
    } else {
        strcpy(mlitems[0]->label, "Activate track");
    }

    /* Action 1: Delete track */
    mlitems[1]->onclick = menu_delete_track;
    strcpy(mlitems[1]->label, "Delete track (permanent)");

    if (clip) {

        mlitems[2]->onclick = menu_grab_clip;
        if (clip->grabbed) {

            strcpy(mlitems[2]->label, "Un-grab clip");
        } else {
            strcpy(mlitems[2]->label, "Grab clip");
        }
        mlitems[3]->onclick = menu_delete_clip;
        strcpy(mlitems[3]->label, "Delete clip (permanent)");
        mlitems[4]->onclick = menu_adjust_clip_start_ramp;
        strcpy(mlitems[4]->label, "Adjust clip start ramp");
        mlitems[5]->onclick = menu_adjust_clip_end_ramp;
        strcpy(mlitems[5]->label, "Adjust clip end ramp");
    }
    TextboxList *tbl = create_menulist(
        proj->jwin,
        0,
        7,
        proj->jwin->fonts[1],
        mlitems,
        num_actions,
        NULL,
        NULL
    );
    position_textbox_list(tbl, mouse_p->x, mouse_p->y);
}

void stop_playback();
void select_audio_out(Textbox *tb, void *proj_v)
{
    stop_playback();
    Project *proj = (Project *)proj_v;
    for (uint8_t i=0; i<proj->num_playback_devices; i++) {
        AudioDevice *pd = proj->playback_devices[i];
        if (strcmp(pd->name, tb->value) == 0) {
            proj->playback_device = pd;
            fprintf(stderr, "Resetting audio out to %s\n", pd->name);
            reset_textbox_value(proj->audio_out, (char *) pd->name);
            return;
        }
    }
}

static void select_audio_out_menu(Textbox *tb, void *proj_v)
{
    Project *proj = (Project *)proj_v;
    MenulistItem *device_mlitems[proj->num_playback_devices];
    for (uint8_t i=0; i<proj->num_playback_devices; i++) {
        device_mlitems[i] = malloc(sizeof(MenulistItem)); //TODO: Danger: memory leak
        strcpy(device_mlitems[i]->label, proj->playback_devices[i]->name);
        // device_mlitems[i]->available = proj->playback_devices[i]->open;
        device_mlitems[i]->available = true;
    }
    TextboxList *tbl = create_menulist(
        proj->jwin,
        0,
        7,
        proj->jwin->fonts[1],
        device_mlitems,
        proj->num_playback_devices,
        select_audio_out,
        proj
    );
    position_textbox_list(tbl, proj->audio_out->container.x, proj->audio_out->container.y);
}


/* An active project is required for jackdaw to run. This function creates a project based on various specifications,
and initalizes a variety of UI "globals." */
Project *create_project(const char* name, uint8_t channels, uint32_t sample_rate, SDL_AudioFormat fmt, uint16_t chunk_size_sframes)
{
    /* Allocate space for project and set name */
    Project *proj = malloc(sizeof(Project));
    strcpy(proj->name, name);

    /* Set audio settings */
    proj->channels = channels;
    proj->sample_rate = sample_rate;
    proj->fmt = fmt;
    proj->chunk_size_sframes = chunk_size_sframes;

    /* Initialize play/record values */
    proj->play_speed = 0;
    proj->playing = false;
    proj->recording = false;
    proj->num_active_clips = 0;

    /* Initialize output */
    proj->output_len = chunk_size_sframes;
    proj->output_L = malloc(sizeof(float) * chunk_size_sframes);
    proj->output_R = malloc(sizeof(float) * chunk_size_sframes);
    memset(proj->output_L, '\0', sizeof(float) * chunk_size_sframes);
    memset(proj->output_R, '\0', sizeof(float) * chunk_size_sframes);


    /* Initialize timeline struct */
    Timeline *tl = (Timeline *)malloc(sizeof(Timeline));
    tl->num_tracks = 0;
    tl->play_pos_sframes = 0;
    tl->in_mark_sframes = 0;
    tl->out_mark_sframes = 0;
    // tl->record_offset_sframes = 0;
    // tl->play_offset = 0;
    // tl->tempo = 120;
    // tl->click_on = false;
    tl->sample_frames_per_pixel = DEFAULT_SFPP; // how "zoomed in" the timeline is. Affects display only
    tl->display_offset_sframes = 0; // horizontal offset of timeline in samples
    tl->display_v_offset = 0;
    tl->num_active_tracks = 0;
    tl->num_clipboard_clips = 0;
    tl->leftmost_clipboard_clip_pos = 0;
    memset(tl->active_track_indices, '\0', MAX_ACTIVE_TRACKS);
    proj->tl = tl;
    tl->proj = proj;


    /* Create SDL_Window and accompanying SDL_Renderer */
    char project_window_name[MAX_NAMELENGTH + 10];
    sprintf(project_window_name, "Jackdaw | %s", name);
    proj->jwin = create_jwin(project_window_name, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED-20, 900, 650);

    tl->timecode_tb = create_textbox(0, 0, 0, 0, proj->jwin->mono_fonts[3], "+00:00:00:00000", &white, NULL, &black, NULL, NULL, NULL, NULL, 0, true, false, CENTER);

    proj->audio_out = NULL;
    activate_audio_devices(proj);

    proj->audio_out_label = create_textbox(
        0, 
        0, 
        2,
        1, 
        proj->jwin->bold_fonts[1],
        "Default output device:  ",
        &white,
        &clear,
        &clear,
        NULL,
        NULL,
        NULL,
        NULL,
        0,
        true,
        false,
        BOTTOM_LEFT
    );
    proj->audio_out = create_textbox(
        //TRACK_IN_W * proj->tl->console_width / 100, 
        AUDIO_OUT_W,
        0, 
        4,
        3, 
        proj->jwin->fonts[1],
        (char *) (proj->playback_device->name),
        NULL,
        NULL,
        NULL,
        select_audio_out_menu,
        proj,
        NULL,
        NULL,
        5 * scale_factor,
        true,
        true,
        CENTER_LEFT
    );

    reset_tl_rects(proj);
    reset_ctrl_rects(proj);

    return proj;
}

static void click_mute_unmute_track(Textbox *tb, void *track_v);
static void click_solo_unsolo_track(Textbox *tb, void *track_v);
/* Create a new track, initialize some UI members */
Track *create_track(Timeline *tl, bool stereo)
{
    fprintf(stderr, "Enter create_track\n");
    Track *track = malloc(sizeof(Track));
    track->tl = tl;
    track->tl_rank = tl->num_tracks++;
    sprintf(track->name, "Track %d", track->tl_rank + 1);

    track->channels = 2; // TODO: allow mono tracks
    track->muted = false;
    track->solo = false;
    track->solo_muted = false;
    // track->record = false;
    track->active = false;
    track->num_clips = 0;
    track->num_grabbed_clips = 0;

    track->num_filters = 0;
	track->filter_freq_response = NULL;
	track->freq_response_len = DEFAULT_FILTER_LEN;
    track->overlap_buffer_L = NULL;
    track->overlap_buffer_R = NULL;
    track->overlap_len = DEFAULT_FILTER_LEN - 1;


    track->rect.h = DEFAULT_TRACK_HEIGHT;
    track->rect.x = tl->rect.x + PADDING;

    if (track->tl_rank == 0) {
        track->rect.y = track->tl->rect.y + RULER_HEIGHT;
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
        track->input = proj->record_devices[0];
    }

    //TODO: create_fslider() function (safer)
    track->vol_ctrl = malloc(sizeof(FSlider));
    track->vol_ctrl->max = 2;
    track->vol_ctrl->min = 0;
    track->vol_ctrl->value = 1;
    track->vol_ctrl->type = FILL;
    track->pan_ctrl = malloc(sizeof(FSlider));
    track->pan_ctrl->max = 1;
    track->pan_ctrl->min = -1;
    track->pan_ctrl->value = 0;
    track->pan_ctrl->type = LINE;

    track->name_box = create_textbox(
        TRACK_NAMEBOX_W,
        0, 
        3,
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
        true,
        true,
        BOTTOM_LEFT
    );
    track->input_label_box = create_textbox(
        0, 
        0, 
        2,
        1,
        proj->jwin->bold_fonts[1],
        "Input:  ",
        NULL,
        &clear,
        &clear,
        NULL,
        NULL,
        NULL,
        NULL,
        0,
        true,
        false,
        BOTTOM_LEFT
    );
    track->input_name_box = create_textbox(
        //TRACK_IN_W * proj->tl->console_width / 100, 
        TRACK_IN_W,
        0, 
        4,
        3, 
        proj->jwin->fonts[1],
        (char *) track->input->name,
        NULL,
        &tl_bckgrnd,
        NULL,
        select_track_input_menu,
        track,
        NULL,
        NULL,
        5 * scale_factor,
        true,
        true,
        CENTER_LEFT
    );
    track->vol_label_box = create_textbox(
        0, 
        0, 
        2,
        1, 
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
        true,
        false,
        BOTTOM_LEFT
    );
    track->pan_label_box = create_textbox(
        0, 
        0, 
        2,
        1, 
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
        true,
        false,
        BOTTOM_LEFT
    );

    track->mute_button_box = create_textbox(
        MUTE_SOLO_W,
        MUTE_SOLO_W,
        0,
        0,
        proj->jwin->bold_fonts[2],
        "M",
        NULL,
        &black,
        &unmuted_bckgrnd,
        click_mute_unmute_track,
        track,
        NULL,
        NULL,
        5 * scale_factor,
        true,
        false,
        CENTER
    );
    track->solo_button_box = create_textbox(
        MUTE_SOLO_W,
        MUTE_SOLO_W,
        0,
        0,
        proj->jwin->bold_fonts[2],
        "S",
        NULL,
        &black,
        &unmuted_bckgrnd,
        click_solo_unsolo_track,
        track,
        NULL,
        NULL,
        5 * scale_factor,
        true,
        false,
        CENTER
    );
    /* TEMPORARY */
    reset_track_internal_rects(track);
    /* END TEMPORARY */

    // fprintf(stderr, "\t->exit create track\n");
    return track;
}

void reset_ctrl_rects(Project *proj)
{
    proj->ctrl_rect = get_rect((SDL_Rect){0, 0, proj->jwin->w, proj->jwin->h}, CTRL_RECT);
    proj->ctrl_rect_col_a = get_rect(proj->ctrl_rect, CTRL_RECT_COL_A);
    proj->audio_out_row = get_rect(proj->ctrl_rect_col_a, AUDIO_OUT_ROW);
    position_textbox(proj->audio_out_label, proj->audio_out_row.x, proj->audio_out_row.y);
    position_textbox(proj->audio_out, proj->audio_out_row.x + proj->audio_out_label->container.w + PADDING, proj->audio_out_row.y);
}

void reset_tl_rects(Project *proj) 
{

    proj->tl->rect = get_rect((SDL_Rect){0, 0, proj->jwin->w, proj->jwin->h,}, TL_RECT);
    proj->tl->audio_rect = (SDL_Rect) {proj->tl->rect.x + TRACK_CONSOLE_W + COLOR_BAR_W + PADDING, proj->tl->rect.y, proj->tl->rect.w, proj->tl->rect.h}; // SET x in track
    proj->tl->rect = get_rect((SDL_Rect){0, 0, proj->jwin->w, proj->jwin->h,}, TL_RECT);
    proj->tl->ruler_tc_container_rect = get_rect(proj->tl->rect, RULER_TC_CONTAINER);

    proj->tl->ruler_rect = get_rect(proj->tl->ruler_tc_container_rect, RULER_RECT);

    proj->tl->tc_rect = get_rect(proj->tl->ruler_tc_container_rect, TC_RECT);
    Track *track = NULL;
    for (int t=0; t<proj->tl->num_tracks; t++) {
        track = proj->tl->tracks[t];
        track->rect.h = DEFAULT_TRACK_HEIGHT;
        track->rect.x = proj->tl->rect.x + PADDING;

        if (track->tl_rank == 0) {
            track->rect.y = track->tl->rect.y + RULER_HEIGHT;
        } else {
            Track *last_track = track->tl->tracks[track->tl_rank - 1];
            track->rect.y = last_track->rect.y + last_track->rect.h + TRACK_SPACING;
        }
        track->rect.w = proj->tl->audio_rect.w;
        reset_track_internal_rects(track);
    }
    position_textbox(proj->tl->timecode_tb, proj->tl->tc_rect.x, proj->tl->tc_rect.y);

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
    position_textbox(track->mute_button_box, track->name_row_rect.x + TRACK_NAMEBOX_W + PADDING, track->name_row_rect.y);
    position_textbox(track->solo_button_box, track->mute_button_box->container.x + track->mute_button_box->container.w + PADDING, track->name_row_rect.y);
    SDL_Rect volslider_rect = (SDL_Rect) {track->vol_label_box->container.x + track->vol_label_box->container.w + PADDING, track->vol_label_box->container.y + PADDING, track->vol_row_rect.w - track->vol_label_box->container.w - (PADDING * 4), track->vol_row_rect.h - (PADDING * 2)};
    set_fslider_rect(track->vol_ctrl, &volslider_rect, 2);
    reset_fslider(track->vol_ctrl); 

    SDL_Rect panslider_rect = (SDL_Rect) {track->pan_label_box->container.x + track->pan_label_box->container.w + PADDING, track->pan_label_box->container.y + PADDING, track->pan_row_rect.w - track->pan_label_box->container.w - (PADDING * 4), track->pan_row_rect.h - (PADDING * 2)};
    set_fslider_rect(track->pan_ctrl, &panslider_rect, 2);
    reset_fslider(track->pan_ctrl);
    for (uint8_t i=0; i<track->num_clips; i++) {
        reset_cliprect(track->clips[i]);
    }
}

/* Reset the clip's container rect for redrawing */
void reset_cliprect(Clip *clip) 
{
    if (!(clip->track)) {
        fprintf(stderr, "Fatal Error: need track to create clip. \n"); //TODO allow loose clip
        exit(1);
    }
    // fprintf(stderr, "Clip Rect: %d, %d, %d, %d\n", clip->rect.x, clip->rect.y, clip->rect.w, clip->rect.h);
    clip->rect.x = get_tl_draw_x(clip->abs_pos_sframes);
    clip->rect.y = clip->track->rect.y + 4;

    clip->rect.w = get_tl_draw_w(clip->len_sframes);
    clip->rect.h = clip->track->rect.h - 8;

    if (clip->namebox) {
        position_textbox(clip->namebox, clip->rect.x + CLIP_NAME_PADDING_X, clip->rect.y + CLIP_NAME_PADDING_Y);
    }

}

/* Create a new clip on a specified track. Clips are usually created empty and filled after recording is done. */
Clip *create_clip(Track *track, uint32_t len_sframes, uint32_t absolute_position) {
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

    clip->len_sframes = len_sframes;
    clip->abs_pos_sframes = absolute_position;
    if (track) {
        clip->track = track;
        clip->channels = track->channels;
        clip->track_rank = track->num_clips;
        track->clips[track->num_clips] = clip;
        track->num_clips++;
        // clip->track = track;
    }
    if (len_sframes != 0) {
        clip->L = malloc(sizeof(float) * len_sframes);
        if (clip->channels == 2) {
            clip->R = malloc(sizeof(float) * len_sframes);
        }
    } else {
        clip->L = NULL;
        clip->R = NULL;
    }
    clip->done = false;
    clip->grabbed = false;
    clip->changed_track = false;
    clip->start_ramp_len = 0;
    clip->end_ramp_len = 0;

    clip->namebox = create_textbox(0, 0, 2, 2, proj->jwin->bold_fonts[1], clip->name, &grey, &clear, &clear, NULL, NULL, NULL, NULL, 0, true, true, BOTTOM_LEFT);
    reset_cliprect(clip);
    // fprintf(stderr, "\t->exit create_clip\n");
    return clip;

}

void destroy_clip(Clip *clip)
{
    // fprintf(stderr, "Enter destroy clip.\n");
    if (clip->L) {
        free(clip->L);
        clip->L = NULL;
    }
    if (clip->R) {
        free(clip->R);
        clip->R = NULL;
    }
    free(clip);
    clip = NULL;
    // fprintf(stderr, "\t->done destroy clip.\n");

}

void create_clip_buffers(Clip *clip, uint32_t buf_len_sframes)
{
    if (clip->L != NULL) {
        fprintf(stderr, "Error: clip '%s' already has a buffer allocated.\n", clip->name);
        return;
    }
    uint32_t buf_len_bytes = sizeof(float) * buf_len_sframes;
    clip->L = malloc(buf_len_bytes);
    if (clip->channels == 2) {
        clip->R = malloc(buf_len_bytes);
    }
}

void delete_track(Track *track)
{
    // fprintf(stderr, "Enter delete track. Index: %d\n", track->tl_rank);
    if (track->active) {
        activate_or_deactivate_track(track->tl_rank);
    }
    int index = track->tl_rank;
    Track *track_iter = NULL;
    for (uint8_t i=0; i<proj->tl->num_tracks; i++) {
        track_iter = proj->tl->tracks[i];
        if (i > index) {
            proj->tl->tracks[i - 1] = track_iter;
            track_iter->tl_rank--;
            track_iter->rect.y -= track->rect.h + PADDING;
            reset_track_internal_rects(track_iter);
        }
    }
    for (uint8_t i=0; i<proj->tl->num_active_tracks; i++) {
        int active_track_index = proj->tl->active_track_indices[i];
        if (active_track_index > index) {
            active_track_index--;
        }
    }

    destroy_track(track);
    // fprintf(stderr, "\t->exit delete track\n");
}

void destroy_track(Track *track)
{
    // fprintf(stderr, "Enter destroy track. Destroy @ %p\n", track);

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

    for (uint8_t i=0; i<track->num_filters; i++) {
        destroy_filter(track->filters[i]);
    }

    destroy_textbox(track->input_label_box);
    destroy_textbox(track->input_name_box);
    destroy_textbox(track->vol_label_box);
    destroy_textbox(track->pan_label_box);
    free(track->vol_ctrl);
    free(track->pan_ctrl);
    free(track);

    // fprintf(stderr, "\t->Exit destroy track\n");
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
                && proj->tl->play_pos_sframes >= clip->abs_pos_sframes 
                && proj->tl->play_pos_sframes <= clip->abs_pos_sframes + clip->len_sframes
            ) {
                grab_clip(clip);
            }
        }
    }
}

void ungrab_clips()
{
    // fprintf(stderr, "Ungrab clips\n");
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
    // fprintf(stderr, "\t->Exit ungrab clips\n");

}

void reset_tl_rect(Timeline *tl)
{
    tl->rect = get_rect((SDL_Rect){0, 0, proj->jwin->w, proj->jwin->h,}, TL_RECT);
    int audio_rect_x = tl->rect.x + TRACK_CONSOLE_W + COLOR_BAR_W + PADDING;
    tl->audio_rect = (SDL_Rect) {audio_rect_x, tl->rect.y, tl->proj->jwin->w - audio_rect_x, tl->rect.h}; // SET x in track
    Track *track = NULL;

    for (int t=0; t<tl->num_tracks; t++) {
        track = tl->tracks[t];
        track->rect.w = tl->rect.w;
    }

}

void activate_or_deactivate_track(uint8_t track_index)
{
    if (!proj) {
        fprintf(stderr, "Error: no project!\n");
        return;
    }
    Track *track = NULL;
    if (track_index < proj->tl->num_tracks && (track = proj->tl->tracks[track_index])) {
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

static void deactivate_all_tracks()
{
    if (!proj) {
        fprintf(stderr, "Error: call to deactivate all tracks without an active project.\n");
        return;
    }
    Track *track = NULL;
    for (uint8_t i=0; i<proj->tl->num_active_tracks; i++) {
        track = proj->tl->tracks[proj->tl->active_track_indices[i]];
        track->active = false;
    }
    proj->tl->num_active_tracks = 0;

}

void activate_deactivate_all_tracks()
{
    if (!proj) {
        fprintf(stderr, "Error: call to activate all tracks without an active project.\n");
        return;
    }
    Track *track = NULL;
    bool some_activated = false;
    for (uint8_t i=0; i<proj->tl->num_tracks; i++) {
        track = proj->tl->tracks[i];
        if (!(track->active)) {
            activate_or_deactivate_track(i);
            some_activated = true;
        }
    }
    if (!some_activated) {
        deactivate_all_tracks();
    }
}

void mute_track(Track *track)
{
    track->muted = true;
    track->mute_button_box->bckgrnd_color = &muted_bckgrnd;
}

static void unmute_track(Track *track)
{
    track->muted = false;
    track->mute_button_box->bckgrnd_color = &unmuted_bckgrnd;
}

void solo_track(Track *track)
{
    track->solo = true;
    track->solo_muted = false;
    track->solo_button_box->bckgrnd_color = &solo_bckgrnd;
    if (track->muted) {
        unmute_track(track);
    }
}

void solo_mute_track(Track *track)
{
    track->solo = false;
    track->solo_muted = true;
    track->solo_button_box->bckgrnd_color = &muted_bckgrnd;

}

static void unsolo_track(Track *track)
{
    track->solo = false;
    track->solo_muted = false;
    track->solo_button_box->bckgrnd_color = &unmuted_bckgrnd;
}

static void click_mute_unmute_track(Textbox *tb, void *track_v)
{

    Track *track = (Track *)track_v;
    if (track->muted) {
        unmute_track(track);
    } else {
        mute_track(track);
    }
}
static void click_solo_unsolo_track(Textbox *tb, void *track_v)
{
    Track *track = (Track *)track_v;
    bool some_solo = false;
    if (!(track->solo)) {
        unmute_track(track);
        solo_track(track);
        some_solo = true;
    } else {
        unsolo_track(track);
        for (uint8_t i=0; i<proj->tl->num_tracks; i++) {
            if (proj->tl->tracks[i]->solo) {
                some_solo = true;
                break;
            }
        }
    }
    if (some_solo) {
        for (uint8_t i=0; i<proj->tl->num_tracks; i++) {
            Track *track = proj->tl->tracks[i];
            if (!(track->solo)) {
                solo_mute_track(track);
            }
        }
    } else {
        for (uint8_t i=0; i<proj->tl->num_tracks; i++) {
            Track *track = proj->tl->tracks[i];
            if (!(track->solo)) {
                unsolo_track(track);
            }
        } 
    }
}

void mute_unmute()
{
    if (!proj) {
        fprintf(stderr, "Error: request to mute/unmute with no active project.\n");
        return;
    }
    Track *track = NULL;
    bool all_muted = true;
    for (uint8_t i=0; i<proj->tl->num_active_tracks; i++) {
        track = proj->tl->tracks[proj->tl->active_track_indices[i]];
        if (!(track->muted)) {
            mute_track(track);
            all_muted = false;
        }
    }
    if (all_muted) {
        for (uint8_t i=0; i<proj->tl->num_active_tracks; i++) {
            track = proj->tl->tracks[proj->tl->active_track_indices[i]];
            unmute_track(track);
        }
    }
}

void solo_unsolo()
{
    if (!proj) {
        fprintf(stderr, "Error: request to solo/unsolo with no active project.\n");
        return;
    }
    Track *track = NULL;
    bool all_active_soloed = true;
    bool some_inactive_soloed = false;
    for (uint8_t i=0; i<proj->tl->num_tracks; i++) {
        track = proj->tl->tracks[i];
        if (!(track->solo)) {
            if (track->active) {
                solo_track(track);
                all_active_soloed = false;
            } else {
                solo_mute_track(track);
            }
        } else if (!(track->active)) {
            some_inactive_soloed = true;
        }
    }
    if (some_inactive_soloed && all_active_soloed) {
        for (uint8_t i=0; i<proj->tl->num_active_tracks; i++) {
            track = proj->tl->tracks[proj->tl->active_track_indices[i]];
            solo_mute_track(track);
        }
    } else if (all_active_soloed) {
        for (uint8_t i=0; i<proj->tl->num_tracks; i++) {
            track = proj->tl->tracks[i];
            unsolo_track(track);
        }
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
                clip->abs_pos_sframes += translate_by;
                reset_cliprect(clip);
            }
        }
    }
}

/* Get count of currently grabbed clips */
uint8_t num_grabbed_clips()
{
    uint8_t ret=0;
    for (uint8_t i=0; i<proj->tl->num_tracks; i++) {
        ret += proj->tl->tracks[i]->num_grabbed_clips;
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

void add_clip_to_track(Clip *clip, Track *track)
{
    // fprintf(stderr, "Adding clip %p to track %p.\n", clip, track);
    track->clips[track->num_clips] = clip;
    clip->track_rank = track->num_clips;
    track->num_clips++;
    clip->track = track;
    if (clip->grabbed) {
        track->num_grabbed_clips++;
    }
    // fprintf(stderr, "\t->done adding clip to track\n");

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
    // fprintf(stderr, "Enter delete grabbed clips\n");
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

void reposition_clip(Clip *clip, int32_t new_pos)
{
    clip->abs_pos_sframes = new_pos;
}

void add_active_clip(Clip *clip)
{
    // fprintf(stderr, "Add active clip\n");
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
}


void copy_clips_to_clipboard()
{
    proj->tl->num_clipboard_clips = 0;
    int32_t leftmost_pos = 0;
    for (uint8_t i=0; i<proj->tl->num_tracks; i++) {
        Track *track = proj->tl->tracks[i];
        bool first_clip = true;
        for (uint8_t j=0; j<track->num_clips; j++) {
            Clip *clip = track->clips[j];
            if (clip->grabbed && proj->tl->num_clipboard_clips < MAX_CLIPBOARD_CLIPS) {
                proj->tl->clip_clipboard[proj->tl->num_clipboard_clips] = clip;
                proj->tl->num_clipboard_clips++;
                if (first_clip || clip->abs_pos_sframes < leftmost_pos) {
                    leftmost_pos = clip->abs_pos_sframes;
                    first_clip = false;
                }
            }
        }
    }
    proj->tl->leftmost_clipboard_clip_pos = leftmost_pos;
}


static Clip *copy_clip(Clip *clip_to_copy)
{
    Clip *new_clip = create_clip(clip_to_copy->track, clip_to_copy->len_sframes, clip_to_copy->abs_pos_sframes);
    // create_clip_buffers(new_clip, clip_to_copy->len_sframes);
    uint32_t buf_len_bytes = clip_to_copy->len_sframes * sizeof(float);
    // new_clip->L = malloc(buf_len_bytes);
    // if (new_clip->channels == 2) {
    //     new_clip->R = malloc(buf_len_bytes);
    // }
    // new_clip->pre_proc = malloc(buf_len_bytes);
    // new_clip->post_proc = malloc(buf_len_bytes);
    memcpy(new_clip->L, clip_to_copy->L, buf_len_bytes);
    if (new_clip->channels == 2) {
        memcpy(new_clip->R, clip_to_copy->R, buf_len_bytes);
    }
    return new_clip;
}

void paste_clips()
{
    int32_t pos_offset = proj->tl->play_pos_sframes - proj->tl->leftmost_clipboard_clip_pos;
    for (uint8_t i=0; i<proj->tl->num_clipboard_clips; i++) {
        Clip *clip_to_copy = proj->tl->clip_clipboard[i];
        Clip *new_clip = copy_clip(clip_to_copy);
        new_clip->abs_pos_sframes += pos_offset;
        new_clip->done = true;
        reset_cliprect(new_clip);
    }

}

void destroy_audio_devices(Project *proj)
{
    AudioDevice *dev = NULL;
    for (uint8_t i=0; i<proj->num_record_devices; i++) {
        dev = proj->record_devices[i];
        if (dev->open) {
            close_audio_device(dev);
        }
        destroy_audio_device(dev);
    }
    for (uint8_t i=0; i<proj->num_playback_devices; i++) {
        dev = proj->playback_devices[i];
        if (dev->open) {
            close_audio_device(dev);
        }
        destroy_audio_device(dev);
    }
    free(proj->record_devices);
    free(proj->playback_devices);
    proj->record_devices = NULL;
    proj->playback_devices = NULL;

}

void activate_audio_devices(Project *proj)
{
    fprintf(stderr, "ACTIVATE AUDIO DEVICES. Channels: %d\n", proj->channels);
    proj->num_playback_devices = query_audio_devices(&(proj->playback_devices), 0);
    proj->num_record_devices = query_audio_devices(&(proj->record_devices), 1);
    AudioDevice *dev = proj->playback_devices[0];
    if (open_audio_device(proj, dev, proj->channels) == 0) {
        fprintf(stderr, "Opened audio device: %s\n\tchannels: %d\n\tformat: %s\n", dev->name, dev->spec.channels, get_audio_fmt_str(dev->spec.format));

    } else {
        fprintf(stderr, "Error: failed to open device %s\n", dev->name);
    }

    for (int i=1; i<proj->num_playback_devices; i++) {
        fprintf(stderr, "device index %d, pointer: %p\n", i, proj->playback_devices[i]);
        AudioDevice *dev = proj->playback_devices[i];
        if (open_audio_device(proj, dev, proj->channels) == 0) {
            fprintf(stderr, "Opened audio device: %s\n\tchannels: %d\n\tformat: %s\n", dev->name, dev->spec.channels, get_audio_fmt_str(dev->spec.format));

        } else {
            fprintf(stderr, "Error: failed to open device %s\n", dev->name);
        }
    }
    // dev = proj->record_devices[0];
    // if (open_audio_device(dev, proj->channels) == 0) { //TODO: stop forcing mono
    //     fprintf(stderr, "Opened record device: %s\n\tchannels: %u\n\tformat: %s\n", dev->name, dev->spec.channels, get_audio_fmt_str(dev->spec.format));

    // } else {
    //     fprintf(stderr, "Error: failed to open record device %s\n", dev->name);
    // }

    for (int i=0; i<proj->num_record_devices; i++) {
        AudioDevice *dev = proj->record_devices[i];
        dev->open = true;
    }
    proj->playback_device = proj->playback_devices[0];
    fprintf(stderr, "CHECKING AUDIO_OUT\n");
    if (proj->audio_out) {
        fprintf(stderr, "OK, it was found.\n");
        reset_textbox_value(proj->audio_out, (char *)proj->playback_device->name);
    }
    fprintf(stderr, "Not found\n");

    for (uint8_t i=0; i<proj->tl->num_tracks; i++) {
        Track *track = proj->tl->tracks[i];
        track->input = proj->record_devices[0];
        reset_textbox_value(track->input_name_box, (char *)track->input->name);
    }
}

static void cut_clip(Clip *clip, int32_t offset_sframes)
{
    Clip *new_clip = create_clip(clip->track, clip->len_sframes - offset_sframes, clip->abs_pos_sframes + offset_sframes);
    // new_clip->len_sframes = clip->len_sframes - offset_sframes;
    // uint32_t new_buff_bytelen = new_clip->len_sframes * sizeof(double);

    // new_clip->L = malloc(sizeof(double) * );
    // new_clip->post_proc = malloc(new_buff_bytelen);
    uint32_t new_clip_buf_len_bytes = (clip->len_sframes - offset_sframes) * sizeof(float);
    memcpy(new_clip->L, clip->L + offset_sframes, new_clip_buf_len_bytes);
    if (new_clip->channels == 2) {
        memcpy(new_clip->R, clip->R + offset_sframes, new_clip_buf_len_bytes);
    }
    // memcpy(new_clip->pre_proc, clip->pre_proc + offset_sframes * clip->channels, new_clip->len_sframes * clip->channels * sizeof(int16_t));
    clip->len_sframes = offset_sframes;
    uint32_t old_clip_new_buf_len_bytes = clip->len_sframes * sizeof(float);
    clip->L = realloc(clip->L, old_clip_new_buf_len_bytes);
    if (clip->channels == 2) {
        clip->R = realloc(clip->R, old_clip_new_buf_len_bytes);
    }
    new_clip->end_ramp_len = clip->end_ramp_len;
    clip->end_ramp_len = 0;
    new_clip->done = true;
    reset_cliprect(clip);
}

void cut_clips()
{
    Track *track = NULL;
    Clip *clip = NULL;
    for (uint8_t i=0; i<proj->tl->num_active_tracks; i++) {
        track = proj->tl->tracks[proj->tl->active_track_indices[i]];
        for (uint8_t j=0; j<track->num_clips; j++) {
            clip = track->clips[j];
            int32_t offset_sframes = proj->tl->play_pos_sframes - clip->abs_pos_sframes;
            if (clip->done && offset_sframes > 0 && offset_sframes < clip->len_sframes) {
                cut_clip(clip, offset_sframes);
            }
        }
    }
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

void add_filter_to_track(Track *track, FilterType type, uint16_t impulse_response_len) 
{
    FIRFilter *filter = create_FIR_filter(type, impulse_response_len, proj->chunk_size_sframes * 2);
    track->filters[track->num_filters] = filter;
    track->num_filters++;
}

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
        fprintf(f, "\t\t\tFormat: %s:\n", get_audio_fmt_str(dev->spec.format));
        fprintf(f, "\t\t\tWrite buffpos: %d:\n", dev->write_buffpos_samples);
    }
    fprintf(f, "\tPlayback devices (%d):\n", proj->num_playback_devices);
    for (uint8_t i=0; i<proj->num_playback_devices; i++) {
        AudioDevice *dev = proj->playback_devices[i];
        fprintf(f, "\t\tDevice at %p:\n", dev);
        fprintf(f, "\t\t\tName: %s:\n", dev->name);
        fprintf(f, "\t\t\tId: %d:\n", dev->id);
        fprintf(f, "\t\t\tOpen: %d:\n", dev->open);
        fprintf(f, "\t\t\tChannels: %d:\n", dev->spec.channels);
        fprintf(f, "\t\t\tFormat: %s:\n", get_audio_fmt_str(dev->spec.format));
        fprintf(f, "\t\t\tWrite buffpos: %d:\n", dev->write_buffpos_samples);
    }
    fprintf(f, "TRACKS (%d):\n", proj->tl->num_tracks);
    Clip *grabbed_clip = NULL;
    for (uint8_t i=0; i<proj->tl->num_tracks; i++) {
        Track *track = proj->tl->tracks[i];
        fprintf(f, "\tTrack at %p\n", track);
        fprintf(f, "\t\tName: %s\n", track->name);
        fprintf(f, "\t\tActive: %d\n", track->active);
        fprintf(f, "\t\tInput: %s\n", track->input->name);
        fprintf(f, "\t\tMuted: %d\n", track->muted);
        fprintf(f, "\t\tSolo: %d\n", track->solo);
        fprintf(f, "\t\tSolo muted: %d\n", track->solo_muted);

        fprintf(f, "\t\tVol: %f\n", track->vol_ctrl->value);
        fprintf(f, "\t\tPan: %f\n", track->pan_ctrl->value);
        fprintf(f, "\t\tClips: %d\n", track->num_clips);
        fprintf(f, "\t\tGrabbed clips: %d\n", track->num_grabbed_clips);
        if (track->num_clips > 0) {
            fprintf(f, "\t\tCLIPS:\n");
            for (uint8_t j=0; j<track->num_clips; j++) {
                Clip *clip = track->clips[j];
                fprintf(f, "\t\t\tClip at %p\n", clip);
                fprintf(f, "\t\t\t\tName: %s\n", clip->name);
                fprintf(f, "\t\t\t\tLen sframes: %d\n", clip->len_sframes);
                fprintf(f, "\t\t\t\tAbs pos: %d\n", clip->abs_pos_sframes);



                fprintf(f, "\t\t\t\tGrabbed: %d\n", clip->grabbed);
                fprintf(f, "\t\t\t\tStart ramp len: %d\n", clip->start_ramp_len);
                fprintf(f, "\t\t\t\tEnd ramp len: %d\n", clip->end_ramp_len);

            }
        }
    }


    fclose(f);
}