/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/


/*****************************************************************************************************************
    session.h

    * dumping ground for "global" program state
    * contains a single active Project
    * session_get() must be called by all functions that access or modify the global state
    * (successor to global Project *proj, deprecated after v0.6.0)
 *****************************************************************************************************************/

#ifndef JDAW_SESSION_H
#define JDAW_SESSION_H

#include "audio_connection.h"
#include "clipref.h"
#include "loading.h"
#include "midi_io.h"
#include "panel.h"
#include "project.h"
#include "status.h"
/* #include "synth.h" */
#include "tempo.h"
#include "timeview.h"
#include "user_event.h"

#define MAX_SESSION_AUDIO_CONNS 32
#define SESSION_NUM_METRONOMES 1
#define MAX_QUEUED_OPS 255

struct audio_io {
    AudioConn *record_conns[MAX_SESSION_AUDIO_CONNS];
    uint8_t num_record_conns;
    AudioConn *playback_conns[MAX_SESSION_AUDIO_CONNS];
    uint8_t num_playback_conns;
    AudioConn *playback_conn;
    /* uint8_t playback_conn_index; */
};

struct session_gui {
    char timeline_label_str[MAX_NAMELENGTH];
    Textbox *timeline_label;

    struct freq_plot *freq_domain_plot;
    Layout *layout;
    Layout *timeline_lt;
    SDL_Rect *audio_rect;
    SDL_Rect *control_bar_rect;
    SDL_Rect *ruler_rect;
    SDL_Rect *console_column_rect;
    SDL_Rect *hamburger;
    SDL_Rect *bun_patty_bun[3];
    Textbox *source_name_tb;

    Textbox *timecode_tb;
    Textbox *loop_play_lemniscate;

    bool panels_initialized;
    PanelArea *panels;

    SDL_Texture *left_arrow_texture;
    SDL_Texture *right_arrow_texture;
    
};

struct playhead_scroll {
    float playhead_frame_incr;
    bool playhead_do_incr;
};


/* for sample mode */
struct drop_save {
    Clip *clip;
    int32_t in;
    int32_t out;
};

struct queued_val_change {
    Endpoint *ep;
    Value new_val;
    bool run_gui_cb;
};

struct status_bar {
    Layout *layout;
    char errstr[MAX_STATUS_STRLEN];
    char callstr[MAX_STATUS_STRLEN];
    char dragstr[MAX_STATUS_STRLEN];
    Textbox *error;
    Textbox *call;
    Textbox *dragstat;
    int stat_timer;
    int call_timer;
    int err_timer;
};

struct queued_ops {
    struct queued_val_change queued_val_changes[NUM_JDAW_THREADS][MAX_QUEUED_OPS];
    uint8_t num_queued_val_changes[NUM_JDAW_THREADS];
    pthread_mutex_t queued_val_changes_lock;
    
    EndptCb queued_callbacks[NUM_JDAW_THREADS][MAX_QUEUED_OPS];
    Endpoint *queued_callback_args[NUM_JDAW_THREADS][MAX_QUEUED_OPS];
    uint8_t num_queued_callbacks[NUM_JDAW_THREADS];
    pthread_mutex_t queued_callback_lock;

    Endpoint *ongoing_changes[NUM_JDAW_THREADS][MAX_QUEUED_OPS];
    uint8_t num_ongoing_changes[NUM_JDAW_THREADS];
    pthread_mutex_t ongoing_changes_lock;

};

struct source_mode {    
    bool source_mode;
    ClipType src_clip_type;
    void *src_clip;
    int32_t src_play_pos_sframes;
    int32_t src_in_sframes;
    int32_t src_out_sframes;
    float src_play_speed;

    TimeView timeview;

    struct drop_save saved_drops[5];
    uint8_t num_dropped;
};

struct playback {
    float play_speed;
    Endpoint play_speed_ep;
    bool loop_play;
    bool dragging;
    bool recording;
    bool playing;
};

/* struct audio_settings { */
/*     uint8_t channels; */
/*     uint32_t sample_rate; //samples per second */
/*     SDL_AudioFormat fmt; */
/*     uint16_t chunk_size_sframes; //sample_frames */
/*     uint16_t fourier_len_sframes; */
/* }; */

/* All persistent "global" data not related to a Project or Window */
typedef struct session {
    struct audio_io audio_io;
    struct midi_io midi_io;
    /* pthread_t main_thread; */
    /* pthread_t dsp_thread; */
    /* pthread_t playback_thread; */
    UserEventHistory history;
    struct playhead_scroll playhead_scroll;
    int quit_count;
    Metronome metronomes[SESSION_NUM_METRONOMES];
    struct status_bar status_bar;
    LoadingScreen loading_screen;
    Animation *animations;
    struct api_server server;
    struct queued_ops queued_ops;
    bool midi_qwerty; // if true, midi qwerty mode is active (local state in midi_qwerty.c)
    bool piano_roll; // if true, piano roll visible (local state in piano_roll.c)
    bool do_tests;
    Draggable dragged_component;
    bool dragging;
    struct source_mode source_mode;
    Automation *automation_recording;
    struct session_gui gui;
    struct playback playback;

    bool proj_initialized;
    Project *proj_reading;
    Project proj;
    
} Session;


Session *session_create();
Session *session_get();
void session_destroy();
void session_set_proj(Session *session, Project *new_proj);
uint32_t session_get_sample_rate();

#endif
