#ifndef JDAW_SESSION_H
#define JDAW_SESSION_H

#include "audio_connection.h"
#include "loading.h"
#include "midi_io.h"
#include "panel.h"
#include "project.h"
#include "status.h"
#include "tempo.h"
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

struct midi_io {
    MIDIDevice in;
    bool in_active;
    MIDIDevice out;
    bool out_active;
};

struct session_gui {
    char timeline_label_str[MAX_NAMELENGTH];
    Textbox *timeline_label;

    struct freq_plot *freq_domain_plot;
    Layout *layout;
    SDL_Rect *audio_rect;
    SDL_Rect *control_bar_rect;
    SDL_Rect *ruler_rect;
    SDL_Rect *console_column_rect;
    SDL_Rect *hamburger;
    SDL_Rect *bun_patty_bun[3];
    Textbox *source_name_tb;

    PanelArea *panels;
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
    Clip *src_clip;
    int32_t src_play_pos_sframes;
    int32_t src_in_sframes;
    int32_t src_out_sframes;
    float src_play_speed;

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

/* All persistent "global" data not related to a Project or Window */
typedef struct session {
    struct audio_io audio_io;
    struct midi_io midi_io;
    pthread_t dsp_thread;
    UserEventHistory history;
    struct playhead_scroll playhead_scroll;
    int quit_count;
    Metronome metronomes[SESSION_NUM_METRONOMES];
    struct status_bar status_bar;
    LoadingScreen loading_screen;
    Animation *animations;
    struct api_server server;
    struct queued_ops queued_ops;
    bool do_tests;
    Draggable dragged_component;
    bool dragging;
    struct source_mode source_mode;
    Automation *automation_recording;
    struct session_gui gui;
    struct playback playback;

    Project proj;
    
} Session;


Session *session_get();

#endif
