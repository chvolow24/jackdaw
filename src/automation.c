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

#include <pthread.h>
#include <stdlib.h>

#include "automation.h"
#include "color.h"
#include "input.h"
#include "layout.h"
#include "layout_xml.h"
#include "modal.h"
#include "project.h"
#include "status.h"
#include "symbol.h"
#include "timeline.h"
#include "transport.h"
#include "value.h"

#define KEYFRAME_DIAG 21
#define AUTOMATION_LT_H 60
#define AUTOMATION_LT_Y 4

#ifndef INSTALL_DIR
#define INSTALL_DIR "."
#endif

#define AUTOMATION_LT_FILEPATH INSTALL_DIR "/assets/layouts/track_automation.xml"

extern Window *main_win;
extern Project *proj;

extern Symbol *SYMBOL_TABLE[];

extern SDL_Color console_bckgrnd;
extern SDL_Color control_bar_bckgrnd;
extern SDL_Color color_mute_solo_grey;
extern SDL_Color color_global_black;
extern SDL_Color color_global_white;
extern SDL_Color color_global_grey;
extern SDL_Color color_global_red;
extern SDL_Color color_global_play_green;
extern SDL_Color color_global_light_grey;
extern SDL_Color color_global_dropdown_green;
extern SDL_Color color_global_quickref_button_blue;
/* SDL_Color automation_bckgrnd = {0, 25, 26, 255}; */
SDL_Color automation_console_bckgrnd = {110, 110, 110, 255};
SDL_Color automation_bckgrnd = {90, 100, 120, 255};

const char *AUTOMATION_LABELS[] = {
    "Volume",
    "Pan",
    "Filter cutoff",
    "Filter bandwidth",
    "Delay time",
    "Delay amplitude",
    "Play speed"
};

/* void automation_clear_cache(Automation *a) */
/* { */
    
/*     /\* fprintf(stderr, "AUTOMATION CLEAR CACHE\n"); *\/ */
/*     /\* a->current = NULL; *\/ */
/*     /\* a->ghost_valid = false; *\/ */
/*     /\* a->changing = true; *\/ */
/* } */


Automation *track_add_automation(Track *track, AutomationType type)
{
    Automation *a = calloc(1, sizeof(Automation));
    a->track = track;
    a->type = type;
    a->read = true;
    a->index = track->num_automations;
    a->keyframe_arrlen = KEYFRAMES_ARR_INITLEN;
    a->keyframes = calloc(a->keyframe_arrlen, sizeof(Keyframe));

    int ret;
    if ((ret = pthread_mutex_init(&a->lock, NULL) != 0)) {
	fprintf(stderr, "Error initializing lock: %s\n", strerror(ret));
	exit(1);
    }
    
    a->kclips_arr_len = KCLIPS_ARR_INITLEN;
    a->kclips = calloc(KCLIPS_ARR_INITLEN, sizeof(KClipRef));
    track->automations[track->num_automations] = a;
    track->num_automations++;
    Value base_kf_val;
    switch (type) {
    case AUTO_VOL:
	a->val_type = JDAW_FLOAT;
	a->min.float_v = 0.0f;
	a->max.float_v = TRACK_VOL_MAX;
	a->range.float_v = TRACK_VOL_MAX;
	a->target_val = &track->vol;
	base_kf_val.float_v = track->vol;
	automation_insert_keyframe_at(a, 0, base_kf_val);
	break;
    case AUTO_PAN:
	a->val_type = JDAW_FLOAT;
	a->min.float_v = 0.0f;
	a->max.float_v = 1.0f;
	a->range.float_v = 1.0f;
	a->target_val = &track->pan;
	base_kf_val.float_v = track->pan;
	automation_insert_keyframe_at(a, 0, base_kf_val);
	/* automation_insert_keyframe_at(a, NULL, base_kf_val, 0); */
	break;
    case AUTO_FIR_FILTER_CUTOFF:
	if (!track->fir_filter) {
	    track_add_default_filter(track);
	}
	a->val_type = JDAW_DOUBLE;
	a->target_val = &track->fir_filter->cutoff_freq_unscaled;
	a->min.double_v = 0.0;
	a->max.double_v = 1.0;
	a->range.double_v = 1.0;
	base_kf_val.double_v = 1.0;
	automation_insert_keyframe_at(a, 0, base_kf_val);
	/* automation_insert_keyframe_after(a, NULL, base_kf_val, 0); */
	break;
    case AUTO_FIR_FILTER_BANDWIDTH:
	if (!track->fir_filter) {
	    track_add_default_filter(track);
	}
	a->val_type = JDAW_DOUBLE;
	a->target_val = &track->fir_filter->bandwidth_unscaled;
	a->min.double_v = 0.0;
	a->max.double_v = 1.0;
	a->range.double_v = 1.0;
	base_kf_val.double_v = 0.5;
	automation_insert_keyframe_at(a, 0, base_kf_val);
	/* automation_insert_keyframe_after(a, NULL, base_kf_val, 0); */
	break;
    case AUTO_DEL_TIME:
	if (!track->delay_line.buf_L) delay_line_init(&track->delay_line);
	a->val_type = JDAW_INT32;
	a->min.int32_v = 10;
	a->max.int32_v = proj->sample_rate * DELAY_LINE_MAX_LEN_S;
	a->range.int32_v = a->max.int32_v - 10;
	a->target_val = &track->delay_line.len;
	base_kf_val.int32_v = proj->sample_rate / 2;
	automation_insert_keyframe_at(a, 0, base_kf_val);
	/* automation_insert_keyframe_after(a, NULL, base_kf_val, 0); */

	break;
    case AUTO_DEL_AMP:
	a->val_type = JDAW_DOUBLE;
	a->target_val = &track->delay_line.amp;
	a->min.double_v = 0.0;
	a->max.double_v = 1.0;
	a->range.double_v = 1.0;
	base_kf_val.double_v = 0.5;
	automation_insert_keyframe_at(a, 0, base_kf_val);
	/* automation_insert_keyframe_after(a, NULL, base_kf_val, 0); */
	break;
    case AUTO_PLAY_SPEED:
	a->val_type = JDAW_FLOAT;
	a->min.float_v = 0.05;
	a->max.float_v = 5.0;
	a->range.float_v = 5.0 - 0.05;
	a->target_val = &proj->play_speed;
	base_kf_val.float_v = 1.0f;
	automation_insert_keyframe_at(a, 0, base_kf_val);
	/* automation_insert_keyframe_after(a, NULL, base_kf_val, 0); */
	break;
    default:
	break;
    }
    track->automation_dropdown->background_color = &color_global_dropdown_green;
    return a;
}

static int add_auto_form(void *mod_v, void *nullarg)
{
    Modal *modal = (Modal *)mod_v;
    ModalEl *el;
    AutomationType t = 0;
    Track *track = NULL;
    for (uint8_t i=0; i<modal->num_els; i++) {
	switch ((el = modal->els[i])->type) {
	case MODAL_EL_RADIO:
	    t = ((RadioButton *)el->obj)->selected_item;
	    track = ((RadioButton *)el->obj)->target;
	    break;
	default:
	    break;
	}
    }
    for (uint8_t i=0; i<track->num_automations; i++) {
	if (track->automations[i]->type == t) {
	    status_set_errstr("Track already has an automation of that type");
	    return 1;
	}
    }
    track_add_automation(track, t);
    track_automations_show_all(track);
    window_pop_modal(main_win);
    return 0;
}

void track_add_new_automation(Track *track)
{
    Layout *lt = layout_add_child(track->tl->proj->layout);
    layout_set_default_dims(lt);
    Modal *m = modal_create(lt);
    /* Automation *a = track_add_automation_internal(track, AUTO_VOL); */
    modal_add_header(m, "Add automation to track", &color_global_light_grey, 4);
    modal_add_radio(
	m,
	&color_global_light_grey,
	(void *)track,
	NULL,
	AUTOMATION_LABELS,
	sizeof(AUTOMATION_LABELS) / sizeof(const char *));
    modal_add_button(m, "Add", add_auto_form);
    window_push_modal(main_win, m);
    modal_reset(m);
    modal_move_onto(m);

}


static void keyframe_labelmaker(char *dst, size_t dstsize, void *target, ValType t)
{
    Automation *a = (Automation *)target;
    Timeline *tl = a->track->tl;
    Keyframe *k = tl->dragging_keyframe;
    if (!k) return;
    char valstr[dstsize];
    /* char tcstr[dstsize]; */
    switch(a->type) {
    case AUTO_VOL:
    case AUTO_DEL_AMP:
    case AUTO_PLAY_SPEED:
	jdaw_val_set_str(valstr, dstsize, k->value, a->val_type, 2);
    break;
    case AUTO_PAN: 
        label_pan(valstr, dstsize, k->value.float_v);
	break;
    case AUTO_FIR_FILTER_CUTOFF:
    case AUTO_FIR_FILTER_BANDWIDTH:
	label_freq_raw_to_hz(valstr, dstsize, k->value.double_v);
	break;
    case AUTO_DEL_TIME:
	label_time_samples_to_msec(valstr, dstsize, k->value.int32_v, proj->sample_rate);
	break;
    }
    /* timecode_str_at(tcstr, dstsize, k->pos); */
    snprintf(dst, dstsize, "%s", valstr);

}

