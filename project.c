/**************************************************************************************************
 * Timeline, Track, Clip, and related functions.
 * Structs defined here contain all data that will be serialized and saved with a project.
 **************************************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "project.h"
#include "gui.h"

#define STD_TRACK_HEIGHT 100
#define TL_RECT (Dim) {ABS, 180}, (Dim) {REL, 10}, (Dim) {REL, 100}, (Dim) {REL, 85}

extern Project *proj;

int16_t get_track_sample(Track *track, Timeline *tl, int pos_in_chunk)
{
    // fprintf(stderr, "Enter get_track_sample\n");

    if (track->muted) {
        return 0;
    }
    int16_t sample = 0;
    for (int i=0; i < track->num_clips; i++) {
        // fprintf(stderr, "\ti, num_clips: %d, %d\n", i, track->num_clips);
        Clip *clip = (track->clips)[i];
        if (!(clip->done)) {
            break;
        }
        long int pos_in_clip = tl->play_position + pos_in_chunk - clip->absolute_position;
        if (pos_in_clip >= 0 && pos_in_clip < clip->length) {
            // fprintf(stderr, "\tpos_in_clip, cliplen: %ld, %d\n", pos_in_clip, clip->length);
            sample += (clip->samples)[pos_in_clip];
        }
    }
    // fprintf(stderr, "\t->exit get_track_sample\n");

    return sample;
}

int16_t *get_mixdown_chunk(Timeline* tl, int length)
{
    fprintf(stderr, "Enter get_mixdown_chunk\n");
    int bytelen = sizeof(int16_t) * length;
    int16_t *mixdown = malloc(bytelen);
    memset(mixdown, '\0', bytelen);
    if (!mixdown) {
        fprintf(stderr, "\nError: could not allocate mixdown chunk.");
        exit(1);
    }

    int i=0;
    int j=0;
    while (i < length) {
        for (int t=0; t<tl->num_tracks; t++) {
            mixdown[i] += get_track_sample((tl->tracks)[t], tl, j);
        }
        j += proj->play_speed;
        i++;
    }
    // for (int i=0; i<length; i++) {
    //     for (int t=0; t<tl->num_tracks; t++) {
    //         mixdown[i] += get_track_sample((tl->tracks)[t], tl, i);
    //     }
    // }
    if (!(proj->recording)) {
        proj->tl->play_position += length * proj->play_speed;
    }
    fprintf(stderr, "\t->exit get_mixdown_chunk\n");

    return mixdown;
}

void reset_winrect(Project *proj) {
    SDL_GL_GetDrawableSize(proj->win, &((proj->winrect).w), &((proj->winrect).h));
}

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
    tl->rect = get_rect(proj->winrect, TL_RECT);
    fprintf(stderr, "tl rect: %d %d %d %d\n", tl->rect.x, tl->rect.y, tl->rect.w, tl->rect.h);
    // tl->rect = relative_rect(&(proj->winrect), 0.05, 0.1, 1, 0.86);
    // draw_rounded_rect(proj->rend, &tl_box, STD_RAD);
    return proj;
}

Track *create_track(Timeline *tl, bool stereo)
{
    fprintf(stderr, "Enter create_track\n");
    Track *track = malloc(sizeof(Track));
    track->tl = tl;
    track->tl_rank = (tl->num_tracks)++;
    sprintf(track->name, "Track %d", track->tl_rank);
    track->stereo = stereo;
    track->muted = false;
    track->solo = false;
    track->record = false;
    track->num_clips = 0;
    track->rect.h = STD_TRACK_HEIGHT * proj->scale_factor;
    (tl->tracks)[tl->num_tracks - 1] = track;
    fprintf(stderr, "\t->exit create track\n");
    return track;
}

Clip *create_clip(Track *track, uint32_t length, uint32_t absolute_position) {
    fprintf(stderr, "Enter create_clip\n");

    Clip *clip = malloc(sizeof(Clip));
    sprintf(clip->name, "T%d-%d", track->tl_rank, track->num_clips);
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