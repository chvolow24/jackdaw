/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

/*****************************************************************************************************************
    project.h

    * Project, Timeline, and Track definitions
    * and related interfaces
*****************************************************************************************************************/

#ifndef JDAW_PROJECT_H
#define JDAW_PROJECT_H


#include <complex.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stdint.h>
#include "api.h"
#include "automation.h"
#include "components.h"
#include "effect.h"
#include "eq.h"
#include "endpoint.h"
#include "grab.h"
#include "midi_clip.h"
#include "tempo.h"
#include "timeview.h"
#include "saturation.h"
#include "synth.h"
#include "textbox.h"

#define MAX_TRACKS 255
#define MAX_TRACK_CLIPS 2048
#define MAX_TRACK_BUS_INS MAX_TRACKS
#define MAX_ACTIVE_CLIPS 255
#define MAX_ACTIVE_TRACKS 255
#define MAX_CLIP_REFS 2048
/* #define MAX_MIDI_REFS 2048 */
#define MAX_CLIPBOARD_CLIPS 255
#define MAX_PROJ_TIMELINES 255
#define MAX_PROJ_AUDIO_CONNS 255
#define MAX_PROJ_CLIPS 2048
#define MAX_PROJ_MIDI_CLIPS MAX_PROJ_CLIPS
#define MAX_GRABBED_CLIPS 255
/* #define MAX_TRACK_FILTERS 4 */
/* #define MAX_TRACK_EFFECTS 16 */

#define TRACK_VOL_STEP 0.03f
#define TRACK_PAN_STEP 0.01f
#define TRACK_VOL_MAX 3.0f
#define TRACK_VOL_MAX_PRE_EXP (pow(TRACK_VOL_MAX, (1.0 / VOL_EXP)))

#define MAX_PLAY_SPEED 4096.0

#define BUBBLE_CORNER_RADIUS 6
#define MUTE_SOLO_BUTTON_CORNER_RADIUS 4

#define PLAYHEAD_TRI_H (10 * main_win->dpi_scale_factor)

#define PROJ_NUM_METRONOMES 1

#define MAX_ANIMATIONS 64

#define ACTIVE_TL ( \
session->proj_reading ? session->proj_reading->timelines[session->proj_reading->active_tl_index] : \
session->proj.timelines[session->proj.active_tl_index]) \

typedef struct project Project;
typedef struct timeline Timeline;
/* typedef struct audio_device AudioDevice; */
typedef struct audio_conn AudioConn;
typedef struct clip Clip;
typedef struct clip_ref ClipRef;
/* typedef struct fir_filter FIRFilter; */
/* typedef struct delay_line DelayLine; */

enum track_in_type {
    AUDIO_CONN,
    MIDI_DEVICE,
};

enum midi_out_type {
    MIDI_OUT_DEVICE,
    MIDI_OUT_SYNTH
};

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

    ClipRef **clips;
    uint16_t num_clips;
    uint16_t clips_alloc_len;
    /* ClipRef *clips[MAX_TRACK_CLIPS]; */
    /* uint16_t num_clips; */

    /* MIDIClipRef **midi_cliprefs; */
    /* uint16_t num_midi_cliprefs; */
    /* uint16_t midi_cliprefs_alloc_len; */
    /* uint8_t num_grabbed_clips; */

    uint16_t num_takes;

    /* AudioConn *input; */
    void *input;
    enum track_in_type input_type;
    AudioConn *output;


    void *midi_out;
    enum midi_out_type midi_out_type;
    Synth *synth; /* Pointer will be duplicated in midi_out */

    MIDIEventRingBuf note_offs;

    float vol_ctrl_val; /* Before scaling */
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

    /* double *buf_L_freq_mag; */
    /* double *buf_R_freq_mag; */

    EffectChain effect_chain;
    
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

    const char* added_from_midi_filepath;
} Track;


/* typedef struct clip_ref { */
/*     char name[MAX_NAMELENGTH]; */
/*     bool deleted; */
/*     int32_t pos_sframes; */
/*     uint32_t in_mark_sframes; */
/*     uint32_t out_mark_sframes; */
/*     uint32_t start_ramp_len; */
/*     uint32_t end_ramp_len; */
/*     Clip *clip; */
/*     Track *track; */
/*     bool home; */
/*     bool grabbed; */

/*     Layout *layout; */
/*     /\* SDL_Rect rect; *\/ */
/*     pthread_mutex_t lock; */
/*     Textbox *label; */

/*     SDL_Texture *waveform_texture; */
/*     pthread_mutex_t waveform_texture_lock; */
/*     bool waveform_redraw; */
/* } ClipRef; */
    
/* typedef struct clip { */
/*     char name[MAX_NAMELENGTH]; */
/*     bool deleted; */
/*     uint8_t channels; */
/*     uint32_t len_sframes; */
/*     /\* ClipRef *refs[MAX_CLIP_REFS]; *\/ */
/*     /\* uint16_t num_refs; *\/ */
    
