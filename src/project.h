/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

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
#include <semaphore.h>
#include <stdbool.h>
#include <stdint.h>
#include "animation.h"
#include "api.h"
#include "automation.h"
#include "components.h"
/* #include "dsp.h" */
#include "effect.h"
#include "eq.h"
#include "endpoint.h"
#include "loading.h"
#include "panel.h"
#include "tempo.h"
#include "thread_safety.h"
#include "saturation.h"
#include "status.h"
#include "textbox.h"
#include "user_event.h"



#define MAX_TRACKS 255
#define MAX_TRACK_CLIPS 2048
#define MAX_TRACK_BUS_INS MAX_TRACKS
#define MAX_ACTIVE_CLIPS 255
#define MAX_ACTIVE_TRACKS 255
#define MAX_CLIP_REFS 2048
#define MAX_CLIPBOARD_CLIPS 255
#define MAX_PROJ_TIMELINES 255
#define MAX_PROJ_AUDIO_CONNS 255
#define MAX_PROJ_CLIPS 2048
#define MAX_GRABBED_CLIPS 255
/* #define MAX_TRACK_FILTERS 4 */
#define MAX_TRACK_EFFECTS 16

#define TRACK_VOL_STEP 0.03f
#define TRACK_PAN_STEP 0.01f
#define TRACK_VOL_MAX 3.0f

#define MAX_PLAY_SPEED 4096.0

#define BUBBLE_CORNER_RADIUS 6
#define MUTE_SOLO_BUTTON_CORNER_RADIUS 4

#define PROJ_TL_LABEL_BUFLEN 50

#define PLAYHEAD_TRI_H (10 * main_win->dpi_scale_factor)

#define PROJ_NUM_METRONOMES 1

#define MAX_QUEUED_OPS 255
#define MAX_ANIMATIONS 64

typedef struct project Project;
typedef struct timeline Timeline;
/* typedef struct audio_device AudioDevice; */
typedef struct audio_conn AudioConn;
typedef struct clip Clip;
typedef struct clip_ref ClipRef;
/* typedef struct fir_filter FIRFilter; */
/* typedef struct delay_line DelayLine; */

typedef struct track {
    char name[MAX_NAMELENGTH];
    bool deleted;
    bool active;
    bool muted;
    bool solo;
    bool solo_muted;
    bool minimized;
    uint8_t channels;
    Timeline *tl; /* Parent timeline */
    uint8_t tl_rank;

    ClipRef *clips[MAX_TRACK_CLIPS];
    uint16_t num_clips;
    /* uint8_t num_grabbed_clips; */

    uint16_t num_takes;

    AudioConn *input;
    AudioConn *output;

    float vol; /* 0.0 - 1.0 attenuation only */
    float pan; /* 0.0 pan left; 0.5 center; 1.0 pan right */
    Slider *vol_ctrl;
    Slider *pan_ctrl;

    /* double *buf_L; */
    /* double *buf_R; */
    /* uint16_t buf_len; */
    /* uint16_t buf_read_pos; */
    /* uint16_t buf_write_pos; */
    /* sem_t *buf_sem; */

    double *buf_L_freq_mag;
    double *buf_R_freq_mag;


    Effect *effects[MAX_TRACK_EFFECTS];
    uint8_t num_effects;
    uint8_t num_effects_per_type[NUM_EFFECT_TYPES];
    pthread_mutex_t effect_chain_lock;
    /* double order_swap_indices[2]; /\* exploting existence of double_pair jdaw val type *\/ */
    /* Endpoint effect_order_swap_ep; */
    
    /* EQ eq; */

    /* Saturation saturation; */

    /* FIRFilter fir_filter; */
    /* bool fir_filter_active; */
    
    /* DelayLine delay_line; */
    /* bool delay_line_active; */

    Automation *automations[MAX_TRACK_AUTOMATIONS];
    uint8_t num_automations;
    int16_t selected_automation;
    bool some_automations_shown;
    bool some_automations_read;
    /* uint8_t num_filters; */
    
    /* FSLIDER *vol_ctrl */
    /* FSlider *pan_ctrl */

    Layout *layout;
    Layout *inner_layout;
    /* Textbox *tb_name; */
    TextEntry *tb_name;
    Textbox *tb_input_label;
    Textbox *tb_input_name;
    Textbox *tb_vol_label;
    Textbox *tb_pan_label;
    Textbox *tb_mute_button;
    Textbox *tb_solo_button;
    SymbolButton *automation_dropdown;

    SDL_Rect *console_rect;
    SDL_Rect *colorbar;
    SDL_Color color;

    /* API */
    APINode api_node;

    Endpoint mute_ep;
    Endpoint solo_ep;
    Endpoint vol_ep;
    Endpoint pan_ep;


    /* Routing */
    Track *bus_out;
    Track **bus_ins;
    uint8_t bus_ins_arrlen;
    uint8_t num_bus_ins;
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

    Layout *layout;
    /* SDL_Rect rect; */
    pthread_mutex_t lock;
    Textbox *label;

    SDL_Texture *waveform_texture;
    pthread_mutex_t waveform_texture_lock;
    bool waveform_redraw;
} ClipRef;
    