bool automation_toggle_read(Automation *a)
{
    Track *track = a->track;
    a->read = !(a->read);
    if (a->read) {
	track->some_automations_read = true;
	textbox_set_background_color(a->read_button->tb, &color_global_play_green);
	textbox_set_text_color(a->read_button->tb, &color_global_white);
    } else {
	textbox_set_background_color(a->read_button->tb, &color_global_grey);
	for (uint8_t i=0; i<track->num_automations; i++) {
	    if (track->automations[i]->read) return 0;
	}
	track->some_automations_read = false;
    }
    return 0;
}

static void automation_set_write(Automation *a, bool to)
{
    a->write = to;
    if (a->write) {
	textbox_set_background_color(a->write_button->tb, &color_global_red);
	/* textbox_set_text_color(a->write_button->tb, &color_global_white); */
    } else {
	/* textbox_set_background_color(a->write_button->tb, &color_global_quickref_button_blue); */
	textbox_set_background_color(a->write_button->tb, &color_global_grey);
	/* textbox_set_text_color(a->write_button->tb, &color_global_black); */
    }
}

bool automation_toggle_write(Automation *a)
{
    /* a->write = !(a->write); */
    automation_set_write(a, !a->write);
    return a->write;
}

static int button_read_action(void *self_v, void *xarg)
{
    Automation *a = (Automation *)xarg;
    automation_toggle_read(a);
    return 0;
}

static int button_write_action(void *self_v, void *xarg)
{
    Automation *a = (Automation *)xarg;
    automation_toggle_write(a);
    return 0;
}

void automation_show(Automation *a)
{
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
	layout_insert_child_at(lt, a->track->layout, a->track->layout->num_children);
	layout_size_to_fit_children_v(a->track->layout, true, 0);

	Layout *tb_lt = layout_get_child_by_name_recursive(lt, "label");
	layout_reset(tb_lt);

	a->label = textbox_create_from_str(
	    AUTOMATION_LABELS[a->type],
	    tb_lt,
	    main_win->mono_bold_font,
	    12,
	    main_win);
	/* a->label->corner_radius = BUBBLE_CORNER_RADIUS; */
	textbox_set_trunc(a->label, false);
	textbox_set_background_color(a->label,  &color_global_quickref_button_blue);
	textbox_set_text_color(a->label, &color_global_white);
	/* textbox_set_border(a->label, &color_global_white, 1); */
	a->label->corner_radius = BUTTON_CORNER_RADIUS;
	textbox_set_pad(a->label, 4, 0);
	textbox_size_to_fit(a->label, 6, 2);
	textbox_reset_full(a->label);

	
	tb_lt = layout_get_child_by_name_recursive(lt, "read");
	Button *button = button_create(
	    tb_lt,
	    "Read",
	    button_read_action,
	    (void *)a,
	    main_win->mono_bold_font,
	    12,
	    &color_global_white,
	    &color_global_play_green
	    );
        textbox_set_border(button->tb, &color_global_black, 1);
	button->tb->corner_radius = MUTE_SOLO_BUTTON_CORNER_RADIUS;
	a->read_button = button;
	
	tb_lt = layout_get_child_by_name_recursive(lt, "write");
	
	button = button_create(
	    tb_lt,
	    "Write",
	    button_write_action,
	    (void *)a,
	    main_win->mono_bold_font,
	    12,
	    &color_global_white,
	    &color_global_grey
	    );
	textbox_set_border(button->tb, &color_global_black, 1);
	button->tb->corner_radius = MUTE_SOLO_BUTTON_CORNER_RADIUS;
	a->write_button = button;
	a->keyframe_label = label_create(0, a->layout, keyframe_labelmaker, a, 0, main_win);
    } else {
	a->layout->h.value.intval = AUTOMATION_LT_H;
	a->layout->y.value.intval = AUTOMATION_LT_Y;
    }
    layout_reset(a->layout);
    layout_size_to_fit_children_v(a->track->layout, true, 0);
    timeline_rectify_track_area(a->track->tl);

}


void track_automations_show_all(Track *track)
{
    if (track->num_automations == 0) return;
    for (uint8_t i=0; i<track->num_automations; i++) {
	automation_show(track->automations[i]);
    }
    track->some_automations_shown = true;
    track->automation_dropdown->symbol = SYMBOL_TABLE[SYMBOL_DROPUP];
    timeline_reset(track->tl, false);
}

void track_automations_hide_all(Track *track)
{
    for (uint8_t i=0; i<track->num_automations; i++) {
	Automation *a = track->automations[i];
	a->shown = false;
	Layout *lt = a->layout;
	if (lt) {
	    lt->h.value.intval = 0;
	    lt->y.value.intval = 0;
	    layout_reset(lt);
	}
    }
    track->automation_dropdown->symbol = SYMBOL_TABLE[SYMBOL_DROPDOWN];
    layout_size_to_fit_children_v(track->layout, true, 0);
    timeline_reset(track->tl, false);
}

static void keyframe_recalculate_m(Automation *a, int16_t keyframe_i)
{
    Keyframe *k = a->keyframes + keyframe_i;
    Keyframe *next = NULL;
    if (keyframe_i + 1 < a->num_keyframes) next = a->keyframes + keyframe_i + 1;
    Value dy;
    int32_t dx;
    if (!next) {
	memset(&dy, '\0', sizeof(dy));
	dx = 1;
    } else {
	dx = next->pos - k->pos;
	dy = jdaw_val_sub(next->value, k->value, a->val_type);
    }
    k->m_fwd.dy = dy;
    k->m_fwd.dx = dx;
}

static void keyframe_set_y_prop(Automation *a, uint16_t insert_i)
{
    Keyframe *k = a->keyframes + insert_i;
    Value v = jdaw_val_sub(k->value, a->min, a->val_type);
    k->draw_y_prop = jdaw_val_div_double(v, a->range, a->val_type);
}

Keyframe *automation_insert_keyframe_at(
    Automation *a,
    int32_t pos,
    Value val)
{
    if (a->num_keyframes + 1 >= a->keyframe_arrlen) {
	a->keyframe_arrlen *= 2;
	Keyframe *old_ptr = a->keyframes;
	a->keyframes = realloc(a->keyframes, a->keyframe_arrlen * sizeof(Keyframe));
	fprintf(stderr, "Reallocing kf array new len: %d\n", a->keyframe_arrlen);
	long int migration = a->keyframes - old_ptr;
	if (a->current) a->current += migration;
	if (a->track->tl->dragging_keyframe) a->track->tl->dragging_keyframe += migration;
    }


    uint16_t insert_i = 0;
    while (insert_i <a->num_keyframes) {
	/* fprintf(stderr, "\tChecking %d...\n", insert_i); */
	if (a->keyframes[insert_i].pos > pos) {
	    /* fprintf(stderr, "\t\tOvershot!\n"); */
	    break;
	}
	insert_i++;
    }
    fprintf(stderr, "INSERT at %d\n", pos);
    fprintf(stderr, "Insert i: %d\n", insert_i);
    uint16_t num_displaced = a->num_keyframes - insert_i;
    if (num_displaced != 0) {
	memmove(a->keyframes + insert_i + 1, a->keyframes + insert_i, num_displaced * sizeof(Keyframe));
    }
    Keyframe *inserted = a->keyframes + insert_i;
    inserted->automation = a;
    inserted->value = val;
    inserted->pos = pos;
    inserted->draw_x = timeline_get_draw_x(pos);
    a->num_keyframes++;
    keyframe_set_y_prop(a, insert_i);
    keyframe_recalculate_m(a, insert_i);
    /* fprintf(stderr, "CMP %d %d\n", insert_i, insert_i > 0); */
    if (insert_i > 0) {
	/* fprintf(stderr, "OK DO LAST!\n"); */
	keyframe_recalculate_m(a, insert_i - 1);
	/* for (uint8_t i=0; i<a->num_keyframes; i++) { */
	/*     fprintf(stderr, "i: %d, pos: %d, dx: %d, dy: %f\n", i, a->keyframes[i].pos, a->keyframes[i].m_fwd.dx, a->keyframes[i].m_fwd.dy.float_v); */
	/* } */
    }
    return inserted;
}

/* Keyframe *automation_insert_keyframe_after( */
/*     Automation *a, */
/*     Keyframe *insert_after, */
/*     Value val, */
/*     int32_t pos) */
/* { */
/*     if (insert_after && pos < insert_after->pos) { */
/* 	fprintf(stderr, "Fatal error: keyframe insert out of order\n"); */
/*     } */
/*     Keyframe *k = calloc(1, sizeof(Keyframe)); */
/*     k->automation = a; */
/*     k->value = val; */
/*     k->pos = pos; */
/*     if (insert_after) { */
/* 	Keyframe *next = insert_after->next; */
/* 	insert_after->next = k; */
/* 	k->prev = insert_after; */
/* 	keyframe_recalculate_m(insert_after, a->val_type); */
/* 	if (next) { */
/* 	    k->next = next; */
/* 	    next->prev = k; */
/* 	} else { */
/* 	    a->last = k; */
/* 	} */
	