/*     ClipRef **refs; */
/*     uint16_t num_refs; */
/*     uint16_t refs_alloc_len; */
    
/*     float *L; */
/*     float *R; */
/*     uint32_t write_bufpos_sframes; */
/*     /\* Recording in *\/ */
/*     Track *target; */
/*     bool recording; */
/*     AudioConn *recorded_from; */

/*     /\* /\\* Xfade *\\/ *\/ */
/*     /\* uint32_t start_ramp_len_sframes; *\/ */
/*     /\* uint32_t end_ramp_len_sframes; *\/ */
    
/* } Clip; */

typedef struct timecode {
    uint8_t hours;
    uint8_t minutes;
    uint8_t seconds;
    uint8_t milliseconds;
    uint32_t frames;
    char str[16]; // e.g. "+00:00:00:96000"
} Timecode;

struct grabbed_clip_info {
    Track *track;
    int32_t pos;
    int32_t start_in_clip;
    int32_t end_in_clip;
    ClipRefEdge grabbed_edge;
    int track_offset;
};

/* The project timeline organizes included tracks and specifies how they should be displayed */
typedef struct timeline {
    char name[MAX_NAMELENGTH];
    uint8_t index;
    int32_t play_pos_sframes; /* Incremented in AUDIO DEVICE thread (small chunks) */
    int32_t read_pos_sframes; /* Incremented in DSP thread (large chunks) */
    /* float last_read_playspeed; */
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
    uint32_t buf_read_pos;
    uint32_t buf_write_pos;
    sem_t *readable_chunks;
    sem_t *writable_chunks;
    sem_t *unpause_sem;
    /* dsp_chunks_info records information about buffered fourier-length
       chunks, which is used in the playback callback to reset the playhead
       position correctly */
    struct dsp_chunk_info *dsp_chunks_info;
    int dsp_chunks_info_read_i;
    int dsp_chunks_info_write_i;
    
    
    Track *tracks[MAX_TRACKS];  
    uint8_t num_tracks;

    ClickTrack *click_tracks[MAX_CLICK_TRACKS];
    uint8_t num_click_tracks;
    
    // uint8_t active_track_indices[MAX_ACTIVE_TRACKS];
    // uint8_t num_active_tracks;

    int layout_selector; /* Agnostic of track "type"; selects audio OR tempo track */
    int track_selector; /* Index of selected track, or -1 if N/A */
    int click_track_selector; /* Index of selected tempo track */
    bool click_track_frozen;

    Timecode timecode;
    
    Project *proj;

    /* ClipRef *grabbed_clips[MAX_GRABBED_CLIPS]; */
    ClipRef *grabbed_clips[MAX_GRABBED_CLIPS];
    uint8_t num_grabbed_clips;
    struct grabbed_clip_info grabbed_clip_info_cache[MAX_GRABBED_CLIPS];
    bool grabbed_clip_cache_initialized;
    bool grabbed_clip_cache_pushed;

    ClipRef *clipboard[MAX_GRABBED_CLIPS];
    /* ClipRef *clipboard[MAX_GRABBED_CLIPS]; */
    uint8_t num_clips_in_clipboard;

    /* Clip *clip_clipboard[MAX_CLIPBOARD_CLIPS]; */
    /* uint8_t num_clipboard_clips; */
    /* int32_t leftmost_clipboard_clip_pos; */
    struct timespec play_pos_moved_at;
    
    Keyframe *dragging_keyframe;
    int32_t dragging_kf_cache_pos;
    Value dragging_kf_cache_val;

    /* GUI members */

    /* DEPRECATE Timeline-level layout;
       it is too complicated to swap this out when swapping projects */
    /* Layout *layout; */
    
    Layout *track_area;

    TimeView timeview;
    /* int32_t display_offset_sframes; */
    /* int sample_frames_per_pixel; */
    /* int display_v_offset; */

    bool needs_redraw;
    bool needs_reset; /* trigger reset from another thread */

    /* API */

    APINode api_node;
} Timeline;


/* A Jackdaw project. Only one can be active at a time. Can persist on disk as a .jdaw file (see dot_jdaw.c, dot_jdaw.h) */
typedef struct project {
    
    char name[MAX_NAMELENGTH];
    Timeline *timelines[MAX_PROJ_TIMELINES];
    uint8_t num_timelines;
    uint8_t active_tl_index;

    /* Audio settings */
    uint8_t channels;
    uint32_t sample_rate; //samples per second
    SDL_AudioFormat fmt;
    uint16_t chunk_size_sframes; //sample_frames
    uint16_t fourier_len_sframes;

    /* Clips */
    Clip *clips[MAX_PROJ_CLIPS];
    uint16_t num_clips;
    uint16_t active_clip_index;

    MIDIClip *midi_clips[MAX_PROJ_MIDI_CLIPS];
    uint16_t num_midi_clips;
    uint16_t active_midi_clip_index;

    /* Output buffers */
    float *output_L;
    float *output_R;
    double *output_L_freq;
    double *output_R_freq;
    EnvelopeFollower output_L_ef;
    EnvelopeFollower output_R_ef;
} Project;

