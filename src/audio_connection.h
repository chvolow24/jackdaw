/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/


/*****************************************************************************************************************
    audio_connection.h

    * Query for available audio connections
    * Open and close individual audio connections
 *****************************************************************************************************************/


#ifndef JDAW_AUDIO_DEVICE_H
#define JDAW_AUDIO_DEVICE_H

#include "SDL.h"
#include <stdbool.h>
#include <time.h>

#define MAX_CONN_NAMELENGTH 128
#define MAX_DEV_NAMELENGTH MAX_CONN_NAMELENGTH
#define MAX_INPUT_CHANNELS 16
/* #include "project.h" */

/* typedef struct project Project; */
typedef struct clip Clip;
typedef struct audio_device AudioDevice;

typedef struct audio_conn AudioConn;

struct channel_dst {
    AudioConn *conn;
    int channel;
};


/* Struct to contain information related to an audio device, including the SDL_AudioDeviceID. */
typedef struct audio_device{
    char name[MAX_CONN_NAMELENGTH];
    SDL_AudioSpec spec;
    int index; /* Valid only between calls to SDL_GetNumAudioDevices */
    SDL_AudioDeviceID id;
    int16_t *rec_buffer;
    uint32_t rec_buf_len_samples;
    int32_t write_bufpos_samples;
    bool open;
    bool playing; /* i.e., "unpaused," has callback running */
    volatile bool request_close;

    bool is_default;
    bool has_channel_cfg;
    struct channel_dst channel_dsts[MAX_INPUT_CHANNELS];
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

/* union audio_conn_substruct { */
/*     AudioDevice device; */
/*     PdConn pd; */
/*     JDAWConn jdaw;    */
/* }; */

struct realtime_tick {
    struct timespec ts;
    int32_t timeline_pos;
};

struct conn_channel_cfg {
    int L_src; /* Identifies channel in source device */
    int R_src; /* -1 if mono */
};

typedef struct audio_conn {
    bool iscapture;
    int index;
    /* const char *name; */
    char name[MAX_CONN_NAMELENGTH];
    bool open;
    bool active;
    bool available;
    bool playing;
    Clip *current_clip; /* The clip currently being recorded, if applicable */
    bool current_clip_repositioned;
    /* struct realtime_tick callback_time; */ /* Deprecated (for now) 2026-01-21 */
    enum audio_conn_type type;
    void *obj; /* AudioDevice, PdConn, or JDAWConn */    
    struct conn_channel_cfg channel_cfg;
    bool is_default;
    
    bool request_playhead_reset;
    int32_t request_playhead_pos;
} AudioConn;


/* int query_audio_conns(Project *proj, int iscapture); */
typedef struct session Session;
int audio_io_get_connections(Session *session, int iscapture);
int audioconn_open(Session *session, AudioConn *conn);
void audioconn_close(AudioConn *conn);
void audioconn_start_playback(AudioConn *conn);
void audioconn_stop_playback(AudioConn *conn);
void audioconn_start_recording(AudioConn *conn);
void audioconn_stop_recording(AudioConn *conn);

void audioconn_handle_connection_event(int index, int iscapture, int event_type);

/* Free audio connection, not including linked device(s). List invalidated. */
void audioconn_destroy(AudioConn *conn);
/* Free audio device, not including linked conn(s). List invalidated. */
void audio_device_destroy(AudioDevice *dev);

void audioconn_reset_chunk_size(AudioConn *conn, uint16_t new_chunk_size);

typedef struct session Session;
void session_init_audio_conns(Session *session);
void session_set_default_out(void *nullarg);

#endif
