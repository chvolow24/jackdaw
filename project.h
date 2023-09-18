/*****************************************************************************************************************
    Jackdaw | a stripped-down, keyboard-focused Digital Audio Workstation | built on SDL (https://libsdl.org/)
******************************************************************************************************************

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

*****************************************************************************************************************/

#ifndef JDAW_PROJECT_H
#define JDAW_PROJECT_H

#include <stdio.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "SDL.h"
#include "SDL_ttf.h"
#include "theme.h"

/* Timeline- and clip-related constants */
#define MAX_TRACKS 25
#define MAX_NAMELENGTH 25
#define TL_RECT (Dim) {ABS, 5}, (Dim) {REL, 20}, (Dim) {REL, 100}, (Dim) {REL, 76}

typedef struct clip Clip;
typedef struct timeline Timeline;
typedef struct audiodevice AudioDevice;
typedef struct track Track;
typedef struct textbox Textbox;

typedef struct track {
    char name[MAX_NAMELENGTH];
    bool active;
    bool stereo;
    bool muted;
    bool solo;
    bool record;
    Timeline *tl;
    uint8_t tl_rank;
    Clip *clips[100];
    uint16_t num_clips;
    AudioDevice *input;

    /* 
    GUI members
    Positions and dimensions are cached to avoid re-calculation on every re-draw. Repositioning and resizing are
    handled in their own functions
     */
    JDAW_Color *color;
    SDL_Rect rect;
    Textbox *name_box;
    Textbox *input_label_box;
    Textbox *input_name_box;


} Track;

typedef struct clip {
    char name[MAX_NAMELENGTH];
    int clip_gain;
    Track *track;
    uint32_t length; // in samples
    uint32_t absolute_position; // in samples
    int16_t *samples;
    bool done;

    /* GUI members */
    SDL_Rect rect;
    void (*onclick)(void);
    void (*onhover)(void);
    Textbox *namebox;
    Textbox *input_label;
    Textbox *input;
} Clip;

typedef struct timeline {
    uint32_t play_position; // in sample frames
    uint32_t record_position; // in sample frames
    uint32_t in_mark;
    uint32_t out_mark;
    pthread_mutex_t play_position_lock;
    Track *tracks[MAX_TRACKS];
    uint8_t num_tracks;
    uint16_t tempo;
    bool click_on;

    /* GUI members */
    SDL_Rect rect;
    SDL_Rect audio_rect;
    uint32_t offset; // in samples frames
    // uint32_t sample_frames_shown; // determines timeline scale
    double sample_frames_per_pixel;
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
    AudioDevice **record_devices;
    AudioDevice **playback_devices;
    int num_record_devices;
    int num_playback_devices;

    /* UI "Globals" */
    uint8_t scale_factor;
    SDL_Rect winrect;
    TTF_Font *fonts[11];
    TTF_Font *bold_fonts[11];

} Project;

int16_t get_track_sample(Track *track, Timeline *tl, uint32_t start_pos, uint32_t pos_in_chunk);
int16_t *get_mixdown_chunk(Timeline* tl, uint32_t length, bool from_mark_in);
Project *create_project(const char* name, bool dark_mode);
Track *create_track(Timeline *tl, bool stereo);
Clip *create_clip(Track *track, uint32_t length, uint32_t absolute_position);
void destroy_clip(Clip *clip);
void destroy_track(Track *track);
void reset_winrect(Project *proj);
void reset_tl_rect(Timeline *tl);


#endif