/*     } else { */
/* 	if (a->first) { */
/* 	    k->next = a->first; */
/* 	    a->first->prev = k; */
/* 	    a->first = k; */
/* 	} else { */
/* 	    a->first = k; */
/* 	    a->last = k; */
/* 	} */
/*     } */
/*     keyframe_recalculate_m(k, a->val_type); */
/*     /\* Keyframe *print = automation->first; *\/ */
/*     /\* while (print) { *\/ */
/*     /\* 	/\\* fprintf(stdout, "keyframe at pos: %d\n", print->pos); *\\/ *\/ */
/*     /\* 	print = print->next; *\/ */
/*     /\* } *\/ */
/*     keyframe_set_y_prop(k); */
/*     k->draw_x = timeline_get_draw_x(k->pos); */
/*     return k; */
/* } */

/* Does not modify automation */
/* void keyframe_destroy(Keyframe *k) */
/* { */
/*     free(k); */
/* } */


static void automation_reset_cache(Automation *a, int32_t pos)
{

    int ops = 0;
    /* fprintf(stderr, "RESET cache pos %d\n", pos); */
    if (!a->current) {
	fprintf(stderr, "A current is null\n");
	if (a->keyframes[0].pos > pos) {
	    a->current = NULL;
	    goto describe;
	    /* return; */
	} else {
	    a->current = a->keyframes;
	}
    }
    while (a->current > a->keyframes && a->current->pos > pos) {
	ops++;
	a->current--;
    }
    if (a->current->pos > pos) {
	a->current = NULL;
	goto describe;
	/* return; */
    }

    while (a->current < a->keyframes + a->num_keyframes - 1 && (int32_t)pos - a->current->pos > a->current->m_fwd.dx) {
	ops++;
	a->current++;
    }
    
describe:
    fprintf(stderr, "OPS: %d\n", ops);
}

static Keyframe *automation_check_get_cache(Automation *a, int32_t pos)
{
    /* automation_reset_cache(a, pos); */
    /* return a->current; */
    /* if (a->keyframes[a->current].pos > pos) { */
    /* 	/\* fprintf(stderr, "Cache pos greater than current, so must reset\n"); *\/ */
    /* 	automation_reset_cache(a, pos); */
    /* 	return a->current; */
    /* } */
    /* if (a->current == a->num_keyframes - 1) { */
    /* 	/\* fprintf(stderr, "We're at the end\n"); *\/ */
    /* 	automation_reset_cache(a, pos); */
    /* 	return a->current; */
    /* } */
    /* if (a->keyframes[a->current + 1].pos > pos) { */
    /* 	/\* fprintf(stderr, "We're OK!\n"); *\/ */
    /* 	return a->current; */
    /* } */
    automation_reset_cache(a, pos);
    /* return a->current; */
    return a->current;
    
    /* bool cache_valid = false; */
    /* int32_t current_pos = a->keyframes[a->current].pos; */
    /* if (current_pos <= pos) { */
    /* 	if (a->current + 1 < a->num_keyframes && a->keyframes[a->current + 1].pos > pos) { */
    /* 	    cache_valid = true; */
    /* 	} */
	
    /* } */
    /* if (!cache_valid) { */
    /* 	automation_reset_cache(a, pos); */
    /* } */
}

static Value automation_value_at(Automation *a, int32_t pos)
{
    /* if (!a->current) { */
    /* 	return a->first->value; */
    /* } */
    /* Value m_dy; */
    /* memset(&m_dy, '\0', sizeof(Value)); */
    /* int32_t m_dx = 1; */
    /* if (a->current->next) { */
    /* 	m_dy = jdaw_val_sub(a->current->next->value, a->current->value, a->val_type); */
    /* 	m_dx = a->current->next->pos - a->current->pos; */
    /* } */



    /* First check to make sure we have the correct keyframe cached */

    /* int32_t current_i = automation_check_get_cache(a, pos); */
    /* if (!a->current) return a->keyframes[0].value; */
    /* Keyframe *current = a->keyframes + current_i; */
    pthread_mutex_lock(&a->lock);
    Keyframe *current = automation_check_get_cache(a, pos);
    pthread_mutex_unlock(&a->lock);
    /* int32_t current_i = current - a->keyframes; */
    /* fprintf(stderr, "Current i in READ: %d\n", current_i); */
    /* int32_ */
    if (!current) return a->keyframes[0].value;
    int32_t pos_rel = pos - current->pos;
    /* double scalar = (double)pos_rel / m_dx; */
    double scalar = (double)pos_rel / current->m_fwd.dx;
    /* return jdaw_val_add(a->current->value, jdaw_val_scale(m_dy, scalar, a->val_type), a->val_type); */
    return jdaw_val_add(current->value, jdaw_val_scale(current->m_fwd.dy, scalar, a->val_type), a->val_type);
}


/* void keyframe_remove_at(Automation *a, int16_t pos) */
/* void keyframe_remove(Keyframe *k) */
/* { */
/*     /\* if (k == k->automation->first) return; *\/ */
/*     /\* if (k->prev) { *\/ */
/*     /\*     k->prev->next = k->next; *\/ */
/*     /\* 	keyframe_recalculate_m(k->prev, k->automation->val_type); *\/ */
/*     /\* } *\/ */
/*     /\* if (k->next) *\/ */
/*     /\* 	k->next->prev = k->prev; *\/ */
/*     /\* /\\* if (k == k->automation->first) *\\/ *\/ */
/*     /\* /\\* 	return; *\\/ *\/ */
/*     /\* /\\* 	/\\\* k->automation->first = k->next; *\\\/ *\\/ *\/ */
/*     /\* if (k == k->automation->last)  *\/ */
/*     /\* 	k->automation->last = k->prev; *\/ */

/*     /\* keyframe_destroy(k); *\/ */
/* } */


