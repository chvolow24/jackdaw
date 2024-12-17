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
    tempo.h

    * Define structs related to tempo tracks, bars, and beats
 *****************************************************************************************************************/

#ifndef JDAW_TEMPO_H
#define JDAW_TEMPO_H

#include <stdint.h>
#include "components.h"
#include "layout.h"
#include "textbox.h"

#define MAX_BEATS_PER_BAR 16
#define MAX_TEMPO_TRACKS 16
#define TEMPO_POS_STR_LEN 32
#define BARS_FOR_NOTHING 2

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

typedef struct tempo_track {
    TempoSegment *segments;
    uint8_t index;
    /* uint8_t num_segments; */
    uint8_t current_segment;

    Metronome *metronome;
    float metronome_vol;
    /* Button *metronome_button; */
    Textbox *metronome_button;
    Textbox * edit_button;
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
} TempoTrack;


TempoTrack *timeline_add_tempo_track(Timeline *tl);
TempoSegment *tempo_track_add_segment(TempoTrack *t, int32_t start_pos, int16_t num_measures, int bpm, int num_beats, ...);
void tempo_segment_fprint(FILE *f, TempoSegment *s);

void tempo_track_draw(TempoTrack *tt);
int32_t tempo_track_bar_beat_subdiv(TempoTrack *tt, int32_t pos, int *bar_p, int *beat_p, int *subdiv_p, TempoSegment **segment_p, bool set_readout);

typedef struct project Project;


void tempo_track_mute_unmute(TempoTrack *t);
void tempo_track_increment_vol(TempoTrack *tt);
void tempo_track_decrement_vol(TempoTrack *tt);
void project_init_metronomes(Project *proj);
void tempo_track_mix_metronome(TempoTrack *tt, float *mixdown_buf, int32_t mixdown_buf_len, int32_t tl_start_pos_sframes, int32_t tl_end_pos_sframes, float step);
bool tempo_track_triage_click(uint8_t button, TempoTrack *t);


void tempo_track_fprint(FILE *f, TempoTrack *tt);
#endif
