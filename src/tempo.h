/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

/*****************************************************************************************************************
    tempo.h

    * Define structs related to tempo tracks, bars, and beats
 *****************************************************************************************************************/

#ifndef JDAW_TEMPO_H
#define JDAW_TEMPO_H

#include <stdint.h>
#include "api.h"
#include "components.h"
#include "endpoint.h"
#include "layout.h"
#include "textbox.h"

#define MAX_BEATS_PER_BAR 13
#define MAX_CLICK_TRACKS 16
#define CLICK_POS_STR_LEN 32
#define BARS_FOR_NOTHING 2

enum beat_prominence {
    BP_SEGMENT=0,
    BP_MEASURE=1,
    BP_BEAT=2,
    BP_SUBDIV=3,
    BP_SUBDIV2=4,
    BP_NONE=5
};

typedef struct measure_config {
    int bpm;
    int32_t dur_sframes;
    uint8_t num_beats;
    uint8_t beat_subdiv_lens[MAX_BEATS_PER_BAR];
    uint8_t num_atoms;
    int32_t atom_dur_approx; /* Not to be used for precise calculations */
} MeasureConfig;

typedef struct click_track ClickTrack;
typedef struct click_segment ClickSegment;
typedef struct click_segment {
    ClickTrack *track;
    int32_t start_pos;
    /* int32_t end_pos; */
    int16_t first_measure_index;
    int32_t num_measures;
    ClickTrack *click_track;
    MeasureConfig cfg;

    ClickSegment *next;
    ClickSegment *prev;

    int32_t start_pos_internal;
    Endpoint start_pos_ep;
} ClickSegment;

typedef struct timeline Timeline;

typedef struct metronome {
    const char *name;
    /* const char *buffer_filenames[2]; */
    float *buffers[2];
    int32_t buf_lens[2];
} Metronome;


enum ts_end_bound_behavior {
    SEGMENT_FIXED_END_POS=0,
    SEGMENT_FIXED_NUM_MEASURES=1
};
typedef struct click_track {
    char name[255];
    ClickSegment *segments;
    uint8_t index;
    /* uint8_t num_segments; */
    uint8_t current_segment;

    Metronome *metronome;
    float metronome_vol;
    /* Button *metronome_button; */
    Textbox *metronome_button;
    Textbox *edit_button;
    Slider *metronome_vol_slider;
    /* float *metronome_buffers[2]; */
    /* uint16_t metronome_buffer_lens[2]; */
    /* bool metronome_offbeats; */
    /* float metronome_volume; */
    Timeline *tl;

    char pos_str[CLICK_POS_STR_LEN];
    Textbox *readout;

    Layout *layout;
    SDL_Rect *colorbar_rect;
    SDL_Rect *console_rect;
    SDL_Rect *right_console_rect;
    SDL_Rect *right_colorbar_rect;

    bool muted;
    enum ts_end_bound_behavior end_bound_behavior;

    /* Settings GUI objs */
    char num_beats_str[3];
    char tempo_str[5];
    char subdiv_len_strs[MAX_BEATS_PER_BAR][2];

    /* API */
    APINode api_node;
    Endpoint metronome_vol_ep;
    Endpoint end_bound_behavior_ep;
    
} ClickTrack;


/* Timeline interface */
ClickTrack *timeline_add_click_track(Timeline *tl);
void timeline_cut_click_track_at_cursor(Timeline *tl);
void timeline_increment_tempo_at_cursor(Timeline *tl, int inc_by);
void click_track_goto_prox_beat(ClickTrack *tt, int direction, enum beat_prominence bp);
/* void timeline_goto_prox_beat(Timeline *tl, int direction, enum beat_prominence bp); */
void timeline_click_track_set_tempo_at_cursor(Timeline *tl);
void timeline_click_track_edit(Timeline *tl);
bool timeline_click_track_delete(Timeline *tl);


/* Required in settings.c */
ClickSegment *click_track_get_segment_at_pos(ClickTrack *t, int32_t pos);
void click_segment_set_config(ClickSegment *s, int num_measures, int bpm, uint8_t num_beats, uint8_t *subdivs, enum ts_end_bound_behavior ebb);
void click_segment_destroy(ClickSegment *s);

void click_segment_fprint(FILE *f, ClickSegment *s);

int32_t click_track_bar_beat_subdiv(ClickTrack *tt, int32_t pos, int *bar_p, int *beat_p, int *subdiv_p, ClickSegment **segment_p, bool set_readout);
void click_track_draw(ClickTrack *tt);

typedef struct project Project;
void click_track_destroy(ClickTrack *tt);

void click_track_mute_unmute(ClickTrack *t);
void click_track_increment_vol(ClickTrack *tt);
void click_track_decrement_vol(ClickTrack *tt);
void click_track_delete_segment_at_cursor(ClickTrack *ct);
/* void project_init_metronomes(Project *proj); */
void session_init_metronomes(Session *session);
void session_destroy_metronomes(Session *session);
void click_track_mix_metronome(ClickTrack *tt, float *mixdown_buf, int32_t mixdown_buf_len, int32_t tl_start_pos_sframes, int32_t tl_end_pos_sframes, float step);
bool click_track_triage_click(uint8_t button, ClickTrack *t);


void click_track_fprint(FILE *f, ClickTrack *tt);
void click_segment_at_cursor_fprint(FILE *f, Timeline *tl);

ClickSegment *click_track_add_segment(ClickTrack *t, int32_t start_pos, int16_t num_measures, int bpm, uint8_t num_beats, uint8_t *subdiv_lens);

#endif
