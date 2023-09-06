#ifndef JDAW_PROJECT_H
#define JDAW_PROJECT_H

#include <stdio.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/* Timeline- and clip-related constants */
#define MAX_TRACKS 25
#define MAX_NAMELENGTH 25

typedef struct clip Clip;
typedef struct timeline Timeline;

typedef struct track {
    char name[MAX_NAMELENGTH];
    bool stereo;
    bool muted;
    bool solo;
    bool record;
    Timeline *tl;
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

typedef struct timeline {
    uint32_t play_position; // in samples
    pthread_mutex_t play_position_lock;
    Track *tracks;
    uint8_t num_tracks;
    uint16_t tempo;
    bool click_on;
} Timeline;

typedef struct project {
    char name[MAX_NAMELENGTH];
    bool dark_mode;
    Timeline *tl;
    Clip *loose_clips;
    uint8_t num_loose_clips;
} Project;


#endif