/**************************************************************************************************
 * Timeline, Track, Clip, and related functions.
 * Structs defined here contain all data that will be serialized and saved with a project.
 **************************************************************************************************/
#include <stdio.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/* Timeline- and clip-related constants */
#define MAX_TRACKS 25
#define MAX_NAMELENGTH 25

typedef struct track {
    char name[MAX_NAMELENGTH];
    bool stereo;
    bool muted;
    bool solo;
    bool record;
    uint8_t tl_rank;
    Clip *clips;
    uint16_t num_clips;
} Track;

typedef struct clip {
    char name[MAX_NAMELENGTH];
    int clip_gain;
    Track *track;
    uint32_t length; // in samples
    uint32_t absolute_position; // in samples
    int16_t *samples;
} Clip;

struct time_sig {
    uint8_t num;
    uint8_t denom;
    uint8_t beats;
    uint8_t subidv;
};

typedef struct timeline {
    uint32_t play_position; // in samples
    pthread_mutex_t play_position_lock;
    Track *tracks;
    uint8_t num_tracks;
    uint16_t tempo;
    bool click_on;
} Timeline;

bool in_clip(Track *track)
{

}

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

// int16_t get_tl_sample(Timeline* tl) // Do I want to advance the play head in this function?
// {
//     int16_t sample = 0;
//     // pthread_mutex_lock(&(tl->play_position_lock));

//     for (int i=0; i<tl->num_tracks; i++) {
//         sample += get_track_sample(tl->tracks + i, tl);
//     }
//     return sample;
//     // pthread_mutex_unlock(&(tl->play_position_lock));
// }

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