static void keyframe_remove(Keyframe *k)
{
    fprintf(stderr, "REMOVE %d\n", k->pos);
    if (k == k->automation->keyframes) return; /* Don't remove last keyframe */
    Automation *a = k->automation;
    uint16_t pos = k - a->keyframes;
    uint16_t num = a->num_keyframes - pos;
    int ret = pthread_mutex_lock(&a->lock);
    if (ret != 0) {
	fprintf(stderr, "ERROR locking in remove: %s\n", strerror(ret));
	exit(1);
    }
    if (a->current == k) {
	if (k > a->keyframes) {
	    a->current = k-1;
	} else {
	    a->current = NULL;
	}
    }
    if (a->track->tl->dragging_keyframe) {
	a->track->tl->dragging_keyframe = NULL;
    }
    memmove(a->keyframes + pos, a->keyframes + pos + 1, num * sizeof(Keyframe));
    a->num_keyframes--;
    pthread_mutex_unlock(&a->lock);
}
static Keyframe *automation_insert_maybe(
    Automation *a,
    Value val,
    int32_t pos,
    int32_t chunk_end_pos,
    float direction)
{
    if (direction < 0.0) return NULL;
    /* fprintf(stderr, "\n\nSTART CHUNK %d-%d\n", pos, chunk_end_pos); */
    uint16_t current_i = automation_check_get_cache(a, pos) - a->keyframes;
    /* fprintf(stderr, "Current i: %d\n", current_i); */
    /* fprintf(stderr, "Same as ret? %d\n", a->current); */
    /* fprintf(stderr, "\tSure? %d < %d < %d\n", a->keyframes[current_i].pos, pos, a->keyframes[current_i + 1].pos); */
    uint16_t remove_start_i;
    if ((remove_start_i = current_i + 1) < a->num_keyframes && a->keyframes[remove_start_i].pos < chunk_end_pos) {
	uint16_t remove_start_i = current_i + 1;
	/* fprintf(stderr, "Start removal check at %d, pos %d\n", remove_start_i, a->keyframes[remove_start_i].pos); */
	uint16_t remove_end_i = remove_start_i;
	while (1) {
	    if (remove_end_i > a->num_keyframes) break;
	    if (a->keyframes[remove_end_i].pos > chunk_end_pos) {
		/* remove_end_i++; */
		break;
	    }
	    remove_end_i++;
	}
	/* while (remove_end_i < a->num_keyframes) { */
	/*     if (a->keyframes[remove_end_i].pos > chunk_end_pos) { */
	/* 	break; */
	/*     } */
	/*     remove_end_i++; */
	/* } */
	/* remove_end_i++; */
	/* fprintf(stderr, "REMOVE CHUNK: %d-%d\n", remove_start_i, remove_end_i - 1); */
	pthread_mutex_lock(&a->lock);
	int32_t remove_count = remove_end_i - remove_start_i;
	int32_t move_count = a->num_keyframes - remove_end_i;
	if (remove_count > 0) {
	    fprintf(stderr, "\t\t%d should be < %d\n", a->keyframes[remove_start_i - 1].pos, pos);
	    for (uint16_t i=0; i<remove_count; i++) {
		fprintf(stderr, "\t\tRemoving pos %d (betw %d and %d)\n", a->keyframes[remove_start_i + i].pos, pos, chunk_end_pos);
	    }
	    fprintf(stderr, "\t\t%d should be > %d\n", a->keyframes[remove_end_i].pos, chunk_end_pos);
	
	    fprintf(stderr, "Moving from %d to %d\n", remove_end_i, remove_start_i);
	    if (move_count > 0)
		memmove(a->keyframes + remove_start_i, a->keyframes + remove_end_i, move_count * sizeof(Keyframe));
	    a->num_keyframes -= remove_count;
	    fprintf(stderr, "\t->removed %d keyframes\n", remove_count);
	    fprintf(stderr, "\t->moved %d keyframes\n", move_count);
	    uint16_t current_i = a->current - a->keyframes;
	    uint16_t dragging_i = a->track->tl->dragging_keyframe - a->keyframes;
	    if (current_i >= remove_start_i && current_i < remove_end_i) {
		a->current = NULL;
	    }
	    if (dragging_i >= remove_start_i && dragging_i < remove_end_i) {
		a->track->tl->dragging_keyframe = NULL;
	    }
	    
	}
	pthread_mutex_unlock(&a->lock);
    }

    /* if (current_i < a->num_keyframes - 1 && a->keyframes[remove_start_i].pos < chunk_end_pos) { */
    /* 	uint16_t remove_end_i = remove_start_i; */
    /* 	while (remove_end_i < a->num_keyframes) { */
    /* 	    if (a->keyframes[remove_end_i].pos > chunk_end_pos) { */
    /* 		/\* remove_end_i--; *\/ */
    /* 		break; */
    /* 	    } */
    /* 	    remove_end_i ++; */
    /* 	} */
    
    static const double diff_prop_thresh = 1e-15;
    static const double m_prop_thresh = 0.008;


    /* Calculate deviance from predicted value */
    Value predicted_value = automation_value_at(a, pos);
    Value diff;

    double ghost_diff_prop = 0;
    if (a->ghost_valid) {
	diff = jdaw_val_sub(a->ghost_val, val, a->val_type);
	ghost_diff_prop = fabs(jdaw_val_div_double(diff, a->range, a->val_type));
    }

    /* fprintf(stderr, "\n\nDiff prop: %f, ghost diff prop: %f\n", diff_prop, ghost_diff_prop); */

    bool ghost_inserted = false;
    /* bool current_inserted = false; */

    if (ghost_diff_prop > diff_prop_thresh) {
	fprintf(stderr, "CHANGING\n");
	if (!a->changing) {
	    a->current = automation_insert_keyframe_at(a, a->ghost_pos, a->ghost_val);
	    ghost_inserted = true;
	    fprintf(stderr, "START changing\n");
	}
	a->changing = true;
    } else {
	fprintf(stderr, "Not changing\n");
	if (a->changing) {
	    fprintf(stderr, "STOP changing\n");
	    if (a->ghost_valid) {
		a->current = automation_insert_keyframe_at(a, a->ghost_pos, a->ghost_val);
		ghost_inserted = true;
	    }
	}
	if (!a->ghost_valid) {
	    fprintf(stderr, "GHOST INVALID!\n");
	    a->current = automation_insert_keyframe_at(a, pos, val);
	    pos++;
	    a->current = automation_insert_keyframe_at(a, pos, val);
	    pos++;
	    /* current_inserted = true; */
	    /* a->keyframes[a->current].dne+=10; */
	    
	}
	a->changing = false;
    }

    /* if (!current_inserted) { */
 	diff = jdaw_val_sub(predicted_value, val, a->val_type);
	double diff_prop = fabs(jdaw_val_div_double(diff, a->range, a->val_type));
	if (diff_prop > diff_prop_thresh) {
	    fprintf(stderr, "DEVIATION from predicted value\n");
	    a->current = automation_insert_keyframe_at(a, pos, val);
	    if (!ghost_inserted) {
		/* Keyframe *p = NULL; */
		/* Keyframe *pp = NULL; */
		/* if (a->current > 0) p = a->keyframes + a->current - 1; */
		/* if (a->current > 1) pp = a->keyframes + a->current - 2; */
		/* if (a->current > a->keyframes) p = a->current - 1; */
		/* if (a->current > a->keyframes + 1) pp = a->current - 2; */
		if (a->current > a->keyframes + 1) {
		    Keyframe *p = a->current - 1;
		    Keyframe *pp = a->current - 2;
		    Value ppm = jdaw_val_scale(pp->m_fwd.dy, 1000.0f / pp->m_fwd.dx, a->val_type);
		    Value pm = jdaw_val_scale(p->m_fwd.dy, 1000.0f / p->m_fwd.dx, a->val_type);
		    diff = jdaw_val_sub(ppm, pm, a->val_type);
		    diff_prop = jdaw_val_div_double(diff, a->range, a->val_type);
		    if (fabs(diff_prop) < m_prop_thresh) {//&& !jdaw_val_is_zero(ppm, a->val_type) && jdaw_val_sign_match(ppm, pm, a->val_type)) {
			/* if (fabs(a->mdelta_prop_cum) < 0.1) { */
			/* a->mdelta_prop_cum += diff_prop; */
			/* fprintf(stderr, "REMOVED %d\n", p->pos); */
			keyframe_remove(p);
			/* } else { */
			/* fprintf(stderr, "Not removed, prop cum\n"); */
			/* a->mdelta_prop_cum = 0.0; */
			/* } */
		    } else {
			fprintf(stderr, "NOT remove %f\n", diff_prop);
			/* a->mdelta_prop_cum = 0.0; */
		    }
		}
	    }

	}
    /* } */
    a->ghost_val = val;
    a->ghost_pos = pos;
    a->ghost_valid = true;

    return NULL;


}





    
    /* /\* fprintf(stderr, "Ghost: %f, Current: %f\n", a->ghost_val.float_v, val.float_v); *\/ */
    /* if (diff_prop + ghost_diff_prop > diff_prop_thresh) { */
	
    /* 	bool ghost_inserted = false; */

	
    /* 	/\* If the value is changing *\/ */
    /* 	if (ghost_diff_prop > diff_prop_thresh) { */
    /* 	    if (!a->changing) { */
    /* 		fprintf(stderr, "START change, diff prop: %f\n", diff_prop); */
    /* 		if (a->ghost_valid) { */
		
    /* 		    ghost_inserted = true; */
    /* 		    fprintf(stderr, "Inserting ghost\n"); */
    /* 		    a->current = automation_insert_keyframe_at(a, a->ghost_pos, a->ghost_val); */
    /* 		    /\* a->ghost_valid = false; *\/ */
    /* 		    /\* return NULL; *\/ */
    /* 		} */
    /* 	    } */
    /* 	    a->changing = true;		 */
    /* 	} else { */
    /* 	    if (a->changing) { */
    /* 		fprintf(stderr, "INTERNAL stop change\n"); */
    /* 	    } */
    /* 	    a->changing = false; */
    /* 	} */
    /* 	a->current = automation_insert_keyframe_at(a, pos, val); */
    /* 	/\* if (ghost_inserted) { *\/ */
    /* 	/\*     a->ghost_valid = true; *\/ */
    /* 	/\*     a->ghost_val = val; *\/ */
    /* 	/\*     a->ghost_pos = pos; *\/ */
    /* 	/\*     fprintf(stderr, "SETTING GHOST VALUE early return\n"); *\/ */

    /* 	/\*     /\\* return NULL; *\\/ *\/ */
    /* 	/\* } else { *\/ */

    /* 	/\* Do the work of removing if nonlinear *\/ */
    /* 	if (!ghost_inserted) { */
    /* 	    Keyframe *p = NULL; */
    /* 	    Keyframe *pp = NULL; */
    /* 	    if (a->current > 0) p = a->keyframes + a->current - 1; */
    /* 	    if (a->current > 1) pp = a->keyframes + a->current - 2; */
    /* 	    if (p && pp) { */

    /* 		Value ppm = jdaw_val_scale(pp->m_fwd.dy, 1000.0f / pp->m_fwd.dx, a->val_type); */
    /* 		Value pm = jdaw_val_scale(p->m_fwd.dy, 1000.0f / p->m_fwd.dx, a->val_type); */
    /* 		diff = jdaw_val_sub(ppm, pm, a->val_type); */
    /* 		diff_prop = jdaw_val_div_double(diff, a->range, a->val_type); */
    /* 		if (fabs(diff_prop) < m_prop_thresh) {//&& !jdaw_val_is_zero(ppm, a->val_type) && jdaw_val_sign_match(ppm, pm, a->val_type)) { */
    /* 		    /\* if (fabs(a->mdelta_prop_cum) < 0.1) { *\/ */
    /* 		    /\* a->mdelta_prop_cum += diff_prop; *\/ */
    /* 		    /\* fprintf(stderr, "REMOVED %d\n", p->pos); *\/ */
    /* 		    keyframe_remove(p); */
    /* 		    /\* } else { *\/ */
    /* 		    /\* fprintf(stderr, "Not removed, prop cum\n"); *\/ */
    /* 		    /\* a->mdelta_prop_cum = 0.0; *\/ */
    /* 		    /\* } *\/ */
    /* 		} else { */
    /* 		    fprintf(stderr, "NOT remove %f\n", diff_prop); */
    /* 		    /\* a->mdelta_prop_cum = 0.0; *\/ */
    /* 		} */
    /* 	    } */
    /* 	} */

    /* 	/\* return a->keyframes + a->current; *\/ */
    /* } else { */

	
    /* 	/\* } *\/ */
    /* 	/\* return NULL; *\/ */
    /* } */
    /* if (ghost_diff_prop < diff_prop_thresh) { */
    /* 	/\* a->changing = false; *\/ */
    /* 		/\* if (a->changing) { *\/ */
    /* 	if (a->changing) { */
    /* 	    fprintf(stderr, "STOP CHANGING\n"); */
    /* 	} */
    /* 	a->changing = false; */
    /* 	/\* a->ghost_val = val; *\/ */
    /* 	/\* a->ghost_pos = pos; *\/ */
    /* 	/\* a->ghost_valid = true; *\/ */
    /* 	/\* a->ghost_valid = false; *\/ */

	
    /* } */

    /* a->ghost_valid = true; */
    /* fprintf(stderr, "SETTING GHOST VALUE AT END\n"); */
    /* a->ghost_val = val; */
    /* a->ghost_pos = pos; */
    /* return NULL; */
    

