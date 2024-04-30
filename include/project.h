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
    project.h

    * Define DAW-specific data structures
	* All structs can be accessed through a single Project struct
 *****************************************************************************************************************/



/*
	Type rules:
		* All sample or sframe (sample fram) elengths in uint32_t
		* All sample or sframe positions in int32_t
			-> Timeline includes negative positions
			-> In positive range at 96000 (maximum) sample rate, room for 372.827 minutes or 6.2138 hours
		* List lengths in smallest unsigned int type that will fit maximum lengths (usually uint8_t)
		* Draw coordinates are regular "int"s
*/

#ifndef JDAW_PROJECT_H
#define JDAW_PROJECT_H


#include <stdbool.h>
#include <stdint.h>

#include "textbox.h"
#include "slider.h"

#define MAX_TRACKS 255
#define MAX_NAMELENGTH 255
#define MAX_TRACK_CLIPS 255
#define MAX_ACTIVE_CLIPS 255
#define MAX_ACTIVE_TRACKS 255
#define MAX_CLIP_REFS 255
#define MAX_CLIPBOARD_CLIPS 255
#define MAX_PROJ_TIMELINES 255
#define MAX_PROJ_AUDIO_DEVICES 255
#define MAX_PROJ_CLIPS 512
#define MAX_GRABBED_CLIPS 255


typedef struct project Project;
typedef struct timeline Timeline;
typedef struct audio_device AudioDevice;
typedef struct clip Clip;
typedef struct clip_ref ClipRef;


typedef struct track {
    char name[MAX_NAMELENGTH];
    bool deleted;
    bool active;
    bool muted;
    bool solo;
    bool solo_muted;
    uint8_t channels;
    Timeline *tl; /* Parent timeline */
    uint8_t tl_rank;

    ClipRef *clips[MAX_TRACK_CLIPS];
    uint8_t num_clips;
    uint8_t num_grabbed_clips;

    uint8_t num_takes;

    AudioDevice *input;
    AudioDevice *output;

    float vol; /* 0.0 - 1.0 attenuation only */
    float pan; /* 0.0 pan left; 0.5 center; 1.0 pan right */
    FSlider *vol_ctrl;
    FSlider *pan_ctrl;
    
    /* FSLIDER *vol_ctrl */
    /* FSlider *pan_ctrl */

    Layout *layout;
    Textbox *tb_name;
    Textbox *tb_input_label;
    Textbox *tb_input_name;
    Textbox *tb_vol_label;
    Textbox *tb_pan_label;
    Textbox *tb_mute_button;
    Textbox *tb_solo_button;

    SDL_Rect *console_rect;
    SDL_Rect *colorbar;
    SDL_Color color;
    // SDL_Rect *vol_bar;
    // SDL_Rect *pan_bar;
    // SDL_Rect *in_bar;
} Track;

typedef struct clip_ref {
    char name[MAX_NAMELENGTH];
    bool deleted;
    int32_t pos_sframes;
    uint32_t in_mark_sframes;
    uint32_t out_mark_sframes;
    uint32_t start_ramp_len;
    uint32_t end_ramp_len;
    Clip *clip;
    Track *track;
    bool home;
    bool grabbed;

    SDL_Rect rect;

    SDL_mutex *lock;
} ClipRef;
    
typedef struct clip {
    char name[MAX_NAMELENGTH];
    bool deleted;
    uint8_t channels;
    uint32_t len_sframes;
    ClipRef *refs[MAX_CLIP_REFS];
    uint8_t num_refs;
    float *L;
    float *R;

    /* Recording in */
    Track *target;
    bool recording;
    AudioDevice *recorded_from;

    /* Xfade */
    uint32_t start_ramp_len_sframes;
    uint32_t end_ramp_len_sframes;

    /* TL navigation */
    /* bool grabbed; */
    /* bool changed_track; */
    
    
} Clip;