typedef struct clip {
    char name[MAX_NAMELENGTH];
    bool deleted;
    uint8_t channels;
    uint32_t len_sframes;
    ClipRef *refs[MAX_CLIP_REFS];
    uint16_t num_refs;
    float *L;
    float *R;
    uint32_t write_bufpos_sframes;
    /* Recording in */
    Track *target;
    bool recording;
    AudioConn *recorded_from;

    /* /\* Xfade *\/ */
    /* uint32_t start_ramp_len_sframes; */
    /* uint32_t end_ramp_len_sframes; */
    
} Clip;

typedef struct timecode {
    uint8_t hours;
    uint8_t minutes;
    uint8_t seconds;
    uint8_t milliseconds;
    uint32_t frames;
    char str[16]; // e.g. "+00:00:00:96000"
} Timecode;

struct track_and_pos {
    Track *track;
    int32_t pos;
    int track_offset;
};

/* The project timeline organizes included tracks and specifies how they should be displayed */
typedef struct timeline {
    char name[MAX_NAMELENGTH];
    uint8_t index;
    int32_t play_pos_sframes; /* Incremented in AUDIO DEVICE thread (small chunks) */
    int32_t read_pos_sframes; /* Incremented in DSP thread (large chunks) */
    int32_t in_mark_sframes;
    int32_t out_mark_sframes;
    int32_t record_from_sframes;

    /*
    - Channel buffers allocated at fourier len (e.g. 1024) * 2
    - writer calls sem_post for every chunk (chunk_size, e.g. 64) available
    - reader calls sem_wait for every chunk requested (in audio thread)
    */
    float *buf_L;
    float *buf_R;
    uint16_t buf_read_pos;
    uint16_t buf_write_pos;
    sem_t *readable_chunks;
    sem_t *writable_chunks;
    sem_t *unpause_sem;
    pthread_t mixdown_thread;
    
    Track *tracks[MAX_TRACKS];  
    uint8_t num_tracks;

    ClickTrack *click_tracks[MAX_CLICK_TRACKS];
    uint8_t num_click_tracks;
    // uint8_t active_track_indices[MAX_ACTIVE_TRACKS];
    // uint8_t num_active_tracks;

    int layout_selector; /* Agnostic of track "type"; selects audio OR tempo track */
    int track_selector; /* Index of selected track, or -1 if N/A */
    int click_track_selector; /* Index of selected tempo track */
    

    Timecode timecode;
    Textbox *timecode_tb;
    
    Project *proj;

    ClipRef *grabbed_clips[MAX_GRABBED_CLIPS];
    uint8_t num_grabbed_clips;
    struct track_and_pos grabbed_clip_pos_cache[MAX_GRABBED_CLIPS];
    bool grabbed_clip_cache_initialized;
    bool grabbed_clip_cache_pushed;

    ClipRef *clipboard[MAX_GRABBED_CLIPS];
    uint8_t num_clips_in_clipboard;

    /* Clip *clip_clipboard[MAX_CLIPBOARD_CLIPS]; */
    /* uint8_t num_clipboard_clips; */
    /* int32_t leftmost_clipboard_clip_pos; */
    struct timespec play_pos_moved_at;
    
    Keyframe *dragging_keyframe;
    int32_t dragging_kf_cache_pos;
    Value dragging_kf_cache_val;

    /* GUI members */
    Layout *layout;
    Layout *track_area;
    int32_t display_offset_sframes; // in samples frames
    int sample_frames_per_pixel;
    int display_v_offset;
    Textbox *loop_play_lemniscate;

    bool needs_redraw;

    /* API */

    APINode api_node;
} Timeline;


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