/* static Keyframe *___automation_insert_maybe( */
/*     Automation *a, */
/*     Value val, */
/*     int32_t pos, */
/*     int32_t end_pos, */
/*     float direction) */
/* { */
/*     /\* Keyframe *k = a->first; *\/ */
/*     /\* int i=0;  *\/ */
/*     /\* while (k) { *\/ */
/*     /\* 	fprintf(stderr, "Kf %d pos: %d\n", i, k->pos); *\/ */
/*     /\* 	k = k->next; *\/ */
/*     /\* 	i++; *\/ */
/*     /\* } *\/ */
/*     bool set_current = false; */
/*     bool set_first = false; */
/*     if (!a->current) { */
/* 	/\* fprintf(stderr, "\n\nResetting current\n"); *\/ */
/* 	set_current = true; */
/*         a->current = automation_get_segment(a, pos); */
/*     } */
/*     if (!a->current) set_first = true; */

/*     if (direction < 0) { */
/* 	while (a->current && a->current->pos > end_pos) { */
/* 	    a->current = a->current->prev; */
/* 	} */
/* 	return NULL; */
/*     } */
/*     static const double diff_prop_thresh = 1e-15; */
/*     static const double m_prop_thresh = 0.008; */
/*     /\* static bool changing = false; *\/ */
/*     /\* fprintf(stderr, "POS %d\n", pos); *\/ */
/*     /\* Overwrite existing keyframes in chunk *\/ */
/*     if (a->current) { */
/* 	Keyframe *next = a->current->next; */
/* 	while (next && next->pos < end_pos) { */
/* 	    Keyframe *to_remove = next; */
/* 	    next = next->next; */
/* 	    bool dont_remove = false; */
/* 	    for (uint16_t i=0; i<a->num_kclips; i++) { */
/* 		if (to_remove == a->kclips[i].first) { */
/* 		    dont_remove = true; */
/* 		} */
/* 	    } */
/* 	    if (!dont_remove) */
/* 		keyframe_remove(to_remove); */
/* 	} */
/*     } */
/*     if (set_current) { */
/* 	a->current = automation_insert_keyframe_after(a, a->current, val, pos); */
/* 	if (set_first) { */
/* 	    fprintf(stderr, "SETTING auto first\n"); */
/* 	    a->first = a->current; */
/* 	} */
/* 	a->current = automation_insert_keyframe_after(a, a->current, val, end_pos); */
/* 	return NULL; */
/*     } */

    
/*     /\* Calculate where current value would be if automation was read *\/ */
/*     Value predicted_value = automation_value_at(a, pos); */

/*     /\* Calculate deviance from predicted value *\/ */
/*     Value diff = jdaw_val_sub(predicted_value, val, a->val_type); */
/*     double diff_prop = fabs(jdaw_val_div_double(diff, a->range, a->val_type)); */

/*     if (a->ghost_valid) { */
/* 	diff = jdaw_val_sub(a->ghost_val, val, a->val_type); */
/* 	diff_prop += fabs(jdaw_val_div_double(diff, a->range, a->val_type)); */
/*     } */

    
/*     /\* Only do work if current value is different from predicted value *\/ */
/*     if (diff_prop > diff_prop_thresh) { */
/* 	/\* Insert "ghost" keyframe if a change has *stopped* occurring *\/ */
/* 	bool ghost_inserted = false; */
/* 	if (a->changing == false && a->ghost_valid) { */
/* 	    a->current = automation_insert_keyframe_after(a, a->current, a->ghost_val, a->ghost_pos); */
/* 	    fprintf(stderr, "INSERT GHOST %d\n", a->ghost_pos); */
/* 	    ghost_inserted = true; */
/* 	} */
/* 	a->changing = true; */
/* 	a->ghost_val = val; */
/* 	a->ghost_pos = pos; */
/* 	a->ghost_valid = true; */

/* 	if (direction > 0) { */
/* 	    Keyframe *k = automation_insert_keyframe_after(a, a->current, val, pos); */
/* 	    fprintf(stderr, "STANDARD INSERTION %d\n", pos); */
/* 	    /\* if (not_removed) { *\/ */
/* 	    /\* 	fprintf(stderr, "Ok now inserted one more at %d\n", k->pos); *\/ */
/* 	    /\* 	fprintf(stderr, "Prev at %d\n", k->prev->pos); *\/ */
/* 	    /\* 	return NULL; *\/ */
/* 	    /\* } *\/ */
/* 	    a->current = k; */
/* 	    if (ghost_inserted) { */
/* 		/\* fprintf(stderr, "Now insert pos %d\n", pos); *\/ */
/* 		/\* fprintf(stderr, "->EXIT\n"); *\/ */
/* 		return NULL; */
/* 	    } */
/* 	    /\* if (set_current) { *\/ */
/* 	    /\* 	fprintf(stderr, "NOT REMOVE set current!\n"); *\/ */
/* 	    /\* 	fprintf(stderr, "NOT REMOVED: %d\n", k->pos); *\/ */
/* 	    /\* 	not_removed = true; *\/ */
/* 	    /\* 	/\\* a->current = automation_insert_keyframe_after(a, a->current, val, pos + 1); *\\/ *\/ */
/* 	    /\* 	return NULL; *\/ */
/* 	    /\* } *\/ */
/* 	    if (k->prev && k->prev->prev) { */
/* 		Keyframe *p = k->prev; */
/* 		Keyframe *pp = k->prev->prev; */
/* 		Value ppm = jdaw_val_scale(pp->m_fwd.dy, 1000.0f / pp->m_fwd.dx, a->val_type); */
/* 		Value pm = jdaw_val_scale(p->m_fwd.dy, 1000.0f / p->m_fwd.dx, a->val_type); */
/* 		diff = jdaw_val_sub(ppm, pm, a->val_type); */
/* 		diff_prop = jdaw_val_div_double(diff, a->range, a->val_type); */
/* 		if (fabs(diff_prop) < m_prop_thresh) {//&& !jdaw_val_is_zero(ppm, a->val_type) && jdaw_val_sign_match(ppm, pm, a->val_type)) { */
/* 		    /\* if (fabs(a->mdelta_prop_cum) < 0.1) { *\/ */
/* 			/\* a->mdelta_prop_cum += diff_prop; *\/ */
/* 			/\* fprintf(stderr, "REMOVED %d\n", p->pos); *\/ */
/* 			keyframe_remove(p); */
/* 		    /\* } else { *\/ */
/* 			/\* fprintf(stderr, "Not removed, prop cum\n"); *\/ */
/* 			/\* a->mdelta_prop_cum = 0.0; *\/ */
/* 		    /\* } *\/ */
/* 		} else { */
/* 		    fprintf(stderr, "NOT remove %f\n", diff_prop); */
/* 		    /\* a->mdelta_prop_cum = 0.0; *\/ */
/* 		} */
/* 	    } */
/* 	    /\* a->current = k; *\/ */
/* 	} */
/*     } else { */
/* 	fprintf(stderr, "STOP CHANGING\n"); */
/*         a->changing = false; */
/* 	a->ghost_val = val; */
/* 	a->ghost_pos = pos; */
/* 	a->ghost_valid = true; */
/*     } */
/*     return NULL; */
/* } */

void breakfn() {

}
void automation_do_write(Automation *a, int32_t pos, int32_t end_pos, float step)
{
    Value v;
    jdaw_val_set(&v, a->val_type, a->target_val);
    automation_insert_maybe(a, v, pos, end_pos, step);
}

