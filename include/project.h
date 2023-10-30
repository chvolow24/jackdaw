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
#define MAX_TRACKS 100
#define MAX_NAMELENGTH 255

#define MAX_ACTIVE_CLIPS 25
#define MAX_ACTIVE_TRACKS 25
#define MAX_TRACK_CLIPS 255


typedef struct clip Clip;
typedef struct timeline Timeline;
typedef struct project Project;
typedef struct audiodevice AudioDevice;
typedef struct track Track;
typedef struct textbox Textbox;
typedef struct jdaw_window JDAWWindow;
typedef struct f_slider FSlider;


/* An audio track in the timeline. This object includes handles to various controls and all resident clips */
typedef struct track {
	char name[MAX_NAMELENGTH];
	bool active;
	bool muted;
	bool solo;
	bool record;
	uint8_t channels;
	Timeline *tl;
	uint8_t tl_rank;
	Clip *clips[MAX_TRACK_CLIPS];
	uint8_t num_clips;
	uint8_t num_grabbed_clips;
	AudioDevice *input;
	FSlider *vol_ctrl;
	FSlider *pan_ctrl;

	/* 
	GUI members
	Positions and dimensions are cached to avoid re-calculation on every re-draw. Repositioning and resizing are
	handled in their own functions
		*/
	JDAW_Color *color;

	SDL_Rect rect;
	SDL_Rect console_rect;
	SDL_Rect name_row_rect;
	SDL_Rect input_row_rect;
	SDL_Rect vol_row_rect;
	SDL_Rect pan_row_rect;

	Textbox *name_box;
	Textbox *input_label_box;
	Textbox *input_name_box;
	Textbox *vol_label_box;
	Textbox *pan_label_box;
	Textbox *mute_button_box;
	Textbox *solo_button_box;


} Track;

/* A chunk of audio, associated with a particular track in the timeline */
typedef struct clip {
		char name[MAX_NAMELENGTH];
		Track *track; // parent track
		uint8_t track_rank; // index of clip in parent track.clips member
		uint8_t channels; // the number of audio channels represented in the audio sample data
		uint32_t len_sframes; // length in sample frames
		int32_t abs_pos_sframes; // timeline position in sample frames
		int16_t *pre_proc; // the raw clip audio data
		int16_t *post_proc; // cached audio data after audio processing
		bool done; // true when the clip has finished recording
		AudioDevice *input; // the device used to record the clip, if applicable

		/* GUI members */
		SDL_Rect rect;
		void (*onclick)(void);
		void (*onhover)(void);
		Textbox *namebox;
		bool grabbed;
} Clip;


typedef struct timecode {
	uint8_t hours;
	uint8_t minutes;
	uint8_t seconds;
	uint8_t milliseconds;
	uint32_t frames;
	char str[15]; // e.g. "00:00:00:96000"
} Timecode;

/* The project timeline organizes included tracks and specifies how they should be displayed */
typedef struct timeline {
		uint32_t play_position; // in sample frames
		// uint32_t record_position; // in sample frames
		uint32_t record_offset;
		uint32_t play_offset;
		uint32_t in_mark;
		uint32_t out_mark;
		pthread_mutex_t play_position_lock;
		Track *tracks[MAX_TRACKS];
		uint8_t num_tracks;
		uint8_t active_track_indices[MAX_ACTIVE_TRACKS];
		uint8_t num_active_tracks;
		uint16_t tempo;
		bool click_on;
		Timecode timecode;
		Project *proj;

		/* GUI members */
		SDL_Rect rect;
		SDL_Rect audio_rect;
		SDL_Rect ruler_tc_container_rect;
		SDL_Rect ruler_rect;
		SDL_Rect tc_rect;
		uint32_t offset; // in samples frames
		int sample_frames_per_pixel;
		// int console_width;
		int v_offset;
		Textbox *timecode_tb;

} Timeline;

/* A Jackdaw project. Only one can be active at a time. Can persist on disk as a .jdaw file (see dot_jdaw.c, dot_jdaw.h) */
typedef struct project {
		char name[MAX_NAMELENGTH];
		// bool dark_mode;
		Timeline *tl;
		Clip *loose_clips[100];
		uint8_t num_loose_clips;
		JDAWWindow *jwin;
		Clip *active_clips[MAX_ACTIVE_CLIPS];
		uint8_t num_active_clips;
		float play_speed;
		bool playing;
		bool recording;
		AudioDevice **record_devices;
		AudioDevice **playback_devices;
		int num_record_devices;
		int num_playback_devices;

		/* Audio settings */
		AudioDevice *playback_device;
		uint8_t channels;
		int sample_rate; //samples per second
		SDL_AudioFormat fmt;
		uint16_t chunk_size; //sample_frames

		/* GUI Members */
		SDL_Rect ctrl_rect;

		SDL_Rect ctrl_rect_col_a;
		SDL_Rect audio_out_row;
		Textbox *audio_out_label;
		Textbox *audio_out;

		SDL_Rect ctrl_rect_col_b;

} Project;

int16_t get_track_sample(Track *track, Timeline *tl, uint32_t start_pos, uint32_t pos_in_chunk);
int16_t *get_mixdown_chunk(Timeline* tl, uint32_t length, bool from_mark_in);
Project *create_empty_project(void);
Project *create_project(const char* name, uint8_t channels, int sample_rate, SDL_AudioFormat fmt, uint16_t chunk_size);
Track *create_track(Timeline *tl, bool stereo);
Clip *create_clip(Track *track, uint32_t length, uint32_t absolute_position);
void destroy_clip(Clip *clip);
void destroy_track(Track *track);
void reset_tl_rect(Timeline *tl);
void select_track_input_menu(void *track_v);
void select_audio_out_menu(void *proj_v);

void activate_or_deactivate_track(uint8_t track_index);
// void deactivate_all_tracks(void);
void activate_deactivate_all_tracks(void);


void mute_track(Track *track);
void solo_track(Track *track);
void mute_unmute(void);
void solo_unsolo(void);

void destroy_audio_devices(Project *proj);
void activate_audio_devices(Project *proj);

void reposition_clip(Clip *clip, uint32_t new_pos);
void add_active_clip(Clip *clip);
void clear_active_clips();
void log_project_state(FILE *f);
void grab_clip(Clip *clip);
void grab_ungrab_clip(Clip *clip);
void grab_clips(void);
void ungrab_clips(void);
uint8_t proj_grabbed_clips(void);
void translate_grabbed_clips(int32_t translate_by);
void remove_clip_from_track(Clip *clip);
void delete_clip(Clip *clip);
void delete_grabbed_clips();
void reset_cliprect(Clip* clip);
void reset_track_internal_rects(Track *track);
void reset_tl_rects(Project *proj);
void reset_ctrl_rects(Project *proj);
bool adjust_track_vol(Track *track, float change_by);
bool adjust_track_pan(Track *track, float change_by);



#endif