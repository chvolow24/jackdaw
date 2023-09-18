/**************************************************************************************************
 * Jackdaw | a stripped-down, keyboard-focused Digital Audio Workstation. Built on SDL.
***************************************************************************************************

  Copyright (C) 2023 Charlie Volow

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

**************************************************************************************************/

/**************************************************************************************************
 * Create and destroy Project, Timeline, Track, and Clip objects
 * Get audio mixdown chunks for playback or saving
 * Save a project to a .jdaw file
 **************************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "project.h"
#include "gui.h"
#include "theme.h"

#define DEFAULT_TRACK_HEIGHT (100 * proj->scale_factor)
#define DEFAULT_SFPP 300 // sample frames per pixel

extern Project *proj;

/* Alternating bright colors to easily distinguish tracks */
JDAW_Color trck_colors[6] = {
    {{210, 18, 18, 255}, {210, 18, 18, 255}},
    {{226, 151, 0, 255}, {226, 151, 0, 255}},
    {{15, 228, 0, 255}, {15, 228, 0, 255}},
    {{0, 226, 219, 255}, {0, 226, 219, 255}},
    {{0, 98, 226, 255}, {0, 98, 226, 255}},
    {{128, 0, 226, 255}, {128, 0, 226, 255}},
};

int trck_color_index = 0;


/* Temporary test functions, for event handling */
void do_thing(Textbox* tb)
{
    fprintf(stderr, "DONE THING: %s\n", tb->value);
}

void do_thing_2(Textbox* tb)
{
    fprintf(stderr, "DONE THING TWO: %s\n", tb->value);

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
void reset_winrect(Project *proj) {
    SDL_GL_GetDrawableSize(proj->win, &((proj->winrect).w), &((proj->winrect).h));
}

/* An active project is required for jackdaw to run. This function creates a project based on various specifications,
and initalizes a variety of UI "globals." */
Project *create_project(const char* name, bool dark_mode)
{
    Project *proj = malloc(sizeof(Project));
    strcpy(proj->name, name);
    proj->dark_mode = dark_mode;
    proj->play_speed = 0;
    Timeline *tl = (Timeline *)malloc(sizeof(Timeline));
    tl->num_tracks = 0;
    tl->play_position = 0;
    tl->tempo = 120;
    tl->click_on = false;
    tl->sample_frames_per_pixel = DEFAULT_SFPP;
    tl->offset = 0;
    proj->tl = tl;
    proj->playing = false;
    proj->recording = false;

    /* Create SDL_Window and accompanying SDL_Renderer */
    char project_window_name[MAX_NAMELENGTH + 10];
    sprintf(project_window_name, "Jackdaw | %s", name);
    proj->win = NULL;
    proj->win = SDL_CreateWindow(
        project_window_name,
        SDL_WINDOWPOS_CENTERED, 
        SDL_WINDOWPOS_CENTERED - 20, 
        900, 
        650, 
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI
    );

    if (!(proj->win)) {
        fprintf(stderr, "Error creating window: %s", SDL_GetError());
        exit(1);
    }

    Uint32 render_flags = SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC;
    proj->rend = SDL_CreateRenderer(proj->win, -1, render_flags);
    if (!(proj->rend)) {
        fprintf(stderr, "Error creating renderer: %s\n", SDL_GetError());
        exit(1);
    }
    // SDL_RenderSetScale(proj->rend,2,2);
    proj->winrect = (SDL_Rect) {0, 0, 0, 0};
    reset_winrect(proj);

    /* Set DPI Scale Factor */
    int rw = 0, rh = 0, ww = 0, wh = 0;
    SDL_GetWindowSize(proj->win, &ww, &wh);
    SDL_GetRendererOutputSize(proj->rend, &rw, &rh);
    proj->scale_factor = (float)rw / (float)ww;
    if (proj->scale_factor != (float)rh / (float)wh) {
        fprintf(stderr, "Error! Scale factor w != h.\n");
    }
    // tl->rect = get_rect(proj->winrect, TL_RECT);
    return proj;
}

/* Create a new track, initialize some UI members */
Track *create_track(Timeline *tl, bool stereo)
{
    fprintf(stderr, "Enter create_track\n");
    Track *track = malloc(sizeof(Track));
    track->tl = tl;
    track->tl_rank = (tl->num_tracks)++;
    sprintf(track->name, "Track %d", track->tl_rank + 1);
    track->stereo = stereo;
    track->muted = false;
    track->solo = false;
    track->record = false;
    track->num_clips = 0;
    track->rect.h = DEFAULT_TRACK_HEIGHT;
    track->rect.x = proj->tl->rect.x + PADDING;
    track->rect.y = proj->tl->rect.y + PADDING + track->tl_rank * (track->rect.h + TRACK_SPACING);
    track->rect.w = proj->winrect.w - track->rect.x;

    (tl->tracks)[tl->num_tracks - 1] = track;

    trck_color_index++;
    trck_color_index %= 5;
    track->color = &(trck_colors[trck_color_index]);
    if (proj && proj->record_devices[0]) {
        track->input = proj->record_devices[0];
    }

    track->name_box = create_textbox(
        NAMEBOX_W, 
        0, 
        2, 
        proj->bold_fonts[1],
        track->name,
        NULL,
        NULL,
        NULL,
        do_thing_2,
        do_thing_2,
        NULL
    );
    fprintf(stderr, "\t->exit create track\n");
    return track;
}

/* Create a new clip on a specified track. Clips are usually created empty and filled after recording is done. */
Clip *create_clip(Track *track, uint32_t length, uint32_t absolute_position) {
    fprintf(stderr, "Enter create_clip\n");

    Clip *clip = malloc(sizeof(Clip));
    sprintf(clip->name, "Track %d, take %d", track->tl_rank + 1, track->num_clips + 1);
    clip->length = length;
    clip->absolute_position = absolute_position;
    // clip->samples = malloc(sizeof(int16_t) * length);
    if (track) {
        track->num_clips += 1;
        track->clips[track->num_clips - 1] = clip;
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

    (track->tl->num_tracks)--;

    /* Free track */
    free(track);
    track = NULL;

    fprintf(stderr, "Exit destroy track\n");
}

void reset_tl_rect(Timeline *tl)
{
    tl->rect = get_rect(proj->winrect, TL_RECT);
}