void automation_reset_keyframe_x(Automation *a)
{
    /* Keyframe *k = a->first; */
    /* while (k) { */
    /* 	k->draw_x = timeline_get_draw_x(k->pos); */
    /* 	k = k->next; */
    /* } */
    for (uint16_t i=0; i<a->num_keyframes; i++) {
	a->keyframes[i].draw_x = timeline_get_draw_x(a->keyframes[i].pos);
    }
}

Keyframe *automation_get_segment(Automation *a, int32_t at)
{
    if (a->keyframes[0].pos > at) return NULL;
    Keyframe *k = a->keyframes;
    while (k < a->keyframes + a->num_keyframes - 1 && k->pos < at) k++;
    if (k == a->keyframes + a->num_keyframes - 1) return k;
    return k - 1;
    /* for (uint16_t i=0; i<a->num_keyframes; i++) { */
    /* 	Keyframe *k = a->keyframes + i; */
    /* 	if (k->pos > at) { */
    /* 	    return i == 0 ? 0 : i - 1; */
    /* 	} */
    /* } */
    /* return a->num_keyframes; */
}
/* Keyframe *automation_get_segment(Automation *a, int32_t at) */
/* { */
/*     /\* Keyframe *k = a->first; *\/ */
/*     /\* if (at < k->pos) return NULL; *\/ */
/*     /\* while (k && k->pos < at) { *\/ */
/*     /\* 	if (k->next) *\/ */
/*     /\* 	    k = k->next; *\/ */
/*     /\* 	else break; *\/ */
/*     /\* } *\/ */
/*     /\* if (k && k->pos > at && k->prev) *\/ */
/*     /\* 	return k->prev; *\/ */
/*     /\* else return k; *\/ */
/*     uint16_t i = automation_check_get_cache(a, at); */
/*     return a->keyframes + i; */
/* } */

static inline bool segment_intersects_screen(int a, int b)
{
    return (b >= 0 && main_win->w_pix >= a);
   
}

static inline bool x_onscreen(int x)
{
    return (x > 0 && x < main_win->w_pix);
}


/* clock_t start; */
/* clock_t running_total_old; */
/* clock_t running_total_new; */
/* int diagnostic_i = 0; */


/* static inline void keyframe_draw_old(int x, int y) */
/* { */
/*     start = clock(); */
/*     int half_diag = KEYFRAME_DIAG / 2; */
/*     x -= half_diag; */
/*     int y1 = y; */
/*     int y2 = y; */
/*     for (int i=0; i<KEYFRAME_DIAG; i++) { */
/* 	SDL_RenderDrawLine(main_win->rend, x, y1, x, y2); */
/* 	if (i < half_diag) { */
/* 	    y1--; */
/* 	    y2++; */
/* 	} else { */
/* 	    y1++; */
/* 	    y2--; */
/* 	} */
/* 	x++; */
/*     } */
/*     running_total_old += (clock() - start); */
/* } */

static void keyframe_draw(int x, int y)
{
    int dim = SYMBOL_STD_DIM * main_win->dpi_scale_factor;
    SDL_Rect dst = {x - dim / 4, y - dim / 4, dim / 2, dim / 2};
    symbol_draw(SYMBOL_TABLE[SYMBOL_KEYFRAME], &dst);
}

/* This function assumes "current" pointer has been set or unset appropriately elsewhere */
Value inline automation_get_value(Automation *a, int32_t pos, float direction)
{
    /* if (!a->current) { */
    /* 	a->current = automation_get_segment(a, pos); */
    /* } */
    /* if (!a->current) { */
    /* 	return a->keyframes[0].value; */
    /* } */

    /* automation_check_get_cache(a, pos); */
    return automation_value_at(a, pos);
    /* if (direction > 0.0f && a->current->next && a->current->next->pos <= pos) { */
    /* 	a->current = a->current->next; */
    /* } else if (direction < 0.0f  && pos < a->current->pos) { */
    /* 	if (a->current->prev) { */
    /* 	    a->current = a->current->prev; */
    /* 	} else { */
    /* 	    a->current = NULL; */
    /* 	    return a->first->value; */
    /* 	} */
    /* } */
    /* return automation_value_at(a, pos); */
}

void automation_draw(Automation *a)
{
    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(automation_bckgrnd));
    SDL_RenderFillRect(main_win->rend, a->bckgrnd_rect);
    
    SDL_RenderSetClipRect(main_win->rend, a->bckgrnd_rect);


    /* Draw KCLIPS */

    static SDL_Color homebckgrnd = {200, 200, 255, 50};
    static SDL_Color bckgrnd = {200, 255, 200, 50};
    static SDL_Color brdr = {200, 200, 200, 200};
    
    /* SDL_SetRenderDrawColor(main_win->rend, 100, 100, 255, 100); */
    SDL_Rect kcliprect = {0, a->layout->rect.y, 0, a->layout->rect.h};
    for (uint16_t i=0; i<a->num_kclips; i++) {
	/* fprintf(stderr, "Draw kclip %d\n", i); */
	KClipRef *kcr = a->kclips + i;
	kcliprect.x = timeline_get_draw_x(kcr->first->pos);
	kcliprect.w = timeline_get_draw_x(kcr->last->pos) - kcliprect.x;
	/* SDL_RenderDrawLine(main_win->rend, x, 0, x, 800); */
	if (kcr->home) {
	    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(homebckgrnd));
	} else {
	    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(bckgrnd));
	}
	SDL_RenderFillRect(main_win->rend, &kcliprect);
	SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(brdr));
	SDL_RenderDrawRect(main_win->rend, &kcliprect);
    }

    
    SDL_SetRenderDrawColor(main_win->rend, 255, 255, 255, 255);
    /* Keyframe *k = a->first; */
    int h = a->layout->rect.h;
    int last_y = 0;
    int bottom_y = a->layout->rect.y + a->layout->rect.h;
    Keyframe *first = a->keyframes;
    Keyframe *last = a->keyframes + a->num_keyframes - 1;
    for (uint16_t i=0; i<a->num_keyframes; i++) {
	Keyframe *k = a->keyframes + i;
	Keyframe *prev = k - 1;
        int y = bottom_y - k->draw_y_prop * h;
	if (x_onscreen(k->draw_x)) {
	    keyframe_draw(k->draw_x, y);
	    if (k == a->track->tl->dragging_keyframe) {
		SDL_Rect r = {k->draw_x - 10, y - 10, 20, 20};
		SDL_RenderDrawRect(main_win->rend, &r);
	    }
	}
	if (last_y != 0 && segment_intersects_screen(prev->draw_x, k->draw_x)) {
	    SDL_RenderDrawLine(main_win->rend, prev->draw_x, last_y, k->draw_x, y);
	}
	if (k == first && k->draw_x > 0) {
	    SDL_RenderDrawLine(main_win->rend, 0, y, k->draw_x, y);
	}
        if (k == last && k->draw_x < main_win->w_pix) {
	    SDL_RenderDrawLine(main_win->rend, k->draw_x, y, main_win->w_pix, y);
	}

	last_y = y;
	/* k = k->next; */
    }
    SDL_RenderSetClipRect(main_win->rend, NULL);
    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(automation_bckgrnd));
    SDL_RenderFillRect(main_win->rend, a->console_rect);


    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(a->track->color));
    SDL_RenderFillRect(main_win->rend, a->color_bar_rect);

    textbox_draw(a->label);
    button_draw(a->read_button);
    button_draw(a->write_button);
    label_draw(a->keyframe_label);
}
/* void ___automation_draw(Automation *a) */
/* { */
/*     SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(automation_bckgrnd)); */
/*     SDL_RenderFillRect(main_win->rend, a->bckgrnd_rect); */

/*     Keyframe *k = a->first; */

/*     int bottom_y = a->layout->rect.y + a->layout->rect.h; */
/*     int y_left; */
/*     int y_right; */
/*     int k_pos; */
/*     int next_pos; */
    
/*     if (!k) return; */
    
/*     next_pos = timeline_get_draw_x(k->pos); */
/*     y_right = bottom_y -  k->draw_y_prop * a->layout->rect.h; */


/*     SDL_RenderSetClipRect(main_win->rend, a->bckgrnd_rect); */
/*     SDL_SetRenderDrawColor(main_win->rend, 255, 255, 255, 255); */

/*     /\* fprintf(stdout, "\n\n\n"); *\/ */
/*     /\* int keyframe_i = 0; *\/ */
/*     bool init_y = true; */
/*     while (k) { */
	
