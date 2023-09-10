#ifndef JDAW_PROJECT_H
#define JDAW_PROJECT_H

#include <stdio.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "SDL2/SDL.h"

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
    Clip *clips[100];
    uint16_t num_clips;
    int height;
} Track;

typedef struct clip {
    char name[MAX_NAMELENGTH];
    int clip_gain;
    Track *track;
    uint32_t length; // in samples
    uint32_t absolute_position; // in samples
    int16_t *samples;
    bool done;
} Clip;

typedef struct timeline {
    uint32_t play_position; // in samples
    pthread_mutex_t play_position_lock;
    Track *tracks[MAX_TRACKS];
    uint8_t num_tracks;
    uint16_t tempo;
    bool click_on;
} Timeline;

typedef struct project {
    char name[MAX_NAMELENGTH];
    bool dark_mode;
    Timeline *tl;
    Clip *loose_clips[100];
    uint8_t num_loose_clips;
    SDL_Window *win;
    SDL_Renderer *rend;
    Clip *active_clip;
    float play_speed;
    bool playing;
    bool recording;
} Project;

int16_t get_track_sample(Track *track, Timeline *tl, int pos_in_chunk);
int16_t *get_mixdown_chunk(Timeline* tl, int length);
Project *create_project(const char* name, bool dark_mode);
Track *create_track(Timeline *tl, bool stereo);
Clip *create_clip(Track *track, uint32_t length, uint32_t absolute_position);
void destroy_clip(Clip *clip);
void destroy_track(Track *track);


#endif