int project_init(
    Project *proj,
    char *name,
    uint8_t channels,
    uint32_t sample_rate,
    SDL_AudioFormat fmt,
    uint16_t chunk_size_sframes,
    uint16_t fourier_len_sframes,
    bool create_empty_timeline
    );

/* Return the index of a timeline to switch to (new one if success) */
uint8_t project_add_timeline(Project *proj, char *name);
void project_reset_tl_label(Project *proj);
void project_set_chunk_size(uint16_t new_chunk_size);
Track *timeline_add_track(Timeline *tl, int at);
Track *timeline_add_track_with_name(Timeline *tl, const char *track_name, int at);

Track *timeline_selected_track(Timeline *tl);
void timeline_select_track(Track *track);
ClickTrack *timeline_selected_click_track(Timeline *tl);
Layout *timeline_selected_layout(Timeline *tl);

void timeline_reset_full(Timeline *tl);
void timeline_reset(Timeline *tl, bool rescaled);

/* ClipRef *track_add_clipref(Track *track, Clip *clip, int32_t record_from_sframes, bool home); */
/* MIDIClipRef *track_add_midiclipref(Track *track, MIDIClip *clip, int32_t record_from_sframes); */
/* int32_t clipref_len(ClipRef *cr); */
/* bool clipref_marked(Timeline *tl, ClipRef *cr); */
/* int32_t clip_ref_len(ClipRef *cr); */
/* void clipref_reset(ClipRef *cr); */
/* void clipref_reset(ClipRef *cr, bool rescaled); */
/* void midi_clipref_reset(MIDIClipRef *mcr, bool rescaled); */

/* void clipref_displace(ClipRef *cr, int displace_by); */
/* void clipref_move_to_track(ClipRef *cr, Track *target); */
/* int clipref_split_stereo_to_mono(ClipRef *cr, ClipRef **new_L, ClipRef **new_R); */

void track_increment_vol(Track *track);
void track_decrement_vol(Track *track);
void track_increment_pan(Track *track);
void track_decrement_pan(Track *track);
bool track_mute(Track *track);
bool track_solo(Track *track);
void track_solomute(Track *track);
void track_unsolomute(Track *track);

/* Explicitly provide an audio connection or MIDI device */
void track_set_input_to(Track *track, enum track_in_type type, void *obj);

/* Create menu to select input */
void track_set_input(Track *track);
void track_set_out_builtin_synth(Track *track);
void track_set_midi_out(Track *track);
void track_rename(Track *track);
void track_set_bus_out(Track *track, Track *bus_out);
void track_delete(Track *track);
void track_undelete(Track *track);
void track_destroy(Track *track, bool displace);

void track_or_tracks_solo(Timeline *tl, Track *opt_track);
void track_or_tracks_mute(Timeline *tl);

bool check_unfreeze_click_track(Timeline *tl);

/* ClipRef *clipref_at_cursor(); */
/* ClipRef *clipref_at_cursor_in_track(Track *track); */
/* ClipRef *clipref_before_cursor(int32_t *pos_dst); */
/* ClipRef *clipref_after_cursor(int32_t *pos_dst); */
/* void clipref_bring_to_front(); */
/* void clipref_rename(ClipRef *cr); */
/* void timeline_ungrab_all_cliprefs(Timeline *tl); */
/* void clipref_grab(ClipRef *cr); */
/* void clipref_ungrab(ClipRef *cr); */
/* void clipref_destroy(ClipRef *cr, bool); */
/* void clipref_destroy_no_displace(ClipRef *cr); */
/* void clipref_delete(ClipRef *cr); */
/* void clipref_undelete(ClipRef *cr); */
/* void clip_destroy(Clip *clip); */
void timeline_delete(Timeline *tl, bool from_undo);
/* void timeline_cache_grabbed_clip_offsets(Timeline *tl); */
/* void timeline_cache_grabbed_clip_positions(Timeline *tl); */
/* void timeline_push_grabbed_clip_move_event(Timeline *tl); */
/* void timeline_ungrab_all_cliprefs(Timeline *tl); */
/* Deprecated; replaced by timeline_delete_grabbed_cliprefs */
/* void timeline_destroy_grabbed_cliprefs(Timeline *tl); */
/* void timeline_delete_grabbed_cliprefs(Timeline *tl); */
/* void timeline_move_track(Timeline *tl, Track *track, int direction, bool from_undo); */
void timeline_switch(uint8_t new_tl_index);

bool timeline_check_set_midi_monitoring();
void timeline_force_stop_midi_monitoring();
void project_deinit(Project *proj);

void session_set_default_out(void *nullarg);

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

/* Last click track above selected track */
ClickTrack *timeline_governing_click_track(Timeline *tl);

/* Last click track above track "t" */
ClickTrack *track_governing_click_track(Track *t);

/* Completion function for renameable proj objects */
int project_obj_name_completion(Text *txt, void *obj);

#endif
