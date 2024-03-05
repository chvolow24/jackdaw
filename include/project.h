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

#include <complex.h>
#include <stdio.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "SDL.h"
#include "SDL_ttf.h"
#include "theme.h"

/* Timeline- and clip-related constants */
#define MAX_TRACKS 100
#define MAX_NAMELENGTH 255
#define MAX_ACTIVE_CLIPS 25
#define MAX_ACTIVE_TRACKS 25
#define MAX_TRACK_CLIPS 255
#define MAX_TRACK_FILTERS 25
#define MAX_CLIPBOARD_CLIPS 25

typedef struct clip Clip;
typedef struct timeline Timeline;
typedef struct project Project;
typedef struct audiodevice AudioDevice;
typedef struct track Track;
typedef struct textbox Textbox;
typedef struct jdaw_window JDAWWindow;
typedef struct f_slider FSlider;
typedef struct fir_filter FIRFilter;

typedef enum filter_type {
    LOWPASS, HIGHPASS, BANDPASS, BANDCUT
} FilterType;

/* An audio track in the timeline. This object includes handles to various controls and all resident clips */
typedef struct track {
	char name[MAX_NAMELENGTH];
	bool active;
	bool muted;
	bool solo;
	bool solo_muted; // true if any other track is soloed
	// uint8_t channels; // Tracks can either be stereo or mono
	// bool record; // 
	uint8_t channels;
	Timeline *tl; // Parent timeline
	uint8_t tl_rank; // Index in timeline, e.g. 0 for track 1, 1 for track 2, etc.
	Clip *clips[MAX_TRACK_CLIPS];
	uint8_t num_clips;
	uint8_t num_grabbed_clips;
	AudioDevice *input;
	AudioDevice *output;
	FSlider *vol_ctrl;
	FSlider *pan_ctrl;

	/* DSP/effects */
	FIRFilter *filters[MAX_TRACK_FILTERS];
	uint8_t num_filters;
	double complex *filter_freq_response;
	uint16_t freq_response_len;
	double *overlap_buffer_L;
	double *overlap_buffer_R;
	uint16_t overlap_len;
	/* 
	GUI members
	Positions and dimensions are cached to avoid re-calculation on every re-draw. Repositioning and resizing are
	handled in their own functions
		*/
	JDAW_Color *color;

	Layout *layout;
	// SDL_Rect rect;
	// SDL_Rect console_rect;
	// SDL_Rect name_row_rect;
	// SDL_Rect input_row_rect;
	// SDL_Rect vol_row_rect;
	// SDL_Rect pan_row_rect;

	// Textbox *name_box;
	// Textbox *input_label_box;
	// Textbox *input_name_box;
	// Textbox *vol_label_box;
	// Textbox *pan_label_box;
	// Textbox *mute_button_box;
	// Textbox *solo_button_box;
	Text name_txt;
	Text input_label_txt;
	Text input_name_txt;
	Text vol_label_txt;
	Text pan_label_txt;
	Text mute_button_txt;
	Text solo_button_txt;

} Track;
3
/* A chunk of audio, associated with a particular track in the timeline */
typedef struct clip {
    char name[MAX_NAMELENGTH];
    Track *track; // parent track
    uint8_t track_rank; // index of clip in parent track
    uint8_t channels;
    // bool stereo;
    uint32_t len_sframes; // length in sample frames
    int32_t abs_pos_sframes; // timeline position in sample frames
    float *L; // Left channel audio data
    float *R; // Right channel audio data
    // int16_t *pre_proc; // the raw clip audio data
    // int16_t *post_proc; // cached audio data after audio processing
    bool done; // true when the clip has finished recording
    AudioDevice *input; // the device used to record the clip, if applicable
    uint32_t start_ramp_len; // length of linear fade in in sample frames
    uint32_t end_ramp_len; // length of linear fade out in sample frames

    bool grabbed;
    bool changed_track; // true if a mouse drag event is occurring, and the clip has already changed track

    /* GUI members */
    Layout *layout;
    Text name_txt;
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
    int32_t play_pos_sframes; // in sample frames
    // uint32_t record_position; // in sample frames
    // int32_t record_offset;
    // int32_t play_offset;
    int32_t in_mark_sframes;
    int32_t out_mark_sframes;
    // pthread_mutex_t play_position_lock;
    Track *tracks[MAX_TRACKS];
    uint8_t num_tracks;
    uint8_t active_track_indices[MAX_ACTIVE_TRACKS];
    uint8_t num_active_tracks;
    // uint16_t tempo;
    // bool click_on;
    Timecode timecode;
    Project *proj; // parent project

    Clip *clip_clipboard[MAX_CLIPBOARD_CLIPS];
    uint8_t num_clipboard_clips;
    int32_t leftmost_clipboard_clip_pos;

    /* GUI members */

    Layout *layout;
    
    int32_t display_offset_sframes; // in samples frames
    int sample_frames_per_pixel;
    int display_v_offset;
    Text timecode_txt;

} Timeline;

