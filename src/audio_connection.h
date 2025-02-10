/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/


/*****************************************************************************************************************
    audio_connection.h

    * Typedef structs for audio connections (e.g. devices)
 *****************************************************************************************************************/


#ifndef JDAW_AUDIO_DEVICE_H
#define JDAW_AUDIO_DEVICE_H

#include <stdbool.h>
#include <time.h>
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
    /* SDL_mutex *buf_lock; */
} PdConn;

typedef struct jdaw_conn {
    float *rec_buffer_L;
    float *rec_buffer_R;
    uint32_t rec_buf_len_sframes;
    int32_t write_bufpos_sframes;
} JDAWConn;

enum audio_conn_type {
    DEVICE,
    PURE_DATA,
    JACKDAW
};

union audio_conn_substruct {
    AudioDevice device;
    PdConn pd;
    JDAWConn jdaw;
    
};

struct realtime_tick {
    struct timespec ts;
    int32_t timeline_pos;
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
    bool current_clip_repositioned;
    struct realtime_tick callback_time;
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

void audioconn_reset_chunk_size(AudioConn *conn, uint16_t new_chunk_size);

void copy_conn_buf_to_clip(Clip *clip, enum audio_conn_type type);
/* int query_audio_devices(Project *proj, int iscapture); */
/* int device_open(Project *proj, AudioDevice *device); */
/* void device_start_playback(AudioDevice *dev); */
/* void device_stop_playback(AudioDevice *dev); */
/* /\* Pause the device, and then close it. Record devices remain closed. *\/ */
/* void device_stop_recording(AudioDevice *dev); */
/* void device_start_recording(AudioDevice *dev); */

#endif
