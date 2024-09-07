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
#include "color.h"
#include "layout.h"
#include "layout_xml.h"
#include "project.h"
#include "timeline.h"
#include "value.h"

#define KEYFRAME_DIAG 21
#define AUTOMATION_LT_H 70

#ifndef INSTALL_DIR
#define INSTALL_DIR "."
#endif

#define AUTOMATION_LT_FILEPATH INSTALL_DIR "/assets/layouts/track_automation.xml"

extern Window *main_win;
extern SDL_Color console_bckgrnd;
extern SDL_Color control_bar_bckgrnd;
/* SDL_Color automation_bckgrnd = {0, 25, 26, 255}; */
SDL_Color automation_bckgrnd = {90, 100, 120, 255};

const char *AUTOMATION_LABELS[] = {
    "Volume",
    "Pan (L->R)",
    "FIR filter cutoff",
    "FIR filter bandwidth",
    "Delay time",
    "Delay amplitude",
    "Delay stereo offset"
};

/* void track_init_default_automations(Track *track) */
/* { */
/*     Automation a = track->vol_automation; */
/*     a.track = track; */
/*     a.type = AUTO_VOL; */
/*     a.min.float_v = 0.0f; */
/*     a.max.float_v = 1.0f; */
/*     a.range.float_v = 1.0f; */

/*     a = track->pan_automation; */
/*     a.track = track; */
/*     a.type = AUTO_PAN; */
/*     a.min.float_v = 0.0f; */
/*     a.min.float_v = 1.0f; */
/*     a.range.float_v = 1.0f; */
/* } */

Automation *track_add_automation(Track *track, AutomationType type)
{
    Automation *a = calloc(1, sizeof(Automation));
    a->track = track;
    a->type = type;
    a->read = true;
    track->automations[track->num_automations] = a;
    track->num_automations++;
    /* Layout *lt = layout_add_child(track->layout->parent); */

    /* layout_write(stdout, lt->parent, 0); */
    switch (type) {
    case AUTO_VOL:
	a->val_type = JDAW_FLOAT;
	a->min.float_v = 0.0f;
	a->max.float_v = TRACK_VOL_MAX;
	a->range.float_v = TRACK_VOL_MAX;
	a->target_val = &track->vol;
	break;
    case AUTO_PAN:
	a->val_type = JDAW_FLOAT;
	a->min.float_v = 0.0f;
	a->max.float_v = 1.0f;
	a->range.float_v = 1.0f;
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

void track_automations_show_all(Track *track)
{
    for (uint8_t i=0; i<track->num_automations; i++) {
	Automation *a = track->automations[i];
	a->shown = true;
	if (!a->layout) {
	    Layout *lt = layout_read_from_xml(AUTOMATION_LT_FILEPATH);
	    a->layout = lt;
	    Layout *console = layout_get_child_by_name_recursive(lt, "automation_console");
	    a->console_rect = &console->rect;
	    a->bckgrnd_rect = &lt->rect;
	    Layout *color_bar = layout_get_child_by_name_recursive(lt, "colorbar");
	    a->color_bar_rect = &color_bar->rect;
	    Layout *main = layout_get_child_by_name_recursive(lt, "main");
	    a->bckgrnd_rect = &main->rect;
	/* layout_write(stdout, track->layout, 0); */
	/* exit(0); */
	    layout_insert_child_at(lt, track->layout->parent, track->layout->index + 1);
	} else {
	    a->layout->h.value.intval = AUTOMATION_LT_H;
	}
    }
    timeline_reset(track->tl);
}

void track_automations_hide_all(Track *track)
{
    for (uint8_t i=0; i<track->num_automations; i++) {
	Automation *a = track->automations[i];
	a->shown = false;
	Layout *lt = a->layout;
	if (lt) {
	    lt->h.value.intval = 0;
	}
    }
    timeline_reset(track->tl);
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
    k->pos = pos;
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
    Keyframe *print = automation->first;
    while (print) {
	fprintf(stdout, "keyframe at pos: %d\n", print->pos);
	print = print->next;
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

static inline bool segment_intersects_screen(int a, int b)
{
    return (b >= 0 && main_win->w_pix >= a);
   
}

static inline bool x_onscreen(int x)
{
    return (x > 0 && x < main_win->w_pix);
}

static inline void keyframe_draw(int x, int y)
{
    int half_diag = KEYFRAME_DIAG / 2;
    x -= half_diag;
    int y1 = y;
    int y2 = y;
    for (int i=0; i<KEYFRAME_DIAG; i++) {
	SDL_RenderDrawLine(main_win->rend, x, y1, x, y2);
	if (i < half_diag) {
	    y1--;
	    y2++;
	} else {
	    y1++;
	    y2--;
	}
	x++;
    }

}

void automation_draw(Automation *a)
{
    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(automation_bckgrnd));
    SDL_RenderFillRect(main_win->rend, a->bckgrnd_rect);
    
    Keyframe *k = a->first;

    bool set_k = true;
    int bottom_y = a->layout->rect.y + a->layout->rect.h;
    int y_left = 0;
    int y_right = 0;
    int k_pos;
    int next_pos;
    SDL_SetRenderDrawColor(main_win->rend, 255, 255, 255, 255);
    /* SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(a->track->color)); */
    while (k) {
	if (set_k) {
	    k_pos = timeline_get_draw_x(k->pos);
	} else {
	    k_pos = next_pos;
	}
	/* fprintf(stdout, "DRAWING k pos: %d\n", k_pos); */
	if (x_onscreen(k_pos)) {
	    if (set_k) {
		Value v = jdaw_val_sub(k->value, a->min, a->val_type);
	        double scaled = jdaw_val_div_double(v, a->range, a->val_type);
	        y_left = bottom_y -  scaled * a->layout->rect.h;
	    } else {
		y_left = y_right;
	    }
	    keyframe_draw(k_pos, y_left);
	}
	if (k->next) {
	    next_pos = timeline_get_draw_x(k->next->pos);
	    if (segment_intersects_screen(k_pos, next_pos)) {
		Value v = jdaw_val_sub(k->next->value, a->min, a->val_type);
		double scaled = jdaw_val_div_double(v, a->range, a->val_type);
		y_right = bottom_y -  scaled * a->layout->rect.h;
		SDL_RenderDrawLine(main_win->rend, k_pos, y_left, next_pos, y_right); 
	    }
	}
	set_k = false;
	k = k->next;
    }

    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(a->track->color));
    SDL_RenderFillRect(main_win->rend, a->color_bar_rect);

    /* SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(console_bckgrnd)); */
    /* SDL_RenderFillRect(main_win->rend, a->console_rect); */

}