/* A Jackdaw project. Only one can be active at a time. Can persist on disk as a .jdaw file (see dot_jdaw.c, dot_jdaw.h) */
typedef struct project {
    char name[MAX_NAMELENGTH];
    Timeline *timelines[MAX_PROJ_TIMELINES];
    uint8_t num_timelines;
    uint8_t active_tl_index;
    char timeline_label_str[PROJ_TL_LABEL_BUFLEN];
    Textbox *timeline_label;
    
    float play_speed;
    Endpoint play_speed_ep;
    bool loop_play;
    bool dragging;
    bool recording;
    bool playing;
    Automation *automation_recording;

    AudioConn *record_conns[MAX_PROJ_AUDIO_CONNS];
    uint8_t num_record_conns;
    AudioConn *playback_conns[MAX_PROJ_AUDIO_CONNS];
    uint8_t num_playback_conns;

    /* Audio settings */
    AudioConn *playback_conn;
    uint8_t channels;
    uint32_t sample_rate; //samples per second
    SDL_AudioFormat fmt;
    uint16_t chunk_size_sframes; //sample_frames
    uint16_t fourier_len_sframes;

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

    /* Panel area */
    PanelArea *panels;

    /* Audio output */
    float *output_L;
    float *output_R;
    /* uint16_t output_len; */

    double *output_L_freq;
    double *output_R_freq;


    /* DSP thread */
    pthread_t dsp_thread;

    struct freq_plot *freq_domain_plot;

    /* In-progress events */
    float playhead_frame_incr;
    bool playhead_do_incr;
    /* bool vol_changing; */
    /* bool vol_up; */
    /* bool pan_changing; */
    /* bool pan_right; */
    /* bool show_output_freq_domain; */

    Draggable dragged_component;
    /* Slider *currently_editing_slider; */
    /* Value cached_slider_val; */
    /* enum slider_target_type cached_slider_type; */

    /* Event History (undo/redo) */
    UserEventHistory history;

    /* Quitting */
    int quit_count;

    /* Metronomes */
    Metronome metronomes[PROJ_NUM_METRONOMES];

    /* GUI Members */
    Layout *layout;
    SDL_Rect *audio_rect;
    SDL_Rect *control_bar_rect;
    SDL_Rect *ruler_rect;
    SDL_Rect *console_column_rect;
    SDL_Rect *hamburger;
    SDL_Rect *bun_patty_bun[3];
    Textbox *source_name_tb;

    /* Status bar */
    struct status_bar status_bar;

    /* Loading Screen */
    LoadingScreen loading_screen;

    /* Source mode state */
    struct drop_save saved_drops[5];
    uint8_t num_dropped;


    /* Animations */
    Animation *animations;
    /* pthread_mutex_t animation_lock; */

    /* Endpoints API */
    struct api_server server;
    /* APINode api_root; */
    /* bool server_active; */
    /* int server_port; */
    
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


    bool do_tests;
} Project;

Project *project_create(
    char *name,
    uint8_t channels,
    uint32_t sample_rate,
    SDL_AudioFormat fmt,
    uint16_t chunk_size_sframes,
    uint16_t fourier_len_sframes
    );

/* Return the index of a timeline to switch to (new one if success) */
uint8_t project_add_timeline(Project *proj, char *name);
void project_reset_tl_label(Project *proj);
void project_set_chunk_size(uint16_t new_chunk_size);
Track *timeline_add_track(Timeline *tl);

