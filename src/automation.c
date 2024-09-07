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
    automation.c

    * All operations related to track automation
 *****************************************************************************************************************/

#include <stdlib.h>

#include "automation.h"
#include "layout.h"
#include "project.h"
#include "timeline.h"
#include "value.h"

extern Window *main_win;

void track_init_default_automations(Track *track)
{
    Automation a = track->vol_automation;
    a.track = track;
    a.type = AUTO_VOL;
    a.min.float_v = 0.0f;
    a.max.float_v = 1.0f;
    a.range.float_v = 1.0f;

    a = track->pan_automation;
    a.track = track;
    a.type = AUTO_PAN;
    a.min.float_v = 0.0f;
    a.min.float_v = 1.0f;
    a.range.float_v = 1.0f;
}

Automation *track_add_automation(Track *track, AutomationType type)
{
    Automation *a = calloc(1, sizeof(Automation));
    a->track = track;
    a->type = type;
    /* Layout *lt = layout_add_child(track->layout->parent); */
    Layout *lt = layout_create();
    lt->w.type = SCALE;
    lt->w.value.floatval = 1.0;
    lt->h.type = ABS;
    lt->h.value.intval = 70;
    lt->y.type = STACK;
    a->layout = lt;
    /* layout_write(stdout, track->layout, 0); */
    /* exit(0); */
    layout_insert_child_at(lt, track->layout->parent, track->layout->index + 1);
    layout_reset(lt->parent);
    /* layout_write(stdout, lt->parent, 0); */
    switch (type) {
    case AUTO_VOL:
	a->val_type = JDAW_FLOAT;
	a->target_val = &track->vol;
    case AUTO_PAN:
	a->val_type = JDAW_FLOAT;
	a->target_val = &track->pan;
	break;
    case AUTO_FIR_FILTER_CUTOFF:
	a->val_type = JDAW_DOUBLE;
	a->target_val = &track->fir_filter->cutoff_freq;
	break;
    case AUTO_FIR_FILTER_BANDWIDTH:
	a->val_type = JDAW_DOUBLE;
	a->target_val = &track->fir_filter->bandwidth;
	break;
    case AUTO_DEL_TIME:
	a->val_type = JDAW_INT32;
	a->target_val = &track->delay_line.len;
    case AUTO_DEL_AMP:
	a->val_type = JDAW_DOUBLE;
	a->target_val = &track->delay_line.amp;
    case AUTO_DEL_STEREO_OFFSET:
	a->val_type = JDAW_INT32;
	a->target_val = &track->delay_line.stereo_offset;
    default:
	break;
    }
    return a;
}

static void keyframe_recalculate_m(Keyframe *k, ValType type)
{
    if (!k->next) return;
    int32_t dx = k->next->pos - k->pos;
    Value dy = jdaw_val_sub(k->next->value, k->value, type);
    k->m_fwd = JDAW_VAL_CAST(dy, type, double) / (double)dx;
}

Keyframe *automation_insert_keyframe_after(
    Automation *automation,
    Keyframe *insert_after,
    Value val,
    int32_t pos)
{
    Keyframe *k = calloc(1, sizeof(Keyframe));
    k->value = val;
    if (insert_after) {
	Keyframe *next = insert_after->next;
	insert_after->next = k;
	k->prev = insert_after;
	keyframe_recalculate_m(insert_after, automation->val_type);
	if (next) {
	    k->next = next;
	    keyframe_recalculate_m(k, automation->val_type);
	} else {
	    automation->last = k;
	}
	
    } else {
	automation->first = k;
	automation->last = k;
    }
    return k;
}

Keyframe *automation_insert_keyframe_before(
    Automation *automation,
    Keyframe *insert_before,
    Value val,
    int32_t pos)
{
    Keyframe *k = calloc(1, sizeof(Keyframe));
    k->value = val;
    if (insert_before) {
	Keyframe *prev = insert_before->prev;
	k->next = insert_before;
	insert_before->prev = k;
	prev->next = k;
	keyframe_recalculate_m(k, automation->val_type);
	if (prev) {
	    k->prev = prev;
	    keyframe_recalculate_m(prev, automation->val_type);
	} else {
	    automation->first = k;
	}
	
    } else {
	automation->first = k;
	automation->last = k;
    }
    return k;
}

Keyframe *automation_get_segment(Automation *a, int32_t at)
{
    Keyframe *k = a->first;
    while (k && k->pos < at) {
	if (k->next) 
	    k = k->next;
	else break;
    }
    if (k && k->pos > at && k->prev)
	return k->prev;
    else return k;
}


static inline bool offscreen_left(int x)
{
    return x < 0;
}
static inline bool offscreen_right(int x)
{
    return x > main_win->w_pix;
}
static inline bool onscreen(int x)
{
    return (x > 0 && x < main_win->w_pix);
}

static inline bool segment_intersects_screen(int a, int b)
{
    return (offscreen_left(a) && offscreen_right(b)) || onscreen(a) || onscreen(b);
}
Automation *automation_draw(Automation *a)
{
    Keyframe *k = a->first;
    while (k && k->next) {
	int k_pos = timeline_get_draw_x(k->pos);
	int next_pos = timeline_get_draw_x(k->next->pos);
	if (segment_intersects_screen(k_pos, next_pos)) {
	    int y_left, y_right;
	    Value v = jdaw_val_sub(k->value, a->min, a->val_type);
	    double scaled = jdaw_val_div_double(v, a->range, a->val_type);
	    y_left = scaled * a->layout->rect.w;
	    v = jdaw_val_sub(k->next->value, a->min, a->val_type);
	    scaled = jdaw_val_div_double(v, a->range, a->val_type);
	    y_right = scaled * a->layout->rect.w;
	    SDL_RenderDrawLine(main_win->rend, k_pos, y_left, next_pos, y_right);
	    
	}
    }
}

