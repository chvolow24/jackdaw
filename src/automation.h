/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

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
#include "test.h"
#include "textbox.h"
#include "value.h"

#define MAX_TRACK_AUTOMATIONS 8
/* #define KCLIPS_ARR_INITLEN 4 */
#define KEYFRAMES_ARR_INITLEN 4

typedef struct track Track;
typedef struct timeline Timeline;

typedef struct automation Automation;
typedef struct keyframe Keyframe;
typedef struct endpoint Endpoint;
/* typedef struct keyframe_clip KClip; */

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
    AUTO_PLAY_SPEED = 6,
    AUTO_ENDPOINT = 7
    
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
    
    /* KClip *kclip; /\* Optional; used for replication of envelopes *\/ */
    /* int32_t pos_rel; /\* Position relative to KClip start *\/ */
} Keyframe;

/* typedef struct keyframe_clipref KClipRef; */

typedef struct automation {
    char name[MAX_NAMELENGTH];
    Track *track;
    int index;
    AutomationType type;
    ValType val_type;
    Value max;
    Value min;
    Value range;
    void *target_val;

    Endpoint *endpoint;
    
    bool read;
    bool write;

    pthread_mutex_t lock; /* TODO: be more specific; what is this for? */
    pthread_mutex_t keyframe_arr_lock; /* Protect ALL keyframes in DSP ops in case of realloc */

    Keyframe *keyframes;
    uint16_t num_keyframes;
    uint16_t keyframe_arrlen;
    Keyframe *current;
    /* int32_t current; */
    /* Keyframe *first; */
    /* Keyframe *last; */
    /* Keyframe *current; */

    Keyframe *undo_cache;
    Value undo_cache_len;
    
    /* Write cache */
    int32_t record_start_pos;
    bool ghost_valid;
    int32_t ghost_pos;
    Value ghost_val;
    bool changing;
    double m_diff_prop_cum;

    /* KClipRef *kclips; */
    /* uint16_t num_kclips; */
    /* uint16_t kclips_arr_len; */
    bool shown;
    Textbox *label;

    bool deleted;
    bool removed;
    
    Label *keyframe_label;
    
    Button *read_button;
    Button *write_button;
    Layout *layout;
    SDL_Rect *console_rect;
    SDL_Rect *bckgrnd_rect;
    SDL_Rect *color_bar_rect;
} Automation;

/* typedef struct keyframe_clip { */
/*     Keyframe *first; */
/*     uint16_t len; */
/* } KClip; */

/* typedef struct keyframe_clipref { */
/*     KClip *kclip; */
/*     int32_t pos; */
/*     bool home; */
/*     Automation *automation; */
/*     Keyframe *first; */
/*     Keyframe *last; */
/* } KClipRef; */



/* Automation *track_add_automation(Track *track, AutomationType type); */
void automation_clear_cache(Automation *a);

/* High-level function opens a modal window for user to select automation type */
void track_add_new_automation(Track *track);

/* To prompt user for automation type, use "track_add_new_automation" instead */
Automation *track_add_automation(Track *track, AutomationType type);
Automation *track_add_automation_from_endpoint(Track *track, Endpoint *ep);

    
Value automation_get_value(Automation *a, int32_t pos, float direction);
void automation_get_range(Automation *a, void *dst, int dst_len, int32_t start_pos, float step);
void automation_draw(Automation *a);
Keyframe *automation_insert_keyframe_at(
    Automation *a,
    int32_t pos,
    Value val);

void automation_remove(Automation *a);
void automation_reinsert(Automation *a);
void automation_delete(Automation *a);
void automation_destroy(Automation *a);
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
void automation_do_write(Automation *a, Value v, int32_t pos, int32_t end_pos, float step);
void automation_reset_keyframe_x(Automation *a);
/* Keyframe *automation_get_segment(Automation *a, int32_t at); */

void automation_show(Automation *a);
void automation_hide(Automation *a);
void track_automations_show_all(Track *track);
void track_automations_hide_all(Track *track);

void automation_unset_dragging_kf(Timeline *tl);
bool automation_triage_click(uint8_t button, Automation *a);
bool automations_triage_motion(Timeline *tl, int xrel, int yrel);
bool automation_record(Automation *a);
bool automation_handle_delete(Automation *a);
bool automation_toggle_read(Automation *a);
/* void keyframe_delete(Keyframe *k); */
void keyframe_delete(Keyframe *k);
int track_select_next_automation(Track *t);
int track_select_prev_automation(Track *t);
int track_select_first_shown_automation(Track *t);
int track_select_last_shown_automation(Track *t);

TEST_FN_DECL(track_automation_order, Track *track);
TEST_FN_DECL(automation_keyframe_order, Automation *a);
TEST_FN_DECL(automation_index, Automation *a);

typedef struct endpoint Endpoint;
void automation_endpoint_write(Endpoint *ep, Value val, int32_t play_pos);
#endif
