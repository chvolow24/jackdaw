/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
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
#define MAX_TEMPO_TRACKS 16
#define TEMPO_POS_STR_LEN 32
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

typedef struct tempo_track TempoTrack;
typedef struct tempo_segment TempoSegment;
typedef struct tempo_segment {
    TempoTrack *track;
    int32_t start_pos;
    int32_t end_pos;
    int16_t first_measure_index;
    int32_t num_measures;
    TempoTrack *tempo_track;
    MeasureConfig cfg;

    TempoSegment *next;
    TempoSegment *prev;
} TempoSegment;

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
typedef struct tempo_track {
    char name[255];
    TempoSegment *segments;
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

    char pos_str[TEMPO_POS_STR_LEN];
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
    
} TempoTrack;


/* Timeline interface */
TempoTrack *timeline_add_tempo_track(Timeline *tl);
void timeline_cut_tempo_track_at_cursor(Timeline *tl);
void timeline_increment_tempo_at_cursor(Timeline *tl, int inc_by);
void timeline_goto_prox_beat(Timeline *tl, int direction, enum beat_prominence bp);
void timeline_tempo_track_set_tempo_at_cursor(Timeline *tl);
void timeline_tempo_track_edit(Timeline *tl);
bool timeline_tempo_track_delete(Timeline *tl);


/* Required in settings.c */
TempoSegment *tempo_track_get_segment_at_pos(TempoTrack *t, int32_t pos);
void tempo_segment_set_config(TempoSegment *s, int num_measures, int bpm, uint8_t num_beats, uint8_t *subdivs, enum ts_end_bound_behavior ebb);
void tempo_segment_destroy(TempoSegment *s);
    /* tempo_segment_set_config(s, -1, cpy->cfg.bpm, cpy->cfg.num_beats, cpy->cfg.beat_subdiv_lens, ebb); */
    /* tempo_segment_destroy(cpy); */

    
/*********************/
/* void tempo_track_reset(TempoTrack *tt); */
/* TempoSegment *tempo_track_add_segment(TempoTrack *t, int32_t start_pos, int16_t num_measures, int bpm, uint8_t num_beats, uint8_t *subdiv_lens); */
void tempo_segment_fprint(FILE *f, TempoSegment *s);

int32_t tempo_track_bar_beat_subdiv(TempoTrack *tt, int32_t pos, int *bar_p, int *beat_p, int *subdiv_p, TempoSegment **segment_p, bool set_readout);
void tempo_track_draw(TempoTrack *tt);
/* void tempo_track_edit_segment_at_cursor(TempoTrack *tt, int num_measures, int bpm, uint8_t num_beats, uint8_t *subdiv_lens); */
typedef struct project Project;
void tempo_track_destroy(TempoTrack *tt);

void tempo_track_mute_unmute(TempoTrack *t);
void tempo_track_increment_vol(TempoTrack *tt);
void tempo_track_decrement_vol(TempoTrack *tt);
void project_init_metronomes(Project *proj);
void project_destroy_metronomes(Project *proj);
void tempo_track_mix_metronome(TempoTrack *tt, float *mixdown_buf, int32_t mixdown_buf_len, int32_t tl_start_pos_sframes, int32_t tl_end_pos_sframes, float step);
bool tempo_track_triage_click(uint8_t button, TempoTrack *t);


void tempo_track_fprint(FILE *f, TempoTrack *tt);
void timeline_segment_at_cursor_fprint(FILE *f, Timeline *tl);

#endif
