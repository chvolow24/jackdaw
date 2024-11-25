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
#include "layout.h"

#define MAX_BEATS_PER_BAR 16
#define MAX_TEMPO_TRACKS 16

typedef struct measure_config {
    int bpm;
    int32_t dur_sframes;
    uint8_t num_beats;
    uint8_t beat_subdiv_lens[MAX_BEATS_PER_BAR];
    uint8_t num_atoms;
} MeasureConfig;

typedef struct tempo_track TempoTrack;
typedef struct tempo_segment TempoSegment;
typedef struct tempo_segment {
    TempoTrack *track;
    int32_t start_pos;
    int32_t end_pos;
    uint16_t first_measure_index; /* 0 if first segment, else sum of previous segment lengths */
    /* int16_t num_measures; */
    TempoTrack *tempo_track;
    MeasureConfig cfg;

    TempoSegment *next;
    TempoSegment *prev;
} TempoSegment;

typedef struct timeline Timeline;

typedef struct tempo_track {
    TempoSegment *segments;
    /* uint8_t num_segments; */
    uint8_t current_segment;

    float *metronome_buffers[2];
    uint16_t metronome_buffer_lens[2];
    
    Timeline *tl;
    Layout *layout;
} TempoTrack;


TempoTrack *timeline_add_tempo_track(Timeline *tl);
TempoSegment *tempo_track_add_segment(TempoTrack *t, int32_t start_pos, int16_t num_measures, int bpm, int num_beats, ...);
void tempo_segment_fprint(FILE *f, TempoSegment *s);

void tempo_track_draw(TempoTrack *tt);

#endif