/* 	k_pos = next_pos; */
/* 	if (k == a->first && next_pos > 0) { */
/* 	    int y = bottom_y - k->draw_y_prop * a->layout->rect.h; */
/* 	    SDL_RenderDrawLine(main_win->rend, 0, y, k_pos, y); */
/* 	} */
/* 	if (x_onscreen(k_pos)) { */
/* 	    keyframe_draw(k_pos, y_right); */
/* 	    if (!k->next) { */
/* 		y_left = bottom_y - a->last->draw_y_prop * a->layout->rect.h; */
/* 		SDL_RenderDrawLine(main_win->rend, k_pos, y_left, main_win->w_pix, y_left); */
/* 		break; */
/* 	    } */
/* 	} */
/* 	if (k->next) { */
/* 	    next_pos = timeline_get_draw_x(k->next->pos); */
/* 	    if (segment_intersects_screen(k_pos, next_pos)) { */
/* 		if (init_y) { */
/* 		    y_left = bottom_y - k->draw_y_prop * a->layout->rect.h; */
/* 		    init_y = false; */
/* 		} else { */
/* 		    y_left = y_right; */
/* 		} */
/* 		y_right = bottom_y -  k->next->draw_y_prop * a->layout->rect.h; */
/* 		/\* fprintf(stdout, "%d: %d->%d\n", keyframe_i, y_left, y_right); *\/ */
/* 		SDL_RenderDrawLine(main_win->rend, k_pos, y_left, next_pos, y_right);  */
/* 	    } */
/* 	} */
/* 	k = k->next; */
/* 	/\* keyframe_i++; *\/ */
/*     } */
/*     /\* if (a->first) { *\/ */
/*     /\* 	y_left = bottom_y - a->first->draw_y_prop * a->layout->rect.h; *\/ */
/*     /\* 	SDL_RenderDrawLine(main_win->rend, 0, y_left, k_pos, y_left); *\/ */
/*     /\* } *\/ */
/*     /\* if (a->last) { *\/ */
/*     /\* 	y_left = bottom_y - a->last->draw_y_prop * a->layout->rect.h; *\/ */
/*     /\* 	SDL_RenderDrawLine(main_win->rend, k_pos, y_left, main_win->w_pix, y_left); *\/ */
/*     /\* } *\/ */

/*     SDL_RenderSetClipRect(main_win->rend, NULL); */
/*     SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(automation_bckgrnd)); */
/*     SDL_RenderFillRect(main_win->rend, a->console_rect); */


/*     SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(a->track->color)); */
/*     SDL_RenderFillRect(main_win->rend, a->color_bar_rect); */

/*     textbox_draw(a->label); */
/*     button_draw(a->read_button); */
/*     button_draw(a->write_button); */
/*     label_draw(a->keyframe_label); */
/* } */


static void automation_editing(Automation *a, Keyframe *k, int x, int y)
{
    label_move(a->keyframe_label, x - 50, y - 50);
    label_reset(a->keyframe_label);   
}
static void keyframe_move(Keyframe *k, int x, int y)
{
    /* fprintf(stdout, "MOVE to %d, %d\n", x, y); */
    int32_t abs_pos = timeline_get_abspos_sframes(x);
    /* Keyframe *k = a->keyframes + i; */
    Automation *a = k->automation;
    uint16_t i = k - a->keyframes;
    
    Keyframe *prev = NULL;
    Keyframe *next = NULL;
    if (i > 0) {
	prev = k-1;
    }
    if (i < a->num_keyframes - 1) {
	next = k+1;
    }
    
    if ((!prev || (abs_pos > prev->pos)) && (!next || (abs_pos < next->pos))) {
	k->draw_x = x;
	double val_prop = (double)(a->bckgrnd_rect->y + a->bckgrnd_rect->h - y) / a->bckgrnd_rect->h;
	Value range_scaled = jdaw_val_scale(a->range, val_prop, a->val_type);
	Value v = jdaw_val_add(a->min, range_scaled, a->val_type);
	if (jdaw_val_less_than(a->max, v, a->val_type)) v = a->max;
	else if (jdaw_val_less_than(v, a->min, a->val_type)) v = a->min;
	k->value = v;
	k->pos = abs_pos;
	keyframe_set_y_prop(a, i);
	keyframe_recalculate_m(a, i);
	if (prev) {
	    keyframe_recalculate_m(a, i-1);
	}
	automation_editing(a, k, x, y);
    }
}

bool automations_triage_motion(Timeline *tl)
{
    Keyframe *k = tl->dragging_keyframe;
    if (k) {
	if (main_win->i_state & I_STATE_MOUSE_L) {
	    keyframe_move(k, main_win->mousep.x, main_win->mousep.y);
	    return true;
	} else {
	    tl->dragging_keyframe = NULL;
	}
    }
	/* Automation *a = k->automation; */
	/* int32_t abs_pos = timeline_get_abspos_sframes(main_win->mousep.x); */
	/* if ((!k->prev || (abs_pos > k->prev->pos)) && (!k->next || (abs_pos < k->next->pos))) { */
	/*     double val_prop = (double)(a->bckgrnd_rect->y + a->bckgrnd_rect->h - main_win->mousep.y) / a->bckgrnd_rect->h; */
	/*     Value range_scaled = jdaw_val_scale(a->range, val_prop, a->val_type); */
	/*     Value v = jdaw_val_add(a->min, range_scaled, a->val_type); */
	/*     if (jdaw_val_less_than(a->max, v, a->val_type)) v = a->max; */
	/*     else if (jdaw_val_less_than(v, a->min, a->val_type)) v = a->min; */
	/*     k->value = v; */
	/*     keyframe_set_y_prop(k); */
	/*     keyframe_recalculate_m(k, a->val_type); */
	/*     if (k->prev) { */
	/* 	keyframe_recalculate_m(k->prev, a->val_type); */
	/*     } */
	/*     k->pos = abs_pos; */

	/* } */
	/* return true; */
    /* } */
    return false;

}


bool automation_triage_click(uint8_t button, Automation *a)
{
    int click_tolerance = 10 * main_win->dpi_scale_factor;;
    /* int32_t epsilon = 10000; */
    if (SDL_PointInRect(&main_win->mousep, &a->layout->rect)) {
	if (button_click(a->read_button, main_win)) return true;
	if (button_click(a->write_button, main_win)) return true;
	if (SDL_PointInRect(&main_win->mousep, a->bckgrnd_rect) && main_win->i_state & I_STATE_CMDCTRL) {
	    int32_t clicked_pos_sframes = timeline_get_abspos_sframes(main_win->mousep.x);
	    /* uint16_t insertion_i = automation_get_segment(a, clicked_pos_sframes); */
	    Keyframe *insertion = automation_get_segment(a, clicked_pos_sframes);
	    Keyframe *next = NULL;
	    int left_kf_dist = click_tolerance + 1;
	    int right_kf_dist = click_tolerance + 1;
	    if (!insertion) next = a->keyframes;
	    else if (insertion < a->keyframes + a->num_keyframes - 1) {
		next = insertion + 1;
	    }
	    /* fprintf(stderr, "\n\nL R: %ld %ld\n", insertion - a->keyframes, next - a->keyframes); */
	    /* if (insertion_i < a->num_keyframes - 1) next = insertion + 1; */
	    if (insertion) left_kf_dist = abs(insertion->draw_x - main_win->mousep.x);
	    if (next) right_kf_dist = abs(next->draw_x - main_win->mousep.x);	
	    /* fprintf(stderr, "LDIST %d RDIST %d\n", left_kf_dist, right_kf_dist); */
	    if (left_kf_dist < click_tolerance && left_kf_dist < right_kf_dist) {
		a->track->tl->dragging_keyframe = insertion;
		keyframe_move(insertion, main_win->mousep.x, main_win->mousep.y);
		/* fprintf(stderr, "Left kf clicked\n"); */
	    } else if (right_kf_dist < click_tolerance) {
		a->track->tl->dragging_keyframe = next;
		keyframe_move(next, main_win->mousep.x, main_win->mousep.y);
		/* fprintf(stderr, "Right kf clicked\n"); */
	    } else {
		double val_prop = (double)(a->bckgrnd_rect->y + a->bckgrnd_rect->h - main_win->mousep.y) / a->bckgrnd_rect->h;
		Value range_scaled = jdaw_val_scale(a->range, val_prop, a->val_type);
		Value v = jdaw_val_add(a->min, range_scaled, a->val_type);
		/* uint16_t new_i = automation_insert_keyframe_at(a, clicked_pos_sframes, v); */
		/* a->track->tl->dragging_keyframe = a->keyframes + new_i; */
		a->track->tl->dragging_keyframe = automation_insert_keyframe_at(a, clicked_pos_sframes, v);

	    }
	    /* Keyframe *insertion_segment = automation_get_segment(a, clicked_pos_sframes); */
	    /* Keyframe *next = NULL; */
	    /* if (!insertion_segment) { */
	    /* 	double val_prop = (double)(a->bckgrnd_rect->y + a->bckgrnd_rect->h - main_win->mousep.y) / a->bckgrnd_rect->h; */
	    /* 	Value range_scaled = jdaw_val_scale(a->range, val_prop, a->val_type); */
	    /* 	Value v = jdaw_val_add(a->min, range_scaled, a->val_type); */
	    /* 	a->track->tl->dragging_keyframe = automation_insert_keyframe_at(a,clicked_pos_sframes, v); */
	    /* 	return true; */
	    /* } */
	    /* /\* if (abs(insertion_segment->pos - clicked_pos_sframes) < epsilon) { *\/ */
	    /* if (abs(insertion_segment->draw_x - main_win->mousep.x) < click_tolerance) { */
	    /* 	a->track->tl->dragging_keyframe = insertion_segment; */
	    /* 	keyframe_move(insertion_segment, main_win->mousep.x, main_win->mousep.y); */
	    /* /\* } else if (insertion_segment->next && abs(insertion_segment->next->pos - clicked_pos_sframes) < epsilon) { *\/ */
	    /* } else if (insertion_segment->next && abs(insertion_segment->next->draw_x - main_win->mousep.x) < click_tolerance) { */
	    /* 	a->track->tl->dragging_keyframe = insertion_segment->next; */
	    /* 	keyframe_move(insertion_segment->next, main_win->mousep.x, main_win->mousep.y); */
	    /* } else { */
	    /* 	double val_prop = (double)(a->bckgrnd_rect->y + a->bckgrnd_rect->h - main_win->mousep.y) / a->bckgrnd_rect->h; */
	    /* 	Value range_scaled = jdaw_val_scale(a->range, val_prop, a->val_type); */
	    /* 	Value v = jdaw_val_add(a->min, range_scaled, a->val_type); */
	    /* 	a->track->tl->dragging_keyframe = automation_insert_keyframe_after(a, insertion_segment,v, clicked_pos_sframes); */
	    /* 	/\* keyframe_move(insertion->next, main_win->mousep.x, main_win->mousep.y); *\/ */
	    /* } */
	    return true;
	}
    }
    return false;
}

