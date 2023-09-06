/**************************************************************************************************
 * Timeline, Track, Clip, and related functions.
 * Structs defined here contain all data that will be serialized and saved with a project.
 **************************************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "project.h"

int16_t get_track_sample(Track *track, Timeline *tl)
{
    if (track->muted) {
        return 0;
    }
    for (int i=0; track->num_clips; i++) {
        Clip *clip = track->clips + i;
        long int pos_in_clip = tl->play_position - clip->absolute_position;
        if (pos_in_clip >= 0 && pos_in_clip < clip->length) {
            return *(clip->samples + pos_in_clip);
        } else {
            return 0;
        }
    }
}

int16_t *get_mixdown_chunk(Timeline* tl, int length)
{
    int bytelen = sizeof(uint32_t) * length;
    int16_t *mixdown = malloc(bytelen);
    memset(mixdown, '\0', bytelen);
    if (!mixdown) {
        fprintf(stderr, "\nError: could not allocate mixdown chunk.");
        exit(1);
    }
    for (int i=0; i<length; i++) {
        for (int t=0; t<tl->num_tracks; t++) {
            mixdown[i] += get_track_sample(tl->tracks + i, tl);
        }
    }
    return mixdown;
}

Project *create_project(const char* name, bool dark_mode)
{
    Project *proj = malloc(sizeof(Project));
    strcpy(proj->name, name);
    proj->dark_mode = dark_mode;
    proj->loose_clips = NULL;
    Timeline *tl = (Timeline *) malloc(sizeof(Timeline));
    tl->num_tracks = 0;
    tl->tracks = NULL;
    tl->play_position = 0;
    tl->tempo = 120;
    tl->click_on = false;
    proj->tl = tl;
    return proj;
}

Track *create_track(Timeline *tl, bool stereo)
{
    Track *track = malloc(sizeof(Track));
    track->tl = tl;
    track->tl_rank = ++(tl->num_tracks);
    sprintf(track->name, "Track %d", track->tl_rank);
    track->stereo = stereo;
    track->muted = false;
    track->solo = false;
    track->record = false;
    track->clips = NULL;
    track->num_clips = 0;
    tl->tracks + tl->num_tracks = track;
    tl->num_tracks++;
    return track;
}

Clip *create_clip(Track *track, uint32_t length, uint32_t absolute_position) {
    track->num_clips += 1;
    Clip* clip = malloc(sizeof(Clip));
    sprintf(clip->name, "T%d-%d", track->tl_rank, track->num_clips);
    track->length = length;
    track->absolute_position = absolute_position;
    track->samples = malloc(sizeof(int16_t) * length);
    return clip;
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

    /* Destroy track clips */
    while (track->num_clips > 0) {
        if (track->clips + num_clips - 1) {
            destroy_clip(track->clips + num_clips - 1);
        }
        track->num_clips--;
    }

    /* Save rank for timeline renumbering */
    int rank = track->tl_rank;

    /* Free track */
    free(track);
    track = NULL;

    /* Renumber subsequent tracks in timeline if not last track */
    while (rank <= tl->num_tracks) {
        Track* t;
        if (t = tl->track + rank) {
            t->tl_rank--;
        } else {
            fprintf("\nError: track expected at rank %d but not found.", rank);
        }
        rank++;
    }
    tl->num_tracks--;
}