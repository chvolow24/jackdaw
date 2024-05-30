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
    audio_device.h

    * Query available audio devices
    * Open, pause/unpause, and otherwise
 *****************************************************************************************************************/


#ifndef JDAW_AUDIO_DEVICE_H
#define JDAW_AUDIO_DEVICE_H

#include <stdbool.h>
#include "SDL.h"


#define MAX_CONN_NAMELENGTH 64
/* #include "project.h" */

typedef struct project Project;
typedef struct clip Clip;
typedef struct audio_device AudioDevice;

/* Struct to contain information related to an audio device, including the SDL_AudioDeviceID. */
typedef struct audio_device{
    /* int index; /\* index in the list created by query_audio_devices *\/ */
    /* const char *name; */
    /* bool open; /\* true if the device has been opened (SDL_OpenAudioDevice) *\/ */
    /* bool iscapture; /\* true if device is a recording device, not a playback device *\/ */
    SDL_AudioDeviceID id;
    SDL_AudioSpec spec;
    int16_t *rec_buffer;
    uint32_t rec_buf_len_samples;
    int32_t write_bufpos_samples;
    /* bool active; */
} AudioDevice;

typedef struct pd_conn {
    float *rec_buffer_L;
    float *rec_buffer_R;
    uint32_t rec_buf_len_sframes;
    int32_t write_bufpos_sframes;
} PdConn;

enum audio_conn_type {
    DEVICE,
    PURE_DATA
};

union audio_conn_substruct {
    AudioDevice device;
    PdConn pd;
    
};
typedef struct audio_conn {
    bool iscapture;
    int index;
    /* const char *name; */
    char name[MAX_CONN_NAMELENGTH];
    bool open;
    bool active;
    bool available;
    Clip *current_clip; /* The clip currently being recorded, if applicable */
    enum audio_conn_type type;
    union audio_conn_substruct c;
} AudioConn;


/* int query_audio_conns(Project *proj, int iscapture); */
int query_audio_connections(Project *proj, int iscapture);
int audioconn_open(Project *proj, AudioConn *conn);
void audioconn_close(AudioConn *conn);
void audioconn_start_playback(AudioConn *conn);
void audioconn_stop_playback(AudioConn *conn);
void audioconn_start_recording(AudioConn *conn);
void audioconn_stop_recording(AudioConn *conn);

void audioconn_handle_connection_event(int index, int iscapture, int event_type);

void audioconn_destroy(AudioConn *conn);
/* int query_audio_devices(Project *proj, int iscapture); */
/* int device_open(Project *proj, AudioDevice *device); */
/* void device_start_playback(AudioDevice *dev); */
/* void device_stop_playback(AudioDevice *dev); */
/* /\* Pause the device, and then close it. Record devices remain closed. *\/ */
/* void device_stop_recording(AudioDevice *dev); */
/* void device_start_recording(AudioDevice *dev); */

#endif