typedef struct timecode {
    uint8_t hours;
    uint8_t minutes;
    uint8_t seconds;
    uint8_t milliseconds;
    uint32_t frames;
    char str[16]; // e.g. "+00:00:00:96000"
} Timecode;

/* The project timeline organizes included tracks and specifies how they should be displayed */
typedef struct timeline {
    char name[MAX_NAMELENGTH];
    
    int32_t play_pos_sframes;
    int32_t in_mark_sframes;
    int32_t out_mark_sframes;
    int32_t record_from_sframes;
    
    Track *tracks[MAX_TRACKS];
    
    uint8_t num_tracks;
    // uint8_t active_track_indices[MAX_ACTIVE_TRACKS];
    // uint8_t num_active_tracks;

    uint8_t track_selector;    

    Timecode timecode;
    Textbox *timecode_tb;
    
    Project *proj;

    /* Clip *clip_clipboard[MAX_CLIPBOARD_CLIPS]; */
    /* uint8_t num_clipboard_clips; */
    /* int32_t leftmost_clipboard_clip_pos; */

    /* GUI members */
    Layout *layout;
    int32_t display_offset_sframes; // in samples frames
    int sample_frames_per_pixel;
    int display_v_offset;
    
} Timeline;



/* A Jackdaw project. Only one can be active at a time. Can persist on disk as a .jdaw file (see dot_jdaw.c, dot_jdaw.h) */
typedef struct project {
    char name[MAX_NAMELENGTH];
    Timeline *timelines[MAX_PROJ_TIMELINES];
    uint8_t num_timelines;
    uint8_t active_tl_index;
    
    float play_speed;
    bool recording;

    AudioDevice *record_devices[MAX_PROJ_AUDIO_DEVICES];
    uint8_t num_record_devices;
    AudioDevice *playback_devices[MAX_PROJ_AUDIO_DEVICES];
    uint8_t num_playback_devices;

    /* Audio settings */
    AudioDevice *playback_device;
    uint8_t channels;
    uint32_t sample_rate; //samples per second
    SDL_AudioFormat fmt;
    uint16_t chunk_size_sframes; //sample_frames

    /* Clips */
    Clip *clips[MAX_PROJ_CLIPS];
    uint16_t num_clips;
    uint16_t active_clip_index;

    /* Src */
    bool source_mode;
    Clip *src_clip;
    int32_t src_play_pos_sframes;
    int32_t src_in_sframes;
    int32_t src_out_sframes;
    float src_play_speed;

    /* Audio output */
    float *output_L;
    float *output_R;
    uint16_t output_len;


    /* In progress */
    /* Track *vol_changing; */
    bool vol_changing;
    bool vol_up;
    bool pan_changing;
    bool pan_right;

    /* GUI Members */
    Layout *layout;
    Textbox *tb_audio_out_label;
    Textbox *tb_audio_out_name;

    SDL_Rect *audio_rect;
    SDL_Rect *control_bar_rect;
    SDL_Rect *ruler_rect;
    SDL_Rect *source_rect;
    SDL_Rect *source_clip_rect;
    Textbox *source_name_tb;
} Project;

Project *project_create(
    char *name,
    uint8_t channels,
    uint32_t sample_rate,
    SDL_AudioFormat fmt,
    uint16_t chunk_size_sframes
    );

/* Return the index of a timeline to switch to (new one if success) */
uint8_t project_add_timeline(Project *proj, char *name);
void timeline_add_track(Timeline *tl);
void timeline_reset(Timeline *tl);
Clip *project_add_clip(AudioDevice *dev, Track *target);
ClipRef *track_create_clip_ref(Track *track, Clip *clip, int32_t record_from_sframes, bool home);
int32_t clip_ref_len(ClipRef *cr);
void clipref_reset(ClipRef *cr);

void track_increment_vol(Track *track);
void track_decrement_vol(Track *track);
void track_increment_pan(Track *track);
void track_decrement_pan(Track *track);

#endif
