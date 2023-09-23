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

#define DEFAULT_TRACK_HEIGHT (100 * scale_factor)
#define DEFAULT_SFPP 300 // sample frames per pixel

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

void rename_track(void *track_v)
{
    Track *track = (Track *)track_v;
    track->name_box->bckgrnd_color = &white;
    track->name_box->show_cursor = true;
    track->name_box->cursor_countdown = CURSOR_COUNTDOWN;
    track->name_box->cursor_pos = strlen(track->name);
    edit_textbox(track->name_box);
    // strcpy(track->name, newname); 
    // memcpy(track->name, newname, strlen(newname)); // TODO: wtf is strcpy producing a trace trap

    track->name_box->bckgrnd_color = &lightergrey;
    track->name_box->show_cursor = false;
}

void select_track_input(void *track_v)
{
    Track *track = (Track *)track_v;

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
            sample += (clip->samples)[pos_in_clip];
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
Project *create_project(const char* name)
{
    Project *proj = malloc(sizeof(Project));
    strcpy(proj->name, name);
    proj->play_speed = 0;
    Timeline *tl = (Timeline *)malloc(sizeof(Timeline));
    tl->num_tracks = 0;
    tl->play_position = 0;
    tl->tempo = 120;
    tl->click_on = false;
    tl->sample_frames_per_pixel = DEFAULT_SFPP;
    tl->offset = 0;
    tl->v_offset = 0;
    proj->tl = tl;
    proj->playing = false;
    proj->recording = false;
    /* Create SDL_Window and accompanying SDL_Renderer */
    char project_window_name[MAX_NAMELENGTH + 10];
    sprintf(project_window_name, "Jackdaw | %s", name);
    proj->jwin = create_jwin(project_window_name, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED-20, 900, 650);

    tl->console_width = TRACK_CONSOLE_WIDTH;
    tl->rect = get_rect((SDL_Rect){0, 0, proj->jwin->w, proj->jwin->h,}, TL_RECT);    
    fprintf(stderr, "win w, h: %d, %d; TL: %d, %d, %d, %d", proj->jwin->w, proj->jwin->h, tl->rect.x, tl->rect.y, tl->rect.w, tl->rect.h);
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
    track->num_clips = 0;
    track->rect.h = DEFAULT_TRACK_HEIGHT;
    track->rect.x = proj->tl->rect.x + PADDING;
    Track *last_track = track->tl->tracks[track->tl_rank - 1];
    if (track->tl_rank == 0) {
        track->rect.y = proj->tl->rect.y + PADDING;
    } else {
        track->rect.y = last_track->rect.y + last_track->rect.h + TRACK_SPACING;
    }
    track->rect.w = proj->jwin->w - track->rect.x;
    (tl->tracks)[tl->num_tracks - 1] = track;

    trck_color_index++;
    trck_color_index %= 7;
    track->color = &(trck_colors[trck_color_index]);
    if (proj && proj->record_devices[0]) {
        track->input = proj->record_devices[0];
    }

    track->name_box = create_textbox(
        NAMEBOX_W * proj->tl->console_width / 100, 
        0, 
        2, 
        proj->jwin->bold_fonts[2],
        track->name,
        &black,
        NULL,
        NULL,
        rename_track,
        NULL,
        NULL,
        0
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
        0
    );
    track->input_name_box = create_textbox(
        TRACK_IN_W * proj->tl->console_width / 100, 
        0, 
        4, 
        proj->jwin->bold_fonts[1],
        (char *) track->input->name,
        NULL,
        NULL,
        NULL,
        select_track_input,
        NULL,
        NULL,
        5 * scale_factor
    );


    //TODO: get this out of create track
    // tl->audio_rect = (SDL_Rect) {tl->rect.x + TRACK_CONSOLE_WIDTH + COLOR_BAR_W, tl->rect.y, tl->rect.w, tl->rect.h};
    fprintf(stderr, "\t->exit create track\n");
    return track;
}

/* Create a new clip on a specified track. Clips are usually created empty and filled after recording is done. */
Clip *create_clip(Track *track, uint32_t length, uint32_t absolute_position) {
    fprintf(stderr, "Enter create_clip\n");

    Clip *clip = malloc(sizeof(Clip));
    sprintf(clip->name, "%s, take %d", track->name, track->num_clips + 1);
    clip->length = length;
    clip->absolute_position = absolute_position;
    // clip->samples = malloc(sizeof(int16_t) * length);
    if (track) {
        track->num_clips += 1;
        track->clips[track->num_clips - 1] = clip;
        clip->track = track;
    }
    clip->done = false;
    return clip;
    fprintf(stderr, "\t->exit create_clip\n");

}

void destroy_clip(Clip *clip)
{
    if (clip->samples) {
        free(clip->samples);
        clip->samples = NULL;
    }
    free(clip);
    clip = NULL;
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
    // TODO: Reorder tracks when a track that is not the last track is deleted.

    // /* Save rank for timeline renumbering */
    // int rank = track->tl_rank;

    // /* Renumber subsequent tracks in timeline if not last track */
    // while (rank <= track->tl->num_tracks) {
    //     Track* t;
    //     if ((t = *(track->tl->tracks + rank))) {
    //         t->tl_rank--;
    //     } else {
    //         fprintf(stderr, "Error: track expected at rank %d but not found.\n", rank);
    //     }
    //     rank++;
    // }

    track->tl->tracks[track->tl->num_tracks - 1] = NULL;
    (track->tl->num_tracks)--;

    /* Free track */
    free(track);

    fprintf(stderr, "Exit destroy track\n");
}

void reset_tl_rect(Timeline *tl)
{
    tl->rect = get_rect((SDL_Rect){0, 0, proj->jwin->w, proj->jwin->h,}, TL_RECT);
    Track *track = NULL;

    for (int t=0; t<tl->num_tracks; t++) {
        track = tl->tracks[t];
        track->rect.w = tl->rect.w;
    }

}