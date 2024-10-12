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
    automation.h

    * Define types related to parameter automation
    * Provide an interface for writing and reading automations
 *****************************************************************************************************************/


#ifndef JDAW_AUTOMATION_H
#define JDAW_AUTOMATION_H

#include "components.h"
#include "layout.h"
#include "textbox.h"
#include "value.h"

#define MAX_TRACK_AUTOMATIONS 8
#define KCLIPS_ARR_INITLEN 4
#define KEYFRAMES_ARR_INITLEN 4

typedef struct track Track;
typedef struct timeline Timeline;

typedef struct automation Automation;
typedef struct keyframe Keyframe;
typedef struct keyframe_clip KClip;

typedef struct automation_slope {
    int32_t dx;
    Value dy;
} AutomationSlope;


typedef enum automation_type {
    AUTO_VOL = 0,
    AUTO_PAN = 1,
    AUTO_FIR_FILTER_CUTOFF = 2,
    AUTO_FIR_FILTER_BANDWIDTH = 3,
    AUTO_DEL_TIME = 4,
    AUTO_DEL_AMP = 5,
    AUTO_PLAY_SPEED = 6
} AutomationType;

typedef struct keyframe {
    Automation *automation;
    int32_t pos;
    Value value;
    AutomationSlope m_fwd;
    /* double m_fwd; /\* slope (change in value per sample) *\/ */

    /* Keyframe *prev; */
    /* Keyframe *next; */
    double draw_y_prop;
    int draw_x;
    
    KClip *kclip; /* Optional; used for replication of envelopes */
    int32_t pos_rel; /* Position relative to KClip start */
} Keyframe;

typedef struct keyframe_clipref KClipRef;
typedef struct automation {
    Track *track;
    int index;
    AutomationType type;
    ValType val_type;
    Value max;
    Value min;
    Value range;
    void *target_val;
    
    bool read;
    bool write;
    

    Keyframe *keyframes;
    uint16_t num_keyframes;
    uint16_t keyframe_arrlen;
    Keyframe *current;
    /* int32_t current; */
    /* Keyframe *first; */
    /* Keyframe *last; */
    /* Keyframe *current; */

    /* Write cache */
    bool ghost_valid;
    int32_t ghost_pos;
    Value ghost_val;
    bool changing;

    KClipRef *kclips;
    uint16_t num_kclips;
    uint16_t kclips_arr_len;
    
    bool shown;
    Textbox *label;
    
    Label *keyframe_label;
    
    Button *read_button;
    Button *write_button;
    Layout *layout;
    SDL_Rect *console_rect;
    SDL_Rect *bckgrnd_rect;
    SDL_Rect *color_bar_rect;
} Automation;

typedef struct keyframe_clip {
    Keyframe *first;
    Keyframe *last;
} KClip;

typedef struct keyframe_clipref {
    KClip *kclip;
    int32_t pos;
    bool home;
    Automation *automation;
    Keyframe *first;
    Keyframe *last;
} KClipRef;

Automation *track_add_automation(Track *track, AutomationType type);
void automation_clear_cache(Automation *a);
void track_add_new_automation(Track *track);
Value automation_get_value(Automation *a, int32_t pos, float direction);
void automation_draw(Automation *a);
Keyframe *automation_insert_keyframe_at(
    Automation *a,
    int32_t pos,
    Value val);
/* Keyframe *automation_insert_keyframe_after( */
/*     Automation *automation, */
/*     Keyframe *insert_after, */
/*     Value val, */
/*     int32_t pos); */
/* Keyframe *automation_insert_keyframe_before( */
/*     Automation *automation, */
/*     Keyframe *insert_before, */
/*     Value val, */
/*     int32_t pos); */
/* Keyframe *automation_insert_maybe( */
/*     Automation *a, */
/*     /\* Keyframe *insert_after, *\/ */
/*     Value val, */
/*     int32_t pos, */
/*     int32_t chunk_end_pos, */
/*     float direction); */
void automation_do_write(Automation *a, int32_t pos, int32_t end_pos, float step);
void automation_reset_keyframe_x(Automation *a);
/* Keyframe *automation_get_segment(Automation *a, int32_t at); */
void track_automations_show_all(Track *track);
void track_automations_hide_all(Track *track);

bool automation_triage_click(uint8_t button, Automation *a);
bool automations_triage_motion(Timeline *tl);
void automation_record(Automation *a);
#endif