/* A Jackdaw project. Only one can be active at a time. Can persist on disk as a .jdaw file (see dot_jdaw.c, dot_jdaw.h) */
typedef struct project {
    char name[MAX_NAMELENGTH];
    // bool dark_mode;
    Timeline *tl;
    /* JDAWWindow *jwin; */
    Clip *active_clips[MAX_ACTIVE_CLIPS];
    uint8_t num_active_clips;
    float play_speed;
    bool playing;
    bool recording;
    AudioDevice **record_devices;
    uint8_t num_record_devices;
    AudioDevice **playback_devices;
    uint8_t num_playback_devices;

    /* Audio settings */
    AudioDevice *playback_device;
    uint8_t channels;
    uint32_t sample_rate; //samples per second
    SDL_AudioFormat fmt;
    uint16_t chunk_size_sframes; //sample_frames


    /* Audio output */
    float *output_L;
    float *output_R;
    uint16_t output_len;

    /* GUI Members */
    Window *win
    Layout *layout;
    Text audio_out_label_txt;
    Text audio_out_name_txt;

} Project;

/* Create a project from some parameters. Fmt should always be AUDIO_S16SYS */
Project *create_project(const char* name, uint8_t channels, uint32_t sample_rate, SDL_AudioFormat fmt, uint16_t chunk_size);
//TODO: lock in AUDIO_S16SYS format

/* Create track on an existing timeline */
Track *create_track(Timeline *tl, bool stereo);

/* Create a clip on an existing tracl */
Clip *create_clip(Track *track, uint32_t len_sframes, uint32_t absolute_position);

/* If the clip buffers do not exist, create them at appropriate length */
void create_clip_buffers(Clip *clip, uint32_t buf_len_sframes);


void destroy_clip(Clip *clip);
void destroy_track(Track *track);
void reset_tl_rect(Timeline *tl);

void activate_or_deactivate_track(uint8_t track_index);
// void deactivate_all_tracks(void);
void activate_deactivate_all_tracks(void);


void mute_track(Track *track);
void solo_track(Track *track);
void solo_mute_track(Track *track);
void mute_unmute(void);
void solo_unsolo(void);


void destroy_audio_devices(Project *proj);
void activate_audio_devices(Project *proj);
void track_actions_menu(Track *track, SDL_Point *mouse_p);
void reposition_clip(Clip *clip, int32_t new_pos);
void add_active_clip(Clip *clip);
void clear_active_clips();
void log_project_state(FILE *f);
void grab_clip(Clip *clip);
void grab_ungrab_clip(Clip *clip);
void grab_clips(void);
void ungrab_clips(void);
uint8_t num_grabbed_clips(void);
void cut_clips();
void translate_grabbed_clips(int32_t translate_by);
void remove_clip_from_track(Clip *clip);
void add_clip_to_track(Clip *clip, Track *track);
void delete_clip(Clip *clip);
void delete_grabbed_clips();
void reset_cliprect(Clip* clip);
void reset_track_internal_rects(Track *track);
void reset_tl_rects(Project *proj);
void reset_ctrl_rects(Project *proj);
bool adjust_track_vol(Track *track, float change_by);
bool adjust_track_pan(Track *track, float change_by);

void copy_clips_to_clipboard();
void paste_clips();

void add_filter_to_track(Track *track, FilterType type, uint16_t impulse_response_len);





#endif