Track *timeline_selected_track(Timeline *tl);
ClickTrack *timeline_selected_click_track(Timeline *tl);
Layout *timeline_selected_layout(Timeline *tl);

void timeline_reset_full(Timeline *tl);
void timeline_reset(Timeline *tl, bool rescaled);
Clip *project_add_clip(AudioConn *dev, Track *target);
ClipRef *track_create_clip_ref(Track *track, Clip *clip, int32_t record_from_sframes, bool home);
int32_t clipref_len(ClipRef *cr);
bool clipref_marked(Timeline *tl, ClipRef *cr);
/* int32_t clip_ref_len(ClipRef *cr); */
/* void clipref_reset(ClipRef *cr); */
void clipref_reset(ClipRef *cr, bool rescaled);

void clipref_displace(ClipRef *cr, int displace_by);
void clipref_move_to_track(ClipRef *cr, Track *target);
int clipref_split_stereo_to_mono(ClipRef *cr, ClipRef **new_L, ClipRef **new_R);

void track_increment_vol(Track *track);
void track_decrement_vol(Track *track);
void track_increment_pan(Track *track);
void track_decrement_pan(Track *track);
bool track_mute(Track *track);
bool track_solo(Track *track);
void track_solomute(Track *track);
void track_unsolomute(Track *track);
void track_set_input(Track *track);
void track_rename(Track *track);
void track_set_bus_out(Track *track, Track *bus_out);
void track_delete(Track *track);
void track_undelete(Track *track);
void track_destroy(Track *track, bool displace);

void track_or_tracks_solo(Timeline *tl, Track *opt_track);
void track_or_tracks_mute(Timeline *tl);

ClipRef *clipref_at_cursor();
ClipRef *clipref_at_cursor_in_track(Track *track);
ClipRef *clipref_before_cursor(int32_t *pos_dst);
ClipRef *clipref_after_cursor(int32_t *pos_dst);
void clipref_bring_to_front();
void clipref_rename(ClipRef *cr);
void timeline_ungrab_all_cliprefs(Timeline *tl);
void clipref_grab(ClipRef *cr);
void clipref_ungrab(ClipRef *cr);
void clipref_destroy(ClipRef *cr, bool);
void clipref_destroy_no_displace(ClipRef *cr);
void clipref_delete(ClipRef *cr);
void clipref_undelete(ClipRef *cr);
void clip_destroy(Clip *clip);
void timeline_delete(Timeline *tl, bool from_undo);
void timeline_cache_grabbed_clip_offsets(Timeline *tl);
void timeline_cache_grabbed_clip_positions(Timeline *tl);
void timeline_push_grabbed_clip_move_event(Timeline *tl);
/* Deprecated; replaced by timeline_delete_grabbed_cliprefs */
void timeline_destroy_grabbed_cliprefs(Timeline *tl);
void timeline_delete_grabbed_cliprefs(Timeline *tl);
void timeline_cut_at_cursor(Timeline *tl);
/* void timeline_move_track(Timeline *tl, Track *track, int direction, bool from_undo); */
void timeline_switch(uint8_t new_tl_index);
void project_destroy(Project *proj);

void project_set_default_out(void *nullarg);

/* void track_move_automation(Automation *a, int direction, bool from_undo); */
void timeline_move_track_or_automation(Timeline *tl, int direction);
void timeline_rectify_track_area(Timeline *tl);
void timeline_rectify_track_indices(Timeline *tl);
bool timeline_refocus_track(Timeline *tl, Track *track, bool at_bottom);
bool timeline_refocus_click_track(Timeline *tl, ClickTrack *tt, bool at_bottom);
void timeline_play_speed_set(double new_speed);
void timeline_play_speed_mult(double scale_factor);
void timeline_play_speed_adj(double dim);
void timeline_scroll_playhead(double dim);

void timeline_reset_loop_play_lemniscate(Timeline *tl);
bool track_minimize(Track *t);
void timeline_minimize_track_or_tracks(Timeline *tl);
#endif