void user_tl_play(void *nullarg);
void user_tl_pause(void *nullarg);

/* static KClip *record_kclip(Automation *a, int32_t start_pos, int32_t end_pos) */
/* { */
/*     /\* if (end_pos <= start_pos) return NULL; *\/ */

/*     /\* Keyframe *first = automation_get_segment(a, start_pos); *\/ */
/*     /\* fprintf(stderr, "first is %p, auto first is %p %d vs %d\n", first, a->first, a->first->pos, start_pos); *\/ */
/*     /\* if (!first) { *\/ */
/*     /\* 	first = a->first; *\/ */
/*     /\* 	while (first && first->pos < start_pos) { *\/ */
/*     /\* 	    first = first->next; *\/ */
/*     /\* 	} *\/ */
/*     /\* } *\/ */
/*     /\* if (!first || !first->next) return NULL; *\/ */
/*     /\* first = first->next; *\/ */
/*     /\* Keyframe *k = first; *\/ */
/*     /\* while (k->pos < end_pos) { *\/ */
/*     /\* 	if (k->next) { *\/ */
/*     /\* 	    k = k->next; *\/ */
/*     /\* 	} else break; *\/ */
/*     /\* } *\/ */
/*     /\* KClip *kc = calloc(1, sizeof(KClip)); *\/ */
/*     /\* kc->first = first; *\/ */
/*     /\* kc->last = k; *\/ */
/*     /\* return kc; *\/ */
/*     return NULL; */
/* } */

/* static void kclip_set_pos_rel(KClip *kc) */
/* { */
/*     /\* Keyframe *k = kc->first; *\/ */
/*     /\* int32_t start_pos = k->pos; *\/ */
/*     /\* while (k) { *\/ */
/*     /\* 	k->pos_rel = k->pos - start_pos; *\/ */
/*     /\* 	if (k == kc->last) break; *\/ */
/*     /\* 	k=k->next; *\/ */
/*     /\* } *\/ */
/* } */

void automation_insert_kclipref(Automation *a, KClip *kc, int32_t pos)
{
    /* Keyframe *k = kc->first; */
    /* int i=0; */
    /* while (k && k != kc->last) { */
    /* 	fprintf(stderr, "K %d: %p\n", i, k); */
    /* 	k = k->next; */
    /* 	i++; */
    /* } */
    
    /* Keyframe *prev = automation_get_segment(a, pos); */
    /* Keyframe *next = NULL; */
    /* if (prev) next = prev->next; */
    /* if (a->num_kclips == a->kclips_arr_len) { */
    /* 	a->kclips_arr_len *= 2; */
    /* 	a->kclips = realloc(a->kclips, a->kclips_arr_len * sizeof(KClipRef)); */
    /* } */
    /* KClipRef *kcr = a->kclips + a->num_kclips; */
    /* kcr->kclip = kc; */
    /* kcr->home = false; */
    /* kcr->automation = a; */
    /* Keyframe *to_copy = kc->first; */
    /* bool first = true; */
    /* int32_t last_pos = INT32_MIN; */
    /* i = 0; */
    /* while (to_copy) { */
    /* 	if (to_copy->pos < last_pos ) { */
    /* 	    fprintf(stderr, "CYCLE DETECTED!!!! Exiting!\n"); */
    /* 	    exit(1); */
    /* 	} */
    /* 	last_pos = to_copy->pos; */
    /*     Keyframe *cpy = calloc(1, sizeof(Keyframe)); */
    /* 	memcpy(cpy, to_copy, sizeof(Keyframe)); */
    /* 	if (first) { */
    /* 	    kcr->first = cpy; */
    /* 	    first = false; */
    /* 	} */
    /* 	cpy->prev = prev; */
    /* 	prev->next = cpy; */
    /* 	cpy->next = NULL; */
    /* 	cpy->pos = to_copy->pos_rel + pos; */
    /* 	fprintf(stderr, "To Copy %d: %p vs %p at pos %d (dist to last: %d)\n", i, to_copy, kc->last, to_copy->pos, kc->last->pos_rel + pos - to_copy->pos); */
    /* 	i++; */
    /* 	cpy->draw_x = timeline_get_draw_x(cpy->pos); */
    /* 	/\* keyframe_set_y_prop(cpy); *\/ */
    /* 	cpy->automation = a; */
    /* 	keyframe_set_y_prop(cpy); */
    /* 	prev = cpy; */
    /* 	if (to_copy == kc->last) { */
    /* 	    kcr->last = cpy; */
    /* 	    break; */
    /* 	} */
    /* 	to_copy = to_copy->next; */
    /* } */
    /* if (next) { */
    /* 	int32_t end_pos = kcr->last->pos; */
    /* 	while (next && next->pos < end_pos) { */
    /* 	    Keyframe *to_remove = next; */
    /* 	    fprintf(stderr, "removing %p at pos %d, before %d\n", to_remove, to_remove->pos,  end_pos); */
    /* 	    next = next->next; */
    /* 	    if (to_remove == a->last) a->last = NULL; */
    /* 	    keyframe_remove(to_remove); */
    /* 	} */
    /* 	kcr->last->next = next; */
    /* 	next->prev = kcr->last; */
    /* } else { */
    /* 	kcr->last->next = NULL; */
    /* } */
    /* /\* Sanity check *\/ */

    /* /\* Keyframe *k = a->first; *\/ */
    /* /\* int ki = 0; *\/ */
    /* /\* while (k) { *\/ */
    /* /\* 	fprintf(stderr, "K (%d): %p, pos: %d\n", ki, k, k->pos); *\/ */
    /* /\* 	ki++; *\/ */
    /* /\* 	k = k->next; *\/ */
    /* /\* 	if (ki > 200) { *\/ */
    /* /\* 	    exit(1); *\/ */
    /* /\* 	} *\/ */
    /* /\* } *\/ */
    /* a->num_kclips++; */
    /* if (!a->last || (kcr->last->next == NULL)) { */
    /* 	a->last = kcr->last; */
    /* } */
    /* /\* automation_reset_keyframe_x(a); *\/ */
}

void automation_record(Automation *a)
{
    /* static int32_t start_pos = 0; */
    /* bool write = automation_toggle_write(a); */
    /* if (write) { */
    /* 	start_pos = a->track->tl->play_pos_sframes; */
    /* 	user_tl_play(NULL); */
    /* 	timeline_play_speed_set(1.0); */
    /* } else { */
    /* 	KClip *kc = record_kclip(a, start_pos, a->track->tl->play_pos_sframes); */
    /* 	if (kc) { */
    /* 	    kclip_set_pos_rel(kc); */
    /* 	    if (a->num_kclips == a->kclips_arr_len) { */
    /* 		a->kclips_arr_len *= 2; */
    /* 		a->kclips = realloc(a->kclips, a->kclips_arr_len * sizeof(KClipRef)); */
    /* 	    } */
    /* 	    KClipRef *kcr = a->kclips + a->num_kclips; */
    /* 	    kcr->pos = kc->first->pos; */
    /* 	    kcr->kclip = kc; */
    /* 	    kcr->automation = a; */
    /* 	    kcr->home = true; */
    /* 	    kcr->first = kc->first; */
    /* 	    kcr->last = kc->last; */
    /* 	    a->num_kclips++; */
	    
    /* 	    /\* int i=0; *\/ */
    /* 	    Keyframe *k = kc->first; */
    /* 	    while (k && k!= kc->last) { */
    /* 		/\* i++; *\/ */
    /* 		k = k->next; */
    /* 		/\* fprintf(stderr, "K %p\n", k); *\/ */
    /* 	    } */
    /* 	    automation_insert_kclipref(a, kc, start_pos + 5 * proj->sample_rate); */
    /* 	} */
    /* 	user_tl_pause(NULL); */
    /* } */
}


/* NO OP */
void automation_clear_cache(Automation *a)
{
    a->ghost_valid = false;

}
