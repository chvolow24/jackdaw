/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

/* NOTE on automation<>endpoints and automation->deleted:
   (see automation.h)
*/


#include <pthread.h>
#include <stdlib.h>

#include "automation.h"
#include "color.h"
#include "endpoint.h"
#include "input.h"
#include "layout.h"
#include "layout_xml.h"
#include "modal.h"
#include "project.h"
#include "session.h"
#include "status.h"
#include "symbol.h"
#include "test.h"
#include "timeline.h"
#include "value.h"

#define KEYFRAME_DIAG 21
#define AUTOMATION_LT_H 60
#define AUTOMATION_LT_Y 4

#ifndef INSTALL_DIR
#define INSTALL_DIR "."
#endif

#define AUTOMATION_LT_FILEPATH INSTALL_DIR "/assets/layouts/track_automation.xml"

extern Window *main_win;
extern struct colors colors;

extern Symbol *SYMBOL_TABLE[];

extern SDL_Color console_bckgrnd;
extern SDL_Color control_bar_bckgrnd;
extern SDL_Color color_mute_solo_grey;








/* SDL_Color automation_bckgrnd = {0, 25, 26, 255}; */
/* SDL_Color automation_console_bckgrnd = {110, 110, 110, 255}; */
/* SDL_Color automation_console_bckgrnd = {90, 100, 120, 255}; */

SDL_Color automation_console_bckgrnd = {90, 100, 110, 255};
SDL_Color automation_bckgrnd = {70, 80, 90, 255};


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

TEST_FN_DECL(layout_num_children, Layout *lt);
void automation_destroy(Automation *a)
{
    TEST_FN_CALL(layout_num_children, a->track->layout);
    if (a->keyframes) 
	free(a->keyframes);
    if (a->undo_cache)
	free(a->undo_cache);
    if (a->label)
	textbox_destroy(a->label);
    if (a->keyframe_label)
	label_destroy(a->keyframe_label);
    if (a->endpoint)
	a->endpoint->automation = NULL;
    if (a->read_button)
	button_destroy(a->read_button);
    if (a->write_button)
	button_destroy(a->write_button);
    if (a->layout)
	layout_destroy(a->layout);
    free(a);
}

/* automation_delete is higher-level and should be used where possible */
void automation_remove(Automation *a)
{
    /* if (a->deleted) return; */
    if (a->removed) return;
    a->removed = true;
    Track *track = a->track;
    track->tl->needs_redraw = true;
    bool displace = false;
    bool some_read = false;
    for (uint16_t i=0; i<track->num_automations; i++) {
	if (track->automations[i] == a) {
	    displace = true;
	    /* layout_remove_child_at(a->layout->parent, a->layout->index); */
	    layout_remove_child(a->layout);
	    
	} else {
	    if (track->automations[i]->read) some_read = true;
	    if (displace) {
		track->automations[i]->index--;
		track->automations[i - 1] = track->automations[i];
	    }  
	}
    }
    layout_reset(a->layout);
    layout_size_to_fit_children_v(a->track->layout, true, 0);
    timeline_rectify_track_area(a->track->tl);

    layout_size_to_fit_children_v(a->layout->cached_parent, true, 0);
    layout_size_to_fit_children_v(track->layout, true, 0);
    timeline_rectify_track_area(a->track->tl);
    /* layout_force_reset(track->layout->parent); */
    /* timeline_reset(track->tl, false); */
    track->num_automations--;
    if (!some_read || track->num_automations == 0) {
	track->automation_dropdown->background_color = &colors.grey;
    }
    if (track->num_automations == 0) {
	track->selected_automation = -1;
	track->automation_dropdown->symbol = SYMBOL_TABLE[SYMBOL_DROPDOWN];
    } else if (track->selected_automation > track->num_automations - 1) {
	track->selected_automation = track->selected_automation - 1;
    }
    TEST_FN_CALL(track_automation_order, track);
}

void automation_reinsert(Automation *a)
{
    if (!a->removed) return;
    if (a->index > a->track->num_automations) a->index = a->track->num_automations;
    a->removed = false;
    /* a->deleted = false; */
    Track *track = a->track;
    track->tl->needs_redraw = true;
    for (int16_t i=track->num_automations; i>=0; i--) {
	if (i < track->num_automations) {
	    /* fprintf(stderr, "incr index, moving %d->%d\n", i, i+1); */
	    track->automations[i]->index++;
	    track->automations[i + 1] = track->automations[i];
	}
	if (i == a->index) {
	    track->automations[i] = a;
	    Layout *automation_container = a->track->layout;
	    layout_insert_child_at(a->layout, automation_container, a->index + 1);
	    track->num_automations++;
	    layout_size_to_fit_children_v(automation_container, true, 0);
	    /* layout_size_to_fit_children_v(a->layout->parent, true, 0); */
	    layout_size_to_fit_children_v(track->layout, true, 0);
	    timeline_rectify_track_area(a->track->tl);
	    track_automations_show_all(track);
	    TEST_FN_CALL(track_automation_order, track);
	    if (a->read) {
		track->automation_dropdown->background_color = &colors.dropdown_green;
	    }
	    return;
	}
    }

    TEST_FN_CALL(track_automation_order, track);
}

NEW_EVENT_FN(undo_add_automation, "undo add automation")
    Automation *a = (Automation *)obj1;
    a->deleted = true;
    automation_remove(a);
}
NEW_EVENT_FN(redo_add_automation, "redo add automation")
    Automation *a = (Automation *)obj1;
    a->deleted = false;
    automation_reinsert(a);
}


NEW_EVENT_FN(dispose_delete_automation, "")
    automation_destroy((Automation *)obj1);
}

NEW_EVENT_FN(undo_delete_automation, "undo delete automation")
    Automation *a = (Automation *)obj1;
    a->deleted = false;
    automation_reinsert(a);
}

NEW_EVENT_FN(redo_delete_automation, "redo delete automation")
    Automation *a = (Automation *)obj1;
    a->deleted = true;
    automation_remove(a);
}

void automation_delete(Automation *a)
{
    automation_remove(a);
    a->deleted = true;
    Value nullval;
    memset(&nullval, '\0', sizeof(Value));
    user_event_push(
	
	undo_delete_automation,
	redo_delete_automation,
	dispose_delete_automation, NULL,
	(void *)a, NULL,
	nullval, nullval, nullval, nullval,
	0, 0, false, false);
}

/* Endpoint MUST have min, max, and default set */
Automation *track_add_automation_from_endpoint(Track *track, Endpoint *ep)
{
    if (track->num_automations == MAX_TRACK_AUTOMATIONS) {
	status_set_errstr("Error: reached maximum number of automations per track\n");
	return NULL;
    }
    Automation *a = calloc(1, sizeof(Automation));
    a->track = track;
    a->type = AUTO_ENDPOINT;
    a->read = true;
    a->index = track->num_automations;
    a->keyframe_arrlen = KEYFRAMES_ARR_INITLEN;
    a->keyframes = calloc(a->keyframe_arrlen, sizeof(Keyframe));
    a->min = ep->min;
    a->max = ep->max;
    a->val_type = ep->val_type;
    a->range = jdaw_val_sub(ep->max, ep->min, ep->val_type);
    /* snprintf(a->name, MAX_NAMELENGTH, "%s", api_endpoint_get_route_until(ep, &track->api_node)); */
    api_endpoint_get_display_route_until(ep, a->name, MAX_NAMELENGTH, &track->api_node);

    int ret;
    if ((ret = pthread_mutex_init(&a->lock, NULL) != 0)) {
	fprintf(stderr, "Error initializing automation lock: %s\n", strerror(ret));
	exit(1);
    }
    if ((ret = pthread_mutex_init(&a->keyframe_arr_lock, NULL) != 0)) {
	fprintf(stderr, "Error initializing keyframe arr lock: %s\n", strerror(ret));
	exit(1);
    }

    
    track->automations[track->num_automations] = a;
    track->num_automations++;

    a->val_type = ep->val_type;
    if (a->val_type == JDAW_BOOL) a->min = (Value){.bool_v = false};
    else a->min = ep->min;
    if (a->val_type == JDAW_BOOL) a->max = (Value){.bool_v = true};
    else a->max = ep->max;
    /* a->max = ep->max; */
    a->range = jdaw_val_sub(ep->max, ep->min, ep->val_type);
    a->target_val = ep->val;
    Value base_kf_val = ep->default_val;
    automation_insert_keyframe_at(a, 0, base_kf_val);
    endpoint_bind_automation(ep, a);
    track->automation_dropdown->background_color = &colors.dropdown_green;

    Value nullval;
    memset(&nullval, '\0', sizeof(Value));
    user_event_push(
	undo_add_automation,
	redo_add_automation,
	NULL, NULL,
	(void *)a, NULL,
	nullval, nullval, nullval, nullval,
	0, 0, false, false);
	
    return a;

}


Automation *track_add_automation(Track *track, AutomationType type)
{
    if (track->num_automations == MAX_TRACK_AUTOMATIONS) {
	status_set_errstr("Error: reached maximum number of automations per track\n");
	return NULL;
    }
    Automation *a = NULL;
    switch (type) {
    /* case AUTO_VOL: */
    /* 	a = track_add_automation_from_endpoint(track, &track->vol_ep); */
    /* 	break; */
    /* case AUTO_PAN: */
    /* 	a = track_add_automation_from_endpoint(track, &track->pan_ep); */
    /* 	/\* automation_insert_keyframe_at(a, NULL, base_kf_val, 0); *\/ */
    /* 	break; */
    /* case AUTO_FIR_FILTER_CUTOFF: */
    /* 	a = track_add_automation_from_endpoint(track, &track->fir_filter.cutoff_ep); */
    /* 	break; */
    /* case AUTO_FIR_FILTER_BANDWIDTH: */
    /* 	a = track_add_automation_from_endpoint(track, &track->fir_filter.bandwidth_ep); */
    /* 	/\* automation_insert_keyframe_after(a, NULL, base_kf_val, 0); *\/ */
    /* 	break; */
	
    /* case AUTO_DEL_TIME: */
    /* 	if (!track->delay_line.buf_L) delay_line_init(&track->delay_line, track, track->tl->session->proj.sample_rate); */
    /* 	a->val_type = JDAW_INT16; */
    /* 	a->min.int16_v = 1; */
    /* 	a->max.int16_v = DELAY_LINE_MAX_LEN_S * 1000;//track->tl->session->proj.sample_rate * DELAY_LINE_MAX_LEN_S; */
    /* 	a->range.int16_v = a->max.int16_v - 1; */
    /* 	a->target_val = &track->delay_line.len_msec; */
    /* 	base_kf_val.int16_v = 100; */
    /* 	automation_insert_keyframe_at(a, 0, base_kf_val); */
    /* 	endpoint_bind_automation(&track->delay_line.len_ep, a); */
    /* 	/\* automation_insert_keyframe_after(a, NULL, base_kf_val, 0); *\/ */

    /* 	break; */
    /* case AUTO_DEL_AMP: */
    /* 	if (!track->delay_line.buf_L) delay_line_init(&track->delay_line, track, track->tl->session->proj.sample_rate); */
    /* 	a->val_type = JDAW_DOUBLE; */
    /* 	a->target_val = &track->delay_line.amp; */
    /* 	a->min.double_v = 0.0; */
    /* 	a->max.double_v = 1.0; */
    /* 	a->range.double_v = 1.0; */
    /* 	base_kf_val.double_v = 0.5; */
    /* 	automation_insert_keyframe_at(a, 0, base_kf_val); */
    /* 	endpoint_bind_automation(&track->delay_line.amp_ep, a); */
    /* 	/\* automation_insert_keyframe_after(a, NULL, base_kf_val, 0); *\/ */
    /* 	break; */
    /* case AUTO_PLAY_SPEED: */
    /* 	a->val_type = JDAW_FLOAT; */
    /* 	a->min.float_v = 0.05; */
    /* 	a->max.float_v = 5.0; */
    /* 	a->range.float_v = 5.0 - 0.05; */
    /* 	a->target_val = &track->tl->session->playback.play_speed; */
    /* 	base_kf_val.float_v = 1.0f; */
    /* 	automation_insert_keyframe_at(a, 0, base_kf_val); */
    /* 	endpoint_bind_automation(&session->playback.play_speed_ep, a); */
    /* 	/\* automation_insert_keyframe_after(a, NULL, base_kf_val, 0); *\/ */
    /* 	break; */
    default:
	a = track_add_automation_from_endpoint(track, &track->vol_ep);
	break;
    }
    track->automation_dropdown->background_color = &colors.dropdown_green;
    if (a) a->type = type;
    return a;
}


static void track_add_automation_from_api_node(Track *track, APINode *node);

static int add_auto_form(void *mod_v, void *nullarg)
{
    Modal *modal = (Modal *)mod_v;
    ModalEl *el;
    /* AutomationType t = 0; */
    Track *track = NULL;
    APINode *node = NULL;
    int ep_index = -1;
    for (uint8_t i=0; i<modal->num_els; i++) {
	switch ((el = modal->els[i])->type) {
	case MODAL_EL_RADIO:
	    ep_index = ((RadioButton *)el->obj)->selected_item;
	    /* t = ((RadioButton *)el->obj)->selected_item; */
	    track = ((RadioButton *)el->obj)->ep->xarg1;
	    node = ((RadioButton *)el->obj)->ep->xarg2;
	    break;
	default:
	    break;
	}
    }

    if (!track || ep_index < 0) {
	fprintf(stderr, "Error: illegal value for ep_index or no track in auto_add_form\n");
	exit(1);
    }
    if (ep_index < node->num_endpoints) {
	Endpoint *ep = node->endpoints[ep_index];
	for (uint8_t i=0; i<track->num_automations; i++) {
	    if (track->automations[i]->endpoint == ep) {
		status_set_errstr("Track already has an automation of that type");
		return 1;
	    }
	    
	    /* if (track->automations[i]->ep =  */
	    /* if (track->automations[i]->type == t) { */
	    /*     status_set_errstr("Track already has an automation of that type"); */
	    /*     return 1; */
	    /* } */
	}
	Automation *a = track_add_automation_from_endpoint(track, ep);
	if (!a) return 1;
	track_automations_show_all(track);
	track->selected_automation = a->index;
	window_pop_modal(main_win);
    } else {
	/* fprintf(stderr, "Item index is %d; num eps is %d\n", ep_index, track->api_node.num_endpoints); */
	APINode *subnode = node->children[ep_index - node->num_endpoints];
	/* fprintf(stderr, "OK NODE: %p:\n", subnode); */
	window_pop_modal(main_win);
	track_add_automation_from_api_node(track, subnode);
    }
    return 0;
}

static void track_add_automation_from_api_node(Track *track, APINode *node)
{
    Session *session = session_get();
    Layout *lt = layout_add_child(session->gui.layout);
    layout_set_default_dims(lt);
    Modal *m = modal_create(lt);
    /* Automation *a = track_add_automation_internal(track, AUTO_VOL); */
    modal_add_header(m, "Add automation to track", &colors.light_grey, 4);
    static int automation_selection = 0;
    static Endpoint automation_selection_ep = {0};
    if (automation_selection_ep.local_id == NULL) {
	endpoint_init(
	    &automation_selection_ep,
	    &automation_selection,
	    JDAW_INT,
	    "",
	    "",
	    JDAW_THREAD_MAIN,
	    NULL, NULL, NULL,
	    track, NULL,NULL,NULL);
	automation_selection_ep.block_undo = true;
    }
    automation_selection_ep.xarg1 = (void *)track;
    automation_selection_ep.xarg2 = (void *)node;

    /* APINode node = track->api_node; */
    /* Endpoint *items[node->num_endpoints + node->num_children]; */

    void *items[node->num_endpoints + node->num_children];
    const char *item_labels[node->num_endpoints + node->num_children];

    char *dynamic_text = NULL;
    if (node->num_children > 0)
	dynamic_text = calloc(node->num_children * 64, sizeof(char));
    char *child_node_item = dynamic_text;

    for (int i=0; i<node->num_endpoints; i++) {
	Endpoint *ep = node->endpoints[i];
	if (ep->automatable) {
	    items[i] = node->endpoints[i];
	    item_labels[i] = node->endpoints[i]->display_name;
	}
    }
    for (int i=0; i<node->num_children; i++) {
	items[node->num_endpoints + i] = node->children[i];
	int num_chars_printed = snprintf(child_node_item, 64, "%s...", node->children[i]->obj_name);
	child_node_item[num_chars_printed] = '\0';
	item_labels[node->num_endpoints + i] = child_node_item;
	child_node_item += num_chars_printed + 1;
    }
    
    ModalEl *el = modal_add_radio(
	m,
	&colors.light_grey,
	&automation_selection_ep,
	item_labels,
	node->num_endpoints + node->num_children);

    RadioButton *rb = el->obj;
    rb->dynamic_text = dynamic_text;
    modal_add_button(m, "Add", add_auto_form);
    window_push_modal(main_win, m);
    modal_reset(m);
    modal_move_onto(m);

}

void track_add_new_automation(Track *track)
{
    track_add_automation_from_api_node(track, &track->api_node);
    return;
}

bool automation_toggle_read(Automation *a)
{
    Track *track = a->track;
    a->read = !(a->read);
    if (a->read) {
	track->some_automations_read = true;
	textbox_set_background_color(a->read_button->tb, &colors.play_green);
	textbox_set_text_color(a->read_button->tb, &colors.white);
	track->automation_dropdown->background_color = &colors.play_green;
    } else {
	if (a->read_button) {
	    textbox_set_background_color(a->read_button->tb, &colors.grey);
	}
	for (uint8_t i=0; i<track->num_automations; i++) {
	    if (track->automations[i]->read) {
		track->some_automations_read = true;
		return 0;
	    }
	}
	track->some_automations_read = false;
	track->automation_dropdown->background_color = &colors.grey;
	track->some_automations_read = false;

    }
    return 0;
}

static void automation_set_write(Automation *a, bool to)
{
    a->write = to;
    if (a->write) {
	textbox_set_background_color(a->write_button->tb, &colors.red);
	/* textbox_set_text_color(a->write_button->tb, &colors.white); */
    } else {
	/* textbox_set_background_color(a->write_button->tb, &colors.quickref_button_blue); */
	textbox_set_background_color(a->write_button->tb, &colors.grey);
	/* textbox_set_text_color(a->write_button->tb, &colors.black); */
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

LabelStrFn auto_labelfns[] = {
    label_amp_to_dbstr, /* Vol */
    label_pan, /* Pan */
    label_freq_raw_to_hz, /* FIR Filter Cutoff */
    label_freq_raw_to_hz, /* FIR Filter Bandwidth */
    label_msec,
    label_amp_to_dbstr,
    NULL
};

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
	layout_insert_child_at(lt, a->track->layout, a->track->layout->num_children);
	layout_size_to_fit_children_v(a->track->layout, true, 0);

	Layout *tb_lt = layout_get_child_by_name_recursive(lt, "label");
	layout_reset(tb_lt);

	a->label = textbox_create_from_str(
	    a->name,
	    /* a->endpoint->display_name, */
	    /* AUTOMATION_LABELS[a->type], */
	    tb_lt,
	    main_win->mono_bold_font,
	    12,
	    main_win);
	/* a->label->corner_radius = BUBBLE_CORNER_RADIUS; */
	textbox_set_trunc(a->label, false);
	textbox_set_background_color(a->label,  &colors.quickref_button_blue);
	textbox_set_text_color(a->label, &colors.white);
	/* textbox_set_border(a->label, &colors.white, 1); */
	a->label->corner_radius = BUTTON_CORNER_RADIUS;
	textbox_size_to_fit(a->label, 10, 3);
	/* textbox_set_align(a->label, CENTER_LEFT); */
	/* textbox_set_pad(a->label, 4, 0); */
	textbox_reset_full(a->label);

	
	tb_lt = layout_get_child_by_name_recursive(lt, "read");
	Button *button = button_create(
	    tb_lt,
	    "Read",
	    button_read_action,
	    (void *)a,
	    main_win->mono_bold_font,
	    12,
	    &colors.white,
	    &colors.play_green
	    );
        textbox_set_border(button->tb, &colors.black, 1);
	button->tb->corner_radius = MUTE_SOLO_BUTTON_CORNER_RADIUS;
	textbox_set_style(button->tb, BUTTON_DARK);
	a->read_button = button;
	
	tb_lt = layout_get_child_by_name_recursive(lt, "write");
	
	button = button_create(
	    tb_lt,
	    "Write",
	    button_write_action,
	    (void *)a,
	    main_win->mono_bold_font,
	    12,
	    &colors.white,
	    &colors.grey
	    );
	textbox_set_border(button->tb, &colors.black, 1);
	button->tb->corner_radius = MUTE_SOLO_BUTTON_CORNER_RADIUS;
	textbox_set_style(button->tb, BUTTON_DARK);
	a->write_button = button;
	/* a->keyframe_label = label_create(0, a->layout, auto_labelfns[a->type], a, a->val_type, main_win); */
	a->keyframe_label = label_create(0, a->layout, a->endpoint->label_fn, a, a->endpoint->val_type, main_win);
    } else {
	a->layout->h.value = AUTOMATION_LT_H;
	a->layout->y.value = AUTOMATION_LT_Y;
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
	    lt->h.value = 0.0f;
	    lt->y.value = 0.0f;
	    layout_reset(lt);
	}
    }
    track->selected_automation = -1;
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
    if (a->val_type == JDAW_BOOL) {
	k->draw_y_prop = v.bool_v ? 1.0f : 0.0f;
    } else {
	k->draw_y_prop = jdaw_val_div_double(v, a->range, a->val_type);
    }
}

static void keyframe_move(Keyframe *k, int32_t new_pos, Value new_value)
{
    Automation *a = k->automation;
    if (new_pos < k->pos && k > a->keyframes && (k-1)->pos >= new_pos) {
	return;
    } else if (new_pos > k->pos && k < a->keyframes + a->num_keyframes - 1 && (k+1)->pos <= new_pos) {
	return;
    }

    k->pos = new_pos;
    k->value = new_value;
    keyframe_recalculate_m(a, k - a->keyframes);
    if (k > a->keyframes) {
	keyframe_recalculate_m(a, k - 1 - a->keyframes);
    }
    keyframe_set_y_prop(a, k - a->keyframes);
    k->draw_x = timeline_get_draw_x(a->track->tl, new_pos);
    a->track->tl->needs_redraw = true;
}

static inline bool kf_in_range(Automation *a, Keyframe *k, uint16_t start, uint16_t end)
{
    int32_t k_ind = k - a->keyframes;
    return
	k_ind >= start && k_ind < end;
}

static void automation_reset_pointers(Automation *a, uint16_t displace_from, uint16_t displace_until, int32_t displace_by)
{
    /* fprintf(stderr, "\n\nDISPLACE FROM index: %d, until %d\n", displace_from, displace_until); */
    /* fprintf(stderr, "DISPLACE BY: %d\n", displace_by); */
    /* if (a->track->tl->dragging_keyframe > a->keyframes + displace_from && a->track->tl->dragging_keyframe < a->keyframes + displace_until) { */
    if (kf_in_range(a, a->track->tl->dragging_keyframe, displace_from, displace_until)) {
	a->track->tl->dragging_keyframe += displace_by;
    }
    if (kf_in_range(a, a->current, displace_from, displace_until)) {
	a->current += displace_by;
    }
    /* for (uint16_t i=0; i<a->num_kclips; i++) { */
    /* 	/\* fprintf(stderr, "\nIndex %d\n", i); *\/ */
    /* 	KClipRef *kcr = a->kclips + i; */
    /* 	/\* fprintf(stderr, "Resetting pointers in Kclipref\n"); *\/ */
    /* 	/\* fprintf(stderr, "\tFirst: %ld, Last: %ld\n", kcr->first - a->keyframes, kcr->last - a->keyframes); *\/ */
    /* 	/\* kcr->first += displace_by; *\/ */
    /* 	if (kf_in_range(a, kcr->first, displace_from, displace_until)) { */
    /* 	/\* if (kcr->first >= a->keyframes + displace_from && kcr->first < a->keyframes + displace_until) { *\/ */
    /* 	    /\* fprintf(stderr, "\t\tDisplace first!\n"); *\/ */
    /* 	    kcr->first += displace_by; */
    /* 	} */
    /* 	/\* if (kcr->last >= a->keyframes + displace_from && kcr->last < a->keyframes + displace_until) { *\/ */
    /* 	if (kf_in_range(a, kcr->last, displace_from, displace_until)) { */
    /* 	    /\* fprintf(stderr, "\t\tDisplace last!\n"); *\/ */
    /* 	    kcr->last += displace_by; */
    /* 	} */
    /* 	if (kcr->home) { */
    /* 	    KClip *kc = kcr->kclip; */
    /* 	    /\* if (kc->first >= a->keyframes + displace_from) { *\/ */
    /* 	    if (kf_in_range(a, kc->first, displace_from, displace_until)) { */
    /* 		kc->first += displace_by; */
    /* 	    } */
    /* 	} */

    /* 	fprintf(stderr, "\tFirst: %ld, Last: %ld\n", kcr->first - a->keyframes,  kcr->last - a->keyframes); */
    /* 	    /\* exit(1); *\/ */
	
    /* } */
}

static long keyframe_arr_resize(Automation *a)
{
    MAIN_THREAD_ONLY();
    pthread_mutex_lock(&a->keyframe_arr_lock);
    Keyframe *old_base_ptr = a->keyframes;
    a->keyframe_arrlen *= 2;
    a->keyframes = realloc(a->keyframes, a->keyframe_arrlen * sizeof(Keyframe));
    long int migration_bytes = (char *)(a->keyframes) - (char *)old_base_ptr;
    /* fprintf(stderr, "Reallocing kf array new len: %d\n", a->keyframe_arrlen); */
    if (a->current) a->current = (Keyframe *)((char *)a->current +  migration_bytes);
    if (a->track->tl->dragging_keyframe)
	a->track->tl->dragging_keyframe = (Keyframe *)((char *)a->track->tl->dragging_keyframe +  migration_bytes);
    /* for (uint16_t i=0; i<a->num_kclips; i++) { */
    /* 	KClipRef *kcr = a->kclips + i; */
    /* 	/\* Keyframe *old = kcr->first; *\/ */
    /* 	kcr->first = (Keyframe *)((char *)kcr->first + migration_bytes); */
    /* 	kcr->last = (Keyframe *)((char *)kcr->last + migration_bytes); */
    /* 	if (kcr->home) { */
    /* 	    KClip *kc = kcr->kclip; */
    /* 	    kc->first = (Keyframe *)((char *)kc->first + migration_bytes); */
    /* 	}	 */
    /* } */
    pthread_mutex_unlock(&a->keyframe_arr_lock);
    return migration_bytes;
}

Keyframe *automation_insert_keyframe_at(
    Automation *a,
    int32_t pos,
    Value val)
{
    a->track->tl->needs_redraw = true;
    if (a->num_keyframes + 1 >= a->keyframe_arrlen) {
	keyframe_arr_resize(a);
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
    Keyframe *k;
    if (insert_i - 1 > 0 && (k = a->keyframes + insert_i - 1)->pos == pos) {
	keyframe_move(k, pos, val);
	return NULL;
    }
    /* fprintf(stderr, "INSERT at %d\n", pos); */
    /* fprintf(stderr, "Insert i: %d\n", insert_i); */
    uint16_t num_displaced = a->num_keyframes - insert_i;
    if (num_displaced != 0) {
	automation_reset_pointers(a, insert_i, a->num_keyframes, 1);
	memmove(a->keyframes + insert_i + 1, a->keyframes + insert_i, num_displaced * sizeof(Keyframe));
    }
    Keyframe *inserted = a->keyframes + insert_i;
    inserted->automation = a;
    inserted->value = val;
    inserted->pos = pos;
    inserted->draw_x = timeline_get_draw_x(a->track->tl, pos);
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
    if (!a->current) {
	if (a->keyframes[0].pos <= pos) {
	    a->current = a->keyframes;
	} else {
	    goto describe;
	}
    }
    while (a->current > a->keyframes && a->current->pos > pos) {
	/* ops++; */
	a->current--;
    }
    if (a->current->pos > pos) {
	a->current = NULL;
	goto describe;
	/* return; */
    }

    while (a->current < a->keyframes + a->num_keyframes - 1 && (int32_t)pos - a->current->pos > a->current->m_fwd.dx) {
	/* ops++; */
	a->current++;
    }
    
describe:
    /* fprintf(stderr, "OPS: %d\n", ops); */
    return;
}

static Keyframe *automation_check_get_cache(Automation *a, int32_t pos)
{
    pthread_mutex_lock(&a->lock);
    automation_reset_cache(a, pos);
    pthread_mutex_unlock(&a->lock);
    return a->current;
}

static Value automation_value_at(Automation *a, int32_t pos)
{
    Keyframe *current = automation_check_get_cache(a, pos);
    if (!current) return a->keyframes[0].value;
    if (a->val_type == JDAW_BOOL) return current->value;
    int32_t pos_rel = pos - current->pos;
    double scalar = (double)pos_rel / current->m_fwd.dx;
    /* fprintf(stderr, "Scalar: %f\n", scalar); */
    Value ret = jdaw_val_add(current->value, jdaw_val_scale(current->m_fwd.dy, scalar, a->val_type), a->val_type);
    return ret;
}


void automation_get_range(Automation *a, void *dst, int dst_len, int32_t start_pos, float step)
{
    pthread_mutex_lock(&a->keyframe_arr_lock);
    size_t arr_incr = jdaw_val_type_size(a->val_type);
    void *arr_ptr = dst;

    bool rev = step < 0.0;
    int32_t end_pos = start_pos + (int32_t)(dst_len * step);
    double pos = (double)start_pos;
    Keyframe *current = automation_check_get_cache(a, start_pos);
    
    int32_t next_kf_pos = rev ? end_pos - 1 : end_pos + 1;
    
    if (!current) { /* no need to worry about reverse case; if condition will never be met*/
	next_kf_pos = a->keyframes[0].pos;
	while (arr_ptr < dst + dst_len * arr_incr) {
	    jdaw_val_set_ptr(arr_ptr, a->val_type, a->keyframes[0].value);
	    arr_ptr += arr_incr;
	    pos += step;
	    if (pos >= next_kf_pos) {
		current = a->keyframes;
		next_kf_pos = end_pos + 1;
		goto do_remainder_of_array;
		/* break; */
	    }
	}
	pthread_mutex_unlock(&a->keyframe_arr_lock);
	return; /* Completed array before first keyframe */
    }

do_remainder_of_array:
    if (rev) {
	if (current > a->keyframes) {
	    next_kf_pos = (current - 1)->pos;
	}
    } else {
	if (current - a->keyframes < a->num_keyframes - 1) {
	    next_kf_pos = (current + 1)->pos;
	}
    }

    while (arr_ptr < dst + dst_len * arr_incr) {
	int32_t pos_rel = pos - current->pos;
	double scalar = (double)pos_rel / current->m_fwd.dx;
	Value val = jdaw_val_add(current->value, jdaw_val_scale(current->m_fwd.dy, scalar, a->val_type), a->val_type);
	jdaw_val_set_ptr(arr_ptr, a->val_type, val);
	arr_ptr += arr_incr;
	pos += step;
	if ((!rev && pos >= next_kf_pos) || (rev && pos <= next_kf_pos)) {
	    current = rev ? current - 1 : current + 1;
	    if (rev) {
		if (current > a->keyframes) {
		    next_kf_pos = (current - 1)->pos;
		} else {
		    next_kf_pos = end_pos - 1;
		}
	    } else {
		if (current - a->keyframes < a->num_keyframes - 1) {
		    next_kf_pos = (current + 1)->pos;
		} else {
		    next_kf_pos = end_pos + 1;
		}
	    }
	}
    }
    pthread_mutex_unlock(&a->keyframe_arr_lock);
    
}

static void keyframe_remove(Keyframe *k)
{
    if (k == k->automation->keyframes) return; /* Don't remove last keyframe */
    Automation *a = k->automation;
    a->track->tl->needs_redraw= true;
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
    automation_reset_pointers(a, pos, a->num_keyframes, -1);
    memmove(a->keyframes + pos, a->keyframes + pos + 1, num * sizeof(Keyframe));
    a->num_keyframes--;
    keyframe_recalculate_m(a, pos - 1);
    
    pthread_mutex_unlock(&a->lock);
}

/* return true if success */
static bool automation_get_kf_range(Automation *a, int32_t start_pos, int32_t end_pos, uint16_t *start_i_dst, uint16_t *end_i_dst)
{
    /* No range if I/O range is invalid */
    if (end_pos <= start_pos) return false;
    
    Keyframe *current = automation_check_get_cache(a, start_pos);
    /* fprintf(stderr, "\n\n CURRENT (pos %d) i == %ld\n", start_pos, current - a->keyframes); */
    
    /* No range if segment at start_pos is the last keyframe */
    if (current - a->keyframes == a->num_keyframes - 1) return false;
    
    Keyframe *end = automation_check_get_cache(a, end_pos);
    if (!current) {
	if (!end) {
	    return false;
	} else {
	    /* fprintf(stderr, "FIRST CASE: !current, range: %d, %ld\n", 0, end - a->keyframes + 1); */
	    *start_i_dst = 0;
	    *end_i_dst = end - a->keyframes + 1;
	}
    } else {
	/* fprintf(stderr, "SECOND CASE: current. range: %ld, %ld\n", current - a->keyframes, end - a->keyframes + 1); */
	*start_i_dst = current - a->keyframes + 1;
	*end_i_dst = end - a->keyframes + 1;
    }
    return true;
    /* uint16_t current_i = 0; */
    /* uint16_t start_i = 0; */
    /* if (!current) { */
    /* 	fprintf(stderr, "NO CURRENT\n"); */
    /* 	if (a->num_keyframes == 1 || end_pos < a->keyframes->pos) { */
    /* 	    fprintf(stderr, "First false\n"); */
    /* 	    return false; */
    /* 	}else if (a->keyframes->pos < end_pos) { */
    /* 	    start_i = 0; */
    /* 	    /\* current = a->keyframes;uint16_t uint16_t  *\/ */
    /* 	} */
    /* } else { */
    /* 	current_i = current - a->keyframes;     */
    /* 	start_i = current_i + 1; */

    /* } */
    /* if (start_i < a->num_keyframes && a->keyframes[start_i].pos < end_pos) { */
    /* 	start_i = current_i + 1; */
    /* 	uint16_t end_i = start_i; */
    /* 	while (1) { */
    /* 	    if (end_i >= a->num_keyframes) break; */
    /* 	    if (a->keyframes[end_i].pos > end_pos) { */
    /* 		break; */
    /* 	    } */
    /* 	    end_i++; */
    /* 	} */
    /* 	*start_i_dst = start_i; */
    /* 	*end_i_dst = end_i; */
    /* 	return true; */
	
    /* } */
    /* fprintf(stderr, "End condition return false\n"); */
    /* return false; */
}

static void automation_remove_kf_range(Automation *a, uint16_t remove_start_i, uint16_t remove_end_i)
{
    a->track->tl->needs_redraw = true;
    /* uint16_t current_i = automation_check_get_cache(a, start_pos) - a->keyframes; */
    /* uint16_t remove_start_i; */
    /* if ((remove_start_i = current_i + 1) < a->num_keyframes && a->keyframes[remove_start_i].pos < end_pos) { */
    /* 	uint16_t remove_start_i = current_i + 1; */
    /* 	uint16_t remove_end_i = remove_start_i; */
    /* 	while (1) { */
    /* 	    if (remove_end_i > a->num_keyframes) break; */
    /* 	    if (a->keyframes[remove_end_i].pos > end_pos) { */
    /* 		break; */
    /* 	    } */
    /* 	    remove_end_i++; */
    /* 	} */
    /* uint16_t remove_start_i, remove_end_i; */
    /* if (!automation_get_kf_range(a, start_pos, end_pos, &remove_start_i, &remove_end_i)) return; */

    /* Do not allow removal of all keyframes */
    if (remove_start_i == 0 && remove_end_i == a->num_keyframes) {
	remove_start_i++;
    }
    pthread_mutex_lock(&a->lock);
    /* fprintf(stderr, "\n\n"); */
    /* if (remove_start_i > 0) { */
    /* 	fprintf(stderr, "\t(-1) %d\n", a->keyframes[remove_start_i - 1].pos); */
    /* } */
    /* fprintf(stderr, "\t(--) %d\n", start_pos); */
    /* for (uint32_t i=remove_start_i; i<remove_end_i; i++) { */
    /* 	fprintf(stderr, "\t(%d) %d\n", i - remove_start_i, a->keyframes[i].pos); */
    /* } */
    /* fprintf(stderr, "\t(--) %d\n", end_pos); */
    /* if (remove_end_i < a->num_keyframes - 1) { */
	/* fprintf(stderr, "\t(++1) %d\n", a->keyframes[remove_end_i].pos); */
    /* } */
    int32_t remove_count = remove_end_i - remove_start_i;
    int32_t move_count = a->num_keyframes - remove_end_i;
    if (remove_count > 0) {
	if (move_count > 0) {
	    automation_reset_pointers(a, remove_end_i, a->num_keyframes, remove_start_i - remove_end_i);
	    memmove(a->keyframes + remove_start_i, a->keyframes + remove_end_i, move_count * sizeof(Keyframe));
	}
	a->num_keyframes -= remove_count;
	uint16_t current_i = a->current - a->keyframes;
	uint16_t dragging_i = a->track->tl->dragging_keyframe - a->keyframes;
	if (current_i >= remove_start_i && current_i < remove_end_i) {
	    a->current = NULL;
	}
	if (dragging_i >= remove_start_i && dragging_i < remove_end_i) {
	    a->track->tl->dragging_keyframe = NULL;
	}
	if (remove_start_i > 0) {
	    keyframe_recalculate_m(a, remove_start_i - 1);
	}
	keyframe_recalculate_m(a, remove_start_i);
	    
    }
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
    Session *session = session_get();

    /* fprintf(stderr, "INSERT maybe\n"); */
    /* Check for insersection with kclips and exit early if intersect */
    /* for (uint16_t i=0; i<a->num_kclips; i++) { */
    /* 	KClipRef *kcr = a->kclips + i; */
    /* 	if (chunk_end_pos > kcr->pos && pos < kcr->last->pos) { */
    /* 	    a->record_start_pos = kcr->last->pos; */
    /* 	    return NULL; */
    /* 	} */
    /* } */
    uint16_t start_i, end_i;
    if (automation_get_kf_range(a, pos, chunk_end_pos, &start_i, &end_i)) {
	automation_remove_kf_range(a, start_i, end_i);
    }
    static const double diff_prop_thresh = 1e-15;
    /* static const double m_prop_thresh = 0.05; */
    static const double m_prop_thresh = 4.0;
    /* double m_prop_thresh = m_prop_thresh_unscaled / ((double)session->proj.sample_rate / 96000.0); */


    /* Calculate deviance from predicted value */
    Value predicted_value = automation_value_at(a, pos);
    Value diff;

    double ghost_diff_prop = 0.0;
    if (a->ghost_valid) {
	diff = jdaw_val_sub(a->ghost_val, val, a->val_type);
	ghost_diff_prop = fabs(jdaw_val_div_double(diff, a->range, a->val_type));
    }

    /* fprintf(stderr, "\n\nDiff prop: %f, ghost diff prop: %f\n", diff_prop, ghost_diff_prop); */

    bool ghost_inserted = false;
    /* bool current_inserted = false; */

    if (ghost_diff_prop > diff_prop_thresh) {
	/* fprintf(stderr, "CHANGING\n"); */
	if (!a->changing) {
	    a->m_diff_prop_cum = 0.0;
	    /* fprintf(stderr, "\tSTART changing, insert 2\n"); */
	    a->current = automation_insert_keyframe_at(a, a->ghost_pos, a->ghost_val);
	    ghost_inserted = true;
	    a->current = automation_insert_keyframe_at(a, pos, val);
	    a->changing = true;
	    /* fprintf(stderr, "Inserted ghost at %d\n", a->ghost_pos); */
	    goto set_ghost;
	    /* return NULL; */
	    /* pos++; */
	}
    } else {
	/* fprintf(stderr, "Not changing\n"); */
	if (a->changing) {
	    /* fprintf(stderr, "\tSTOP changing, insert one\n"); */
	    /* keyframe_move(a->current, pos, val); */
	    a->current = automation_insert_keyframe_at(a, pos, val);
	    a->changing = false;
	    goto set_ghost;
	    /* if (a->ghost_valid) { */
	    /* 	a->current = automation_insert_keyframe_at(a, a->ghost_pos, a->ghost_val); */
	    /* 	goto se */
	    /* 	ghost_inserted = true; */
	    /* } */
	}
	if (!a->ghost_valid) {
	    /* fprintf(stderr, "\tGhost invalid, insert two\n"); */
	    a->m_diff_prop_cum = 0.0;
	    a->current = automation_insert_keyframe_at(a, pos, val);
	    pos++;
	    /* fprintf(stderr, "Inserting second one ;)\n"); */
	    a->current = automation_insert_keyframe_at(a, pos, val);
	    pos++;
	    /* return NULL; */
	    goto set_ghost;
	    
	    /* current_inserted = true; */
	    /* a->keyframes[a->current].dne+=10; */
	    
	}
	/* a->changing = false; */
    }

    /* if (!current_inserted) { */
    diff = jdaw_val_sub(predicted_value, val, a->val_type);
    double diff_prop = fabs(jdaw_val_div_double(diff, a->range, a->val_type));
    if (diff_prop > diff_prop_thresh) {
	/* fprintf(stderr, "DEVIATION from predicted value\n"); */
	/* a->current = automation_insert_keyframe_at(a, pos, val); */
	if (!ghost_inserted) {
	    if (a->current > a->keyframes + 1) {
		/* Keyframe *p = a->current - 1; */
		Keyframe *pp = a->current - 1;
		Value val_diff = jdaw_val_sub(val, a->current->value, a->val_type);
		/* Value ppm = jdaw_val_scale(pp->m_fwd.dy, 1000.0f / pp->m_fwd.dx, a->val_type); */
		/* Value pm = jdaw_val_scale(val_diff, 1000.0f / (pos - a->current->pos), a->val_type); */
		/* fprintf(stderr, "\tpos - current_pos: %d; pp_m_fwd_dx: %d\n", pos - a->current->pos, pp->m_fwd.dx); */
		/* Divide dx by sample rate to get dx in seconds */
		/* double pp_dx_s = (double)pp->m_fwd.dx / session->proj.sample_rate; */
		/* double p_dx_s = (double)(pos - a->current->pos) / session->proj.sample_rate; */
		/* /\* fprintf(stderr, "\tppdx pdx s: %f, %f\n", pp_dx_s, p_dx_s); *\/ */
		/* fprintf(stderr, "\tppdy, pdy: %f, %f\n",  */
		Value ppm = jdaw_val_scale(pp->m_fwd.dy, (double)session->proj.sample_rate / pp->m_fwd.dx, a->val_type);
		Value pm = jdaw_val_scale(val_diff, (double)session->proj.sample_rate / (double)(pos - a->current->pos), a->val_type);

		/* fprintf(stderr, "\tpm, ppm: %f, %f\n", pm.float_v, ppm.float_v); */
		diff = jdaw_val_sub(ppm, pm, a->val_type);
		a->m_diff_prop_cum += fabs(jdaw_val_div_double(diff, a->range, a->val_type));
		/* fprintf(stderr, "DIFF PROP: %f\n", diff_prop); */
		if (a->m_diff_prop_cum < m_prop_thresh) {//&& !jdaw_val_is_zero(ppm, a->val_type) && jdaw_val_sign_match(ppm, pm, a->val_type)) {
		    /* fprintf(stderr, "\tMOVING one\n"); */
		    keyframe_move(a->current, pos, val);
		} else {
		    a->m_diff_prop_cum = 0.0;
		    /* fprintf(stderr, "\tINSERT normal\n"); */
		    a->current = automation_insert_keyframe_at(a, pos, val);
		}
	    }
	}

    }
    /* } */
set_ghost:
    a->ghost_val = val;
    a->ghost_pos = pos;
    a->ghost_valid = true;

    return NULL;
}

void automation_do_write(Automation *a, Value v, int32_t pos, int32_t end_pos, float step)
{
    /* Value v; */
    /* jdaw_val_set(&v, a->val_type, a->target_val); */
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
	a->keyframes[i].draw_x = timeline_get_draw_x(a->track->tl, a->keyframes[i].pos);
    }
}

Keyframe *automation_get_segment(Automation *a, int32_t at)
{
    Keyframe *k = a->keyframes;
    if (k->pos > at) return NULL;
    while (k) {
	if (k - a->keyframes == a->num_keyframes - 1) {
	    return k;
	} else if ((k+1)->pos > at) {
	    return k;
	}
	k++;
    }
    return NULL;
    /* if (a->keyframes[0].pos > at) return NULL; */
    /* Keyframe *k = a->keyframes; */
    /* while (k < a->keyframes + a->num_keyframes - 1 && k->pos < at) k++; */
    /* if (k == a->keyframes + a->num_keyframes - 1) return k; */
    /* /\* Suspicious for case where k == a->keyframes *\/ */
    /* return k - 1; */

}

static inline bool segment_intersects_screen(int a, int b)
{
    return (b >= 0 && main_win->w_pix >= a);
   
}

static inline bool x_onscreen(int x)
{
    return (x > 0 && x < main_win->w_pix);
}


static void keyframe_draw(int x, int y)
{
    int dim = SYMBOL_STD_DIM * main_win->dpi_scale_factor;
    SDL_Rect dst = {x - dim / 4, y - dim / 4, dim / 2, dim / 2};
    symbol_draw(SYMBOL_TABLE[SYMBOL_KEYFRAME], &dst);
}

/* This function assumes "current" pointer has been set or unset appropriately elsewhere */
Value inline automation_get_value(Automation *a, int32_t pos, float direction)
{
    return automation_value_at(a, pos);
}

/* void automation_get_value_range(Automation *a, int32_t start_pos, void *dst_arr, int num_items, float step) */
/* { */
    
/* } */

void automation_draw(Automation *a)
{
    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(automation_bckgrnd));
    SDL_RenderFillRect(main_win->rend, a->bckgrnd_rect);
        
    SDL_RenderSetClipRect(main_win->rend, a->bckgrnd_rect);
    /* Draw KCLIPS */

    /* static SDL_Color homebckgrnd = {200, 200, 255, 50}; */
    /* static SDL_Color bckgrnd = {200, 255, 200, 50}; */
    /* static SDL_Color brdr = {200, 200, 200, 200}; */
    
    /* /\* SDL_SetRenderDrawColor(main_win->rend, 100, 100, 255, 100); *\/ */
    /* SDL_Rect kcliprect = {0, a->layout->rect.y, 0, a->layout->rect.h}; */
    /* for (uint16_t i=0; i<a->num_kclips; i++) { */
    /* 	/\* fprintf(stderr, "Draw kclip %d\n", i); *\/ */
    /* 	KClipRef *kcr = a->kclips + i; */
    /* 	kcliprect.x = timeline_get_draw_x(kcr->first->pos); */
    /* 	kcliprect.w = timeline_get_draw_x(kcr->last->pos) - kcliprect.x; */
    /* 	/\* SDL_RenderDrawLine(main_win->rend, x, 0, x, 800); *\/ */
    /* 	if (kcr->home) { */
    /* 	    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(homebckgrnd)); */
    /* 	} else { */
    /* 	    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(bckgrnd)); */
    /* 	} */
    /* 	SDL_RenderFillRect(main_win->rend, &kcliprect); */
    /* 	SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(brdr)); */
    /* 	SDL_RenderDrawRect(main_win->rend, &kcliprect); */
    /* } */

    
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
	if (a->val_type == JDAW_BOOL) {
	    if (k->value.bool_v) {
		SDL_SetRenderDrawColor(main_win->rend, 0, 255, 0, 30);
	    } else {
		SDL_SetRenderDrawColor(main_win->rend, 255, 0, 0, 30);
	    }
	    SDL_Rect fill_rect;
	    fill_rect.x = k->draw_x;
	    if (k - a->keyframes < a->num_keyframes - 1) {
		fill_rect.w = (k + 1)->draw_x - k->draw_x;
	    } else {
		fill_rect.w = main_win->w_pix - k->draw_x;
	    }
	    fill_rect.y = a->layout->rect.y;
	    fill_rect.h = a->layout->rect.h;
	    SDL_RenderFillRect(main_win->rend, &fill_rect);
	} else {
	    if (last_y != 0 && segment_intersects_screen(prev->draw_x, k->draw_x)) {
		SDL_RenderDrawLine(main_win->rend, prev->draw_x, last_y, k->draw_x, y);
	    }
	    if (k == first && k->draw_x > 0) {
		SDL_RenderDrawLine(main_win->rend, 0, y, k->draw_x, y);
	    }
	    if (k == last && k->draw_x < main_win->w_pix) {
		SDL_RenderDrawLine(main_win->rend, k->draw_x, y, main_win->w_pix, y);
	    }
	}
	last_y = y;
	/* k = k->next; */
    }
    SDL_RenderSetClipRect(main_win->rend, NULL);
    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(automation_console_bckgrnd));
    SDL_RenderFillRect(main_win->rend, a->console_rect);

    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(a->track->color));
    SDL_RenderFillRect(main_win->rend, a->color_bar_rect);

    /* SDL_RenderSetClipRect(main_win->rend, a->console_rect); */
    if (a->index != a->track->selected_automation) {
	SDL_RenderSetClipRect(main_win->rend, a->console_rect);
	textbox_draw(a->label);
	SDL_RenderSetClipRect(main_win->rend, NULL);
    } else {
	textbox_draw(a->label);
    }
    button_draw(a->read_button);
    button_draw(a->write_button);
    label_draw(a->keyframe_label);

    /* SDL_RenderSetClipRect(main_win->rend, NULL); */

    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(colors.grey));
    SDL_Rect ltrect = a->layout->rect;
    ltrect.x = a->console_rect->x;
    SDL_RenderDrawRect(main_win->rend, &ltrect);
    ltrect.x += 1;
    ltrect.y += 1;
    ltrect.h -= 2;
    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(colors.black));
    SDL_RenderDrawRect(main_win->rend, &ltrect);
		     
}

static void automation_editing(Automation *a, Keyframe *k, int x, int y)
{
    label_move(a->keyframe_label, x - 50, y - 50);
    label_reset(a->keyframe_label, k->value);   
}


static void keyframe_move_coords(Keyframe *k, int x, int y)
{
    /* fprintf(stdout, "MOVE to %d, %d\n", x, y); */
    /* Keyframe *k = a->keyframes + i; */
    Automation *a = k->automation;
    int32_t abs_pos = timeline_get_abspos_sframes(a->track->tl, x);
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
	keyframe_move(k, abs_pos, v);
	automation_editing(a, k, x, y);
    }
}



NEW_EVENT_FN(undo_redo_move_keyframe, "undo/redo move keyframe")
    Automation *a = (Automation *)obj1;
    a->track->tl->needs_redraw = true;
    uint16_t index = *((uint16_t *)obj2);
    Keyframe *k = a->keyframes + index;
    keyframe_move(k, val1.int32_v, val2);
}


static void automation_set_dragging_kf(Keyframe *k)
{
    Timeline *tl = k->automation->track->tl;
    tl->dragging_keyframe = k;
    tl->dragging_kf_cache_pos = k->pos;
    tl->dragging_kf_cache_val = k->value;
}

void automation_unset_dragging_kf(Timeline *tl)
{
    Keyframe *k;
    if ((k = tl->dragging_keyframe)) {
	if (k->pos != tl->dragging_kf_cache_pos || !jdaw_val_equal(k->value, tl->dragging_kf_cache_val, k->automation->val_type)) {
	    uint16_t *index = malloc(sizeof(int32_t));
	    *index = k - k->automation->keyframes;
	    Value cache_pos_val = {.int32_v = tl->dragging_kf_cache_pos};
	    Value new_pos_val = {.int32_v = k->pos};
	    user_event_push(
		
		undo_redo_move_keyframe,
		undo_redo_move_keyframe,
		NULL, NULL,
		(void *)k->automation, (void *)index,
		cache_pos_val, tl->dragging_kf_cache_val,
		new_pos_val, k->value,
		0, 0, false, true); 
	}
	tl->dragging_keyframe = NULL;
    }
}


bool automations_triage_motion(Timeline *tl, int xrel, int yrel)
{
    Keyframe *k = tl->dragging_keyframe;
    if (k) {
	Automation *a = k->automation;
	if (main_win->i_state & I_STATE_MOUSE_L) {
	    if (main_win->i_state & I_STATE_SHIFT && !(main_win->i_state & I_STATE_CMDCTRL)) {
		if (k > a->keyframes) {
		    keyframe_move(k, timeline_get_abspos_sframes(a->track->tl, main_win->mousep.x), (k-1)->value);
		}
	    } else if (main_win->i_state & I_STATE_SHIFT && main_win->i_state & I_STATE_CMDCTRL) {
	        if (k > a->keyframes) {
		    keyframe_move_coords(k, main_win->mousep.x, main_win->mousep.y);
		    keyframe_move(k, (k-1)->pos + 1, k->value);
		}
	    } else {
		keyframe_move_coords(k, main_win->mousep.x, main_win->mousep.y);
	    }
	    TEST_FN_CALL(automation_keyframe_order, a);
	    return true;
	} else {
	    automation_unset_dragging_kf(tl);
	    /* if (tl->dragging_keyframe) { */
	    /* 	tl->dragging_keyframe = NULL; */
	    /* } */
	}
    }
    return false;
}


/* void kclipref_set_pos(KClipRef *kcr, int32_t new_pos) */
/* { */
/*     Automation *a = kcr->automation; */
/*     int32_t len_sframes = kcr->last->pos - kcr->first->pos; */
/*     int32_t new_end_pos = new_pos + len_sframes; */

/*     /\* Remove (overwrite) keyframes in destination range *\/ */
/*     automation_remove_kf_range(a, new_pos, new_end_pos + 1); */

/*     /\* Keyframe positions will move by this amount *\/ */
/*     int32_t pos_diff = new_pos - kcr->pos; */


/*     /\* Make a copy of the KCR *\/ */
/*     uint16_t kcr_len = kcr->last - kcr->first + 1; */
/*     Keyframe cpy[kcr_len]; */
/*     memcpy(cpy, kcr->first, kcr_len * sizeof(Keyframe)); */

/*     /\* Record indices of current KCR *\/ */
/*     uint16_t kcr_first_i = kcr->first - a->keyframes; */
/*     uint16_t kcr_last_i = kcr->last - a->keyframes; */
/*     /\* uint16_t move_len_kfs = a->num_keyframes - kcr_last_i; *\/ */


/*     fprintf(stderr, "KEYFRAMES BEFORE REMOVE KCLIP\n"); */
/*     fprintf(stderr, "(kcr index range is %d - %d\n", kcr_first_i, kcr_last_i); */
/*     for (uint16_t i =0; i<a->num_keyframes; i++) { */
/* 	fprintf(stderr, "\t%d pos: %d\n", i, a->keyframes[i].pos); */
/*     } */
/*     automation_remove_kf_range(a, kcr->first->pos - 1, kcr->last->pos + 1); */
/*     /\* Fill space left by clip *\/ */
/*     /\* automation_reset_pointers(a, kcr_first_i, a->num_keyframes, -1 * kcr_len); *\/ */
/*     /\* memmove(a->keyframes + kcr_first_i, a->keyframes + kcr_last_i, move_len_kfs * sizeof(Keyframe)); *\/ */
    
/*     fprintf(stderr, "KEYFRAMES AFTER MOVE\n"); */
/*     for (uint16_t i=0; i<a->num_keyframes; i++) { */
/* 	fprintf(stderr, "\t%d pos: %d\n", i, a->keyframes[i].pos); */
/*     } */
    
/*     /\* Insert clip back into array *\/ */
/*     Keyframe *insertion_seg = automation_get_segment(a, new_pos); */
/*     uint16_t insertion_i = insertion_seg - a->keyframes + 1; */
/*     fprintf(stderr, "INSERTION I: %d, num: %d, arrlen: %d, new end i: %d\n", insertion_i, a->num_keyframes, a->keyframe_arrlen, insertion_i + kcr_len); */
/*     int32_t num_to_move = a->num_keyframes - insertion_i; */
/*     if (num_to_move > 0) { */
/* 	automation_reset_pointers(a, insertion_i, a->num_keyframes, kcr_len); */
/* 	memmove(a->keyframes + insertion_i + kcr_len, a->keyframes + insertion_i, num_to_move * sizeof(Keyframe)); */
/*     } */
/*     memcpy(a->keyframes + insertion_i, cpy, kcr_len * sizeof(Keyframe)); */
/*     /\* if (insertion_i > 0)  *\/     */
/*     a->num_keyframes += kcr_len; */
/*     kcr->first = a->keyframes + insertion_i; */
/*     kcr->last = a->keyframes + insertion_i + kcr_len - 1; */
/*     Keyframe *k = kcr->first; */
/*     while (k <= kcr->last) { */
/* 	k->pos += pos_diff; */
/* 	k->draw_x = timeline_get_draw_x(k->pos); */
/* 	k++; */
/*     } */
/*     if (kcr->first > a->keyframes) { */
/* 	keyframe_recalculate_m(a, kcr->first - a->keyframes - 1); */
/*     } */
/*     keyframe_recalculate_m(a, kcr->last - a->keyframes); */
/*     fprintf(stderr, "KEYFRAMES AT END\n"); */
/*     for (uint16_t i =0; i<a->num_keyframes; i++) { */
/* 	fprintf(stderr, "\t%d pos: %d\n", i, a->keyframes[i].pos); */
/*     } */

/* } */

NEW_EVENT_FN(redo_insert_keyframe, "redo insert keyframe")
    Automation *a = (Automation *)obj1;
    int32_t pos = val1.int32_v;
    Value val = val2;
    automation_insert_keyframe_at(a, pos, val);
    /* free(self->obj2); */
    /* self->obj2 = malloc(sizeof(int32_t)); */
    /* *((int32_t *)self->obj2) = k - a->keyframes; */
}
NEW_EVENT_FN(undo_insert_keyframe, "undo insert keyframe")
    Automation *a = (Automation *)obj1;
    /* Keyframe *k = (Keyframe *)obj2; */
    /* int32_t index = *(int32_t *)obj2; */
    int32_t index = val1.int32_v;
    /* automation_remove_kf_range(a, k->pos - 1, k->pos); */
    keyframe_remove(a->keyframes + index);
}

NEW_EVENT_FN(undo_redo_automation_write, "undo/redo automation write")
    Automation *a = (Automation *)obj1;
    Keyframe *cached_arr = (Keyframe *)obj2;
    uint16_t num_keyframes = val1.uint16_v;
    uint16_t cache_start_i = val2.uint16_v;
    automation_remove_kf_range(a, 0, a->num_keyframes);
    memcpy(a->keyframes, cached_arr + cache_start_i, num_keyframes * sizeof(Keyframe));
    a->num_keyframes = num_keyframes;
    automation_clear_cache(a);
}

static void automation_write_set_undo_cache(Automation *a)
{
    if (a->undo_cache) free(a->undo_cache);
    a->undo_cache = malloc(a->num_keyframes * sizeof(Keyframe));
    memcpy(a->undo_cache, a->keyframes, a->num_keyframes * sizeof(Keyframe));
    a->undo_cache_len.uint16_v = a->num_keyframes;
    a->record_start_pos = a->track->tl->play_pos_sframes;
}


static void automation_push_write_event(Automation *a)
{

    uint16_t new_start_pos = a->undo_cache_len.uint16_v;
    Keyframe *undo_cache = malloc((new_start_pos + a->num_keyframes) * sizeof(Keyframe));
    /* a->undo_cache = realloc(a->undo_cache, (new_start_pos + a->num_keyframes) * sizeof(Keyframe));	 */
    memcpy(undo_cache, a->undo_cache, a->undo_cache_len.int32_v * sizeof(Keyframe));
    memcpy(undo_cache + new_start_pos, a->keyframes, a->num_keyframes * sizeof(Keyframe));
    Value redo_cache_len = {.uint16_v = a->num_keyframes};
    Value zero = {.uint16_v = 0};
    user_event_push(
	
	undo_redo_automation_write,
	undo_redo_automation_write,
	NULL, NULL,
	(void *)a,
	(void *)undo_cache,
	a->undo_cache_len, zero,
	redo_cache_len, a->undo_cache_len,
	0, 0, false, true);
}

bool automation_triage_click(uint8_t button, Automation *a)
{
    if (!a->shown) return false;
    int click_tolerance = 10 * main_win->dpi_scale_factor;
    /* int32_t epsilon = 10000; */
    if (SDL_PointInRect(&main_win->mousep, &a->layout->rect)) {
	if (button_click(a->read_button, main_win)) return true;
	if (button_click(a->write_button, main_win)) {
	    if (a->write) {
		automation_write_set_undo_cache(a);
	    } else {
		automation_push_write_event(a);
	    }
	    return true;
	}
	if (SDL_PointInRect(&main_win->mousep, a->bckgrnd_rect)) {
	    /* if (main_win->i_state & I_STATE_CMDCTRL) { */
	    int32_t clicked_pos_sframes = timeline_get_abspos_sframes(a->track->tl, main_win->mousep.x);
	    /* uint16_t insertion_i = automation_get_segment(a, clicked_pos_sframes); */
	    Keyframe *insertion = automation_get_segment(a, clicked_pos_sframes);
	    Keyframe *next = NULL;
	    int left_kf_dist = click_tolerance + 1;
	    int right_kf_dist = click_tolerance + 1;
	    if (!insertion) next = a->keyframes;
	    else if (insertion < a->keyframes + a->num_keyframes - 1) {
		next = insertion + 1;
	    }
	    /* fprintf(stderr, "insertion: %ld, next: %ld\n", insertion - a->keyframes, next - a->keyframes); */
	    /* if (insertion_i < a->num_keyframes - 1) next = insertion + 1; */
	    if (insertion) left_kf_dist = abs(insertion->draw_x - main_win->mousep.x);
	    if (next) right_kf_dist = abs(next->draw_x - main_win->mousep.x);	
	    /* fprintf(stderr, "LDIST %d RDIST %d\n", left_kf_dist, right_kf_dist); */

	    if (left_kf_dist < click_tolerance && left_kf_dist < right_kf_dist && !(main_win->i_state & I_STATE_SHIFT)) {
		/* a->track->tl->dragging_keyframe = insertion; */
		automation_set_dragging_kf(insertion);
		keyframe_move_coords(insertion, main_win->mousep.x, main_win->mousep.y);
		/* fprintf(stderr, "Left kf clicked\n"); */
	    } else if (right_kf_dist < click_tolerance && !(main_win->i_state & I_STATE_SHIFT)) {
		automation_set_dragging_kf(next);
		/* a->track->tl->dragging_keyframe = next; */
		/* a->track->tl->dragging_kf_cache_pos = next->pos; */
		/* a->track->tl->dragging_kf_cache_val = next->value; */
		keyframe_move_coords(next, main_win->mousep.x, main_win->mousep.y);
		/* fprintf(stderr, "Right kf clicked\n"); */
	    } else if (main_win->i_state & I_STATE_CMDCTRL) {
		double val_prop = (double)(a->bckgrnd_rect->y + a->bckgrnd_rect->h - main_win->mousep.y) / a->bckgrnd_rect->h;
		Value range_scaled = jdaw_val_scale(a->range, val_prop, a->val_type);
		Value v = jdaw_val_add(a->min, range_scaled, a->val_type);
		/* uint16_t new_i = automation_insert_keyframe_at(a, clicked_pos_sframes, v); */
		/* a->track->tl->dragging_keyframe = a->keyframes + new_i; */
		Keyframe *k = a->track->tl->dragging_keyframe = automation_insert_keyframe_at(a, clicked_pos_sframes, v);
		if (k) {
		    Value nullval;
		    Value indexval;
		    indexval.int32_v = k - a->keyframes;
		    Value posval = {.int32_v = k->pos};
		    /* int32_t *index = malloc(sizeof(int32_t)); */
		    /* *index = k - a->keyframes; */
		    memset(&nullval, '\0', sizeof(Value));
		    user_event_push(
			
			undo_insert_keyframe,
			redo_insert_keyframe,
			NULL, NULL,
			a, NULL,
			indexval, nullval,
			posval, k->value,
			0, 0, false, false);
		    automation_set_dragging_kf(k);
		    return true;
		}
		/* a->track->tl->dragging_kf_cache_pos = k->pos; */
		/* a->track->tl->dragging_kf_cache_val = k->value; */
	    } else {
		automation_unset_dragging_kf(a->track->tl);
		/* a->track->tl->dragging_keyframe = NULL; */
	    }
	    return true;
	}
    }
    return false;
}

void user_tl_play(void *nullarg);
void user_tl_pause(void *nullarg);

/* static void kclip_set_pos_rel(KClip *kc) */
/* { */
/*     Keyframe *k = kc->first; */
/*     int32_t init_pos = k->pos; */
/*     while (k < kc->first + kc->len) { */
/* 	k->pos_rel = k->pos - init_pos; */
/* 	k++; */
/*     } */
/* } */

/* static KClip *record_kclip(Automation *a, int32_t start_pos, int32_t end_pos) */
/* { */
/*     if (end_pos <= start_pos) return NULL; */

/*     end_pos += session->proj.sample_rate / 60.0; */
/*     Keyframe *first = automation_get_segment(a, start_pos); */
/*     if (!first) first = a->keyframes; */
/*     if (first < a->keyframes + a->num_keyframes) first++; else return NULL; */
/*     Keyframe *last = automation_get_segment(a, end_pos); */
/*     uint16_t len = 1 + last - first; */
/*     if (first == last) return NULL; */
/*     fprintf(stderr, "\n\nSTART pos: %d, end pos: %d\n", start_pos, end_pos); */
/*     fprintf(stderr, "FIRST POS: %d, LAST POS: %d\n", first->pos, last->pos); */
/*     fprintf(stderr, "next pos: %d\n", (last + 1)->pos); */
    
/*     KClip *kc = calloc(1, sizeof(KClip)); */
/*     kc->first = first; */
/*     kc->len = len; */
/*     return kc; */
/*     /\* return NULL; *\/ */
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

/* void _____automation_insert_kclipref(Automation *a, KClip *kc, int32_t pos) */
/* { */

/*     while (a->num_keyframes + kc->len >= a->keyframe_arrlen) { */
/* 	keyframe_arr_resize(a); */
/*     } */
/*     a->num_keyframes += kc->len; */
/*     Keyframe *insert_after = automation_get_segment(a, pos); */
/*     uint16_t num_to_move = a->keyframes + a->num_keyframes - insert_after; */
/*     Keyframe *dst = insert_after + kc->len + 1; */
/*     memmove(dst, insert_after + 1, num_to_move * sizeof(Keyframe)); */
/*     memcpy(insert_after + 1, kc->first, kc->len * sizeof(Keyframe)); */
/*     Keyframe *k = insert_after + 1; */
/*     while (k < dst) { */
/* 	k->pos = pos + k->pos_rel; */
/* 	keyframe_recalculate_m(a, k - a->keyframes); */
/* 	k->draw_x = timeline_get_draw_x(k->pos); */
/* 	k++; */
	
/*     } */
/*     if (a->num_kclips + 1 >= a->kclips_arr_len) { */
/* 	a->kclips_arr_len *= 2; */
	
/* 	a->kclips = realloc(a->kclips, a->kclips_arr_len * sizeof(KClipRef)); */
/*     } */
/*     KClipRef *kcr = a->kclips + a->num_kclips; */

/*     kcr->first = insert_after + 1; */
/*     kcr->last = kcr->first + kc->len; */
/*     kcr->pos = pos; */
/*     kcr->automation = a; */
/*     a->num_kclips++; */
/*     /\* Keyframe *k = kc->first; *\/ */
/*     /\* int i=0; *\/ */
/*     /\* while (k && k != kc->last) { *\/ */
/*     /\* 	fprintf(stderr, "K %d: %p\n", i, k); *\/ */
/*     /\* 	k = k->next; *\/ */
/*     /\* 	i++; *\/ */
/*     /\* } *\/ */
    
/*     /\* Keyframe *prev = automation_get_segment(a, pos); *\/ */
/*     /\* Keyframe *next = NULL; *\/ */
/*     /\* if (prev) next = prev->next; *\/ */
/*     /\* if (a->num_kclips == a->kclips_arr_len) { *\/ */
/*     /\* 	a->kclips_arr_len *= 2; *\/ */
/*     /\* 	a->kclips = realloc(a->kclips, a->kclips_arr_len * sizeof(KClipRef)); *\/ */
/*     /\* } *\/ */
/*     /\* KClipRef *kcr = a->kclips + a->num_kclips; *\/ */
/*     /\* kcr->kclip = kc; *\/ */
/*     /\* kcr->home = false; *\/ */
/*     /\* kcr->automation = a; *\/ */
/*     /\* Keyframe *to_copy = kc->first; *\/ */
/*     /\* bool first = true; *\/ */
/*     /\* int32_t last_pos = INT32_MIN; *\/ */
/*     /\* i = 0; *\/ */
/*     /\* while (to_copy) { *\/ */
/*     /\* 	if (to_copy->pos < last_pos ) { *\/ */
/*     /\* 	    fprintf(stderr, "CYCLE DETECTED!!!! Exiting!\n"); *\/ */
/*     /\* 	    exit(1); *\/ */
/*     /\* 	} *\/ */
/*     /\* 	last_pos = to_copy->pos; *\/ */
/*     /\*     Keyframe *cpy = calloc(1, sizeof(Keyframe)); *\/ */
/*     /\* 	memcpy(cpy, to_copy, sizeof(Keyframe)); *\/ */
/*     /\* 	if (first) { *\/ */
/*     /\* 	    kcr->first = cpy; *\/ */
/*     /\* 	    first = false; *\/ */
/*     /\* 	} *\/ */
/*     /\* 	cpy->prev = prev; *\/ */
/*     /\* 	prev->next = cpy; *\/ */
/*     /\* 	cpy->next = NULL; *\/ */
/*     /\* 	cpy->pos = to_copy->pos_rel + pos; *\/ */
/*     /\* 	fprintf(stderr, "To Copy %d: %p vs %p at pos %d (dist to last: %d)\n", i, to_copy, kc->last, to_copy->pos, kc->last->pos_rel + pos - to_copy->pos); *\/ */
/*     /\* 	i++; *\/ */
/*     /\* 	cpy->draw_x = timeline_get_draw_x(cpy->pos); *\/ */
/*     /\* 	/\\* keyframe_set_y_prop(cpy); *\\/ *\/ */
/*     /\* 	cpy->automation = a; *\/ */
/*     /\* 	keyframe_set_y_prop(cpy); *\/ */
/*     /\* 	prev = cpy; *\/ */
/*     /\* 	if (to_copy == kc->last) { *\/ */
/*     /\* 	    kcr->last = cpy; *\/ */
/*     /\* 	    break; *\/ */
/*     /\* 	} *\/ */
/*     /\* 	to_copy = to_copy->next; *\/ */
/*     /\* } *\/ */
/*     /\* if (next) { *\/ */
/*     /\* 	int32_t end_pos = kcr->last->pos; *\/ */
/*     /\* 	while (next && next->pos < end_pos) { *\/ */
/*     /\* 	    Keyframe *to_remove = next; *\/ */
/*     /\* 	    fprintf(stderr, "removing %p at pos %d, before %d\n", to_remove, to_remove->pos,  end_pos); *\/ */
/*     /\* 	    next = next->next; *\/ */
/*     /\* 	    if (to_remove == a->last) a->last = NULL; *\/ */
/*     /\* 	    keyframe_remove(to_remove); *\/ */
/*     /\* 	} *\/ */
/*     /\* 	kcr->last->next = next; *\/ */
/*     /\* 	next->prev = kcr->last; *\/ */
/*     /\* } else { *\/ */
/*     /\* 	kcr->last->next = NULL; *\/ */
/*     /\* } *\/ */
/*     /\* /\\* Sanity check *\\/ *\/ */

/*     /\* /\\* Keyframe *k = a->first; *\\/ *\/ */
/*     /\* /\\* int ki = 0; *\\/ *\/ */
/*     /\* /\\* while (k) { *\\/ *\/ */
/*     /\* /\\* 	fprintf(stderr, "K (%d): %p, pos: %d\n", ki, k, k->pos); *\\/ *\/ */
/*     /\* /\\* 	ki++; *\\/ *\/ */
/*     /\* /\\* 	k = k->next; *\\/ *\/ */
/*     /\* /\\* 	if (ki > 200) { *\\/ *\/ */
/*     /\* /\\* 	    exit(1); *\\/ *\/ */
/*     /\* /\\* 	} *\\/ *\/ */
/*     /\* /\\* } *\\/ *\/ */
/*     /\* a->num_kclips++; *\/ */
/*     /\* if (!a->last || (kcr->last->next == NULL)) { *\/ */
/*     /\* 	a->last = kcr->last; *\/ */
/*     /\* } *\/ */
/*     /\* /\\* automation_reset_keyframe_x(a); *\\/ *\/ */
/* } */

/* ONLY move by values smaller than the kcr width; otherwise, will delete lots of keyframes */
/* void ___kclipref_move(KClipRef *kcr, int32_t move_by) */
/* { */
/*     /\* fprintf(stderr, "\n\nNEXT TO LAST POS: %d\n", (kcr->last - 1)->pos); *\/ */
/*     /\* fprintf(stderr, "LAST POS %d\n", kcr->last->pos); *\/ */
/*     /\* fprintf(stderr, "NEXT POS: %d\n", (kcr->last + 1)->pos); *\/ */
/*     int32_t remove_range_start, remove_range_end; */
/*     Automation *a = kcr->automation; */
/*     if (move_by > 0) { */
/* 	remove_range_start = kcr->last->pos + 1; */
/* 	remove_range_end = remove_range_start + move_by; */
/*     } else if (move_by < 0) { */
/* 	remove_range_start = kcr->pos - move_by; */
/* 	remove_range_end = kcr->pos - 1; */
/*     } else { */
/* 	return; */
/*     } */
/*     /\* fprintf(stderr, "REMOVING KF RANGE: %d - %d\n", remove_range_start, remove_range_end); *\/ */
/*     automation_remove_kf_range(a, remove_range_start, remove_range_end); */
/*     kcr->pos += move_by; */
/*     Keyframe *k = kcr->first; */
/*     while (1) { */
/* 	k->pos += move_by; */
/* 	k->draw_x = timeline_get_draw_x(k->pos); */
/* 	if (k == kcr->last) break; */
/* 	k++; */
/*     } */
/*     keyframe_recalculate_m(a, kcr->last - a->keyframes); */
/* } */


bool automation_record(Automation *a)
{
    /* static int32_t start_pos = 0; */
    /* static Keyframe *cache; */
    /* static Value cache_len; */
    bool write = automation_toggle_write(a);
    if (write) {
	automation_clear_cache(a);
	automation_write_set_undo_cache(a);
	user_tl_play(NULL);
	timeline_play_speed_set(1.0);
    } else {
	/* TODO: KClips */
	/* Fuck KClips */
	
	/* fprintf(stderr, "\n\nFINISH RECORD, START AND END: %d, %d\n", a->record_start_pos, a->track->tl->play_pos_sframes); */
	/* KClip *kc = record_kclip(a, a->record_start_pos, a->track->tl->play_pos_sframes); */
	/* if (kc) { */
	/*     kclip_set_pos_rel(kc); */
	/*     if (a->num_kclips + 1 >= a->kclips_arr_len) { */
	/* 	a->kclips_arr_len *= 2; */
	/* 	a->kclips = realloc(a->kclips, a->kclips_arr_len * sizeof(KClipRef)); */
	/*     } */
	/*     KClipRef *kcr = a->kclips + a->num_kclips; */
	/*     kcr->pos = kc->first->pos; */
	/*     kcr->kclip = kc; */
	/*     kcr->automation = a; */
	/*     kcr->home = true; */
	/*     kcr->first = kc->first; */
	/*     kcr->last = kc->first + kc->len - 1; */
	/*     /\* kcr->last = kc->last; *\/ */
	/*     a->num_kclips++; */

	/*     fprintf(stderr, "\n\n\nKCLIPS:\n"); */
	/*     for (uint16_t i=0; i<a->num_kclips; i++) { */
	/* 	KClipRef *kcr = a->kclips + i; */
	/* 	fprintf(stderr, "\t%d: %p, first: %d, last: %d\n", i, a->kclips + i, kcr->first->pos, kcr->last->pos); */
	/*     } */
	/* } */

	automation_push_write_event(a);
	/* uint16_t new_start_pos = a->undo_cache_len.uint16_v; */
	/* a->undo_cache = realloc(a->undo_cache, (new_start_pos + a->num_keyframes) * sizeof(Keyframe));	 */
	/* memcpy(a->undo_cache + new_start_pos, a->keyframes, a->num_keyframes * sizeof(Keyframe)); */
	/* Value redo_cache_len = {.uint16_v = a->num_keyframes}; */
	/* Value zero = {.uint16_v = 0}; */
	/* user_event_push( */
	/*      */
	/*     undo_redo_automation_write, */
	/*     undo_redo_automation_write, */
	/*     NULL, NULL, */
	/*     (void *)a, */
	/*     (void *)a->undo_cache, */
	/*     a->undo_cache_len, zero, */
	/*     redo_cache_len, a->undo_cache_len, */
	/*     0, 0, false, false); */
	user_tl_pause(NULL);
    }
    return write;
}

void automation_clear_cache(Automation *a)
{
    a->ghost_valid = false;
    a->m_diff_prop_cum = 0.0;
}

NEW_EVENT_FN(undo_delete_keyframe, "undo delete keyframe")
    Keyframe *inserted = automation_insert_keyframe_at((Automation *)obj1, val1.int32_v, val2);
    self->obj2 = (void *)inserted;
}

NEW_EVENT_FN(redo_delete_keyframe, "redo delete keyframe")
    Keyframe *k = (Keyframe *)obj2;
    keyframe_remove(k);
}

void keyframe_delete(Keyframe *k)
{
    /* Keyframe *cpy = malloc(sizeof(Keyframe)); */
    /* memcpy(cpy, k, sizeof(Keyframe)); */
    Value pos = {.int32_v = k->pos};
    Automation *a = k->automation;
    Value v = k->value;
    keyframe_remove(k);
    Value nullval;
    memset(&nullval, '\0', sizeof(Value));
    user_event_push(
	
	undo_delete_keyframe,
	redo_delete_keyframe,
	NULL, NULL,
	(void *)a, NULL,
	pos, v,
	nullval, nullval,
	0, 0, false, false);
}

void automation_delete_keyframe_range(Automation *a, int32_t start_pos, int32_t end_pos)
{
    uint16_t start_i, end_i;
    /* Don't get keyframe range here */
    if (automation_get_kf_range(a, start_pos, end_pos, &start_i, &end_i)) {
	automation_write_set_undo_cache(a);
	/* uint16_t num_keyframes = end_i - start_i; */
	/* fprintf(stderr, "true, num: %d\n", num_keyframes); */
	/* Keyframe *range_cpy = malloc(num_keyframes * sizeof(Keyframe)); */
	/* memcpy(range_cpy, a->keyframes + start_i, num_keyframes * sizeof(Keyframe)); */

	automation_remove_kf_range(a, start_i, end_i);
	automation_push_write_event(a);
    }
    /* fprintf(stderr, "False\n"); */
	
}


/* TEST_FN_DEF(automation_keyframe_order, { */
/* 	int32_t pos = a->keyframes->pos; */
/* 	for (uint16_t i=0; i<a->num_keyframes; i++) { */
/* 	    if (i != 0 && a->keyframes[i].pos <= pos) { */
/* 		fprintf(stderr, "ERROR:\n"); */
/* 		fprintf(stderr, "Keyframe positions out of order.\n"); */
/* 		for (uint16_t j=0; j<a->num_keyframes; j++) { */
/* 		    fprintf(stderr, "\ti: %d, pos: %d", j, a->keyframes[j].pos); */
/* 		    if (i==j) { */
/* 			fprintf(stderr, " <--\n"); */
/* 		    } else { */
/* 			fprintf(stderr, "\n"); */
/* 		    } */
/* 		} */
/* 		return 1; */
/* 	    } */
/* 	    pos = a->keyframes[i].pos; */
/* 	} */
/* 	return 0; */
/*     }, Automation *a); */

bool automation_handle_delete(Automation *a)
{
    Timeline *tl = a->track->tl;
    if (tl->dragging_keyframe) {
	keyframe_delete(tl->dragging_keyframe);
	tl->dragging_keyframe = NULL;
	status_cat_callstr(" selected keyframe");
	tl->needs_redraw = true;
	return true;
    }
    if (tl->in_mark_sframes < tl->out_mark_sframes) {
	automation_delete_keyframe_range(a, tl->in_mark_sframes, tl->out_mark_sframes);
	status_cat_callstr(" keyframe in->out");
	tl->needs_redraw = true;
	return true;
    }
    return false;
}

int track_select_next_automation(Track *t)
{
    while (1) {
	t->selected_automation++;
	
	/* Advanced past end */
	if (t->selected_automation > t->num_automations - 1) {
	    
	    /* More tracks below */
	    if (t->layout->index < t->layout->parent->num_children - 1) {
		t->selected_automation = -1;
		break;
	    } else {
		/* Stay where we are */
		t->selected_automation--;
		break;
	    }
	} else if (t->automations[t->selected_automation]->shown) {
	    break;
	} else if (t->selected_automation == t->num_automations - 1) {
	    /* At last automation, but it is not shown */
	    t->selected_automation = -1;
	    break;
	}
	/* Keep advancing if selected automation is not shown */
    }
    return t->selected_automation;
}

int track_select_prev_automation(Track *t)
{
    if (t->num_automations == 0) return -1;
    while (t->selected_automation > -1) {
	t->selected_automation--;
	if (t->selected_automation == -1) {
	    break;
	} else if (t->automations[t->selected_automation]->shown) {
	    break;
	} else if (t->selected_automation == 0) {
	    t->selected_automation = -1;
	    break;
	}
    }
    return t->selected_automation;
}

int track_select_last_shown_automation(Track *t)
{
    if (t->num_automations == 0) return -1;
    int i = t->num_automations - 1;
    for (; i>=0; i--) {
	if (t->automations[i]->shown) {
	    break;
	}
    }
    if (i==0 && !t->automations[i]->shown) {
	t->selected_automation = -1;
    } else {
	t->selected_automation = i;
    }
    return t->selected_automation;
}

int track_select_first_shown_automation(Track *t)
{
    if (t->num_automations == 0) return -1;
    int i=0;
    for (; i<t->num_automations-1; i++) {
	if (t->automations[i]->shown) {
	    break;
	}
    }
    if (i==t->num_automations-1 && !t->automations[i]->shown) {
	t->selected_automation = -1;
    } else {
	t->selected_automation = i;
    }
    return t->selected_automation;
}

TEST_FN_DEF(track_automation_order,
	{
	    bool a_index_fault = false;
	    bool layout_index_fault = false;
	    for (uint8_t i=0; i<track->num_automations; i++) {
		Automation *a = track->automations[i];
		if (a->index != i) a_index_fault = true;
		if (a->layout->index != i + 1) layout_index_fault = true;
	    }
	    if (a_index_fault || layout_index_fault) {
		fprintf(stderr, "Fault (%d, %d) in track \"%s\"\n", a_index_fault, layout_index_fault, track->name);
		for (uint8_t i=0; i<track->num_automations; i++) {
		    Automation *a = track->automations[i];
		    fprintf(stderr, "i: %d, index: %d, layout: %lld\n",i, a->index, a->layout->index);
		}
		return 1;
	    }
	    return 0;
	}, Track *track);
    

/* void TEST_track_automation_order(Track *track) */
/* { */

/*     bool a_index_fault = false; */
/*     bool layout_index_fault = false; */
/*     for (uint8_t i=0; i<track->num_automations; i++) { */
/* 	Automation *a = track->automations[i]; */
/* 	if (a->index != i) a_index_fault = true; */
/* 	if (a->layout->index != i) layout_index_fault = true; */
/*     } */

/*     if (a_index_fault || layout_index_fault) { */
/* 	fprintf(stderr, "Fault (%d, %d) in track \"%s\"\n", a_index_fault, layout_index_fault, track->name); */
/* 	for (uint8_t i=0; i<track->num_automations; i++) { */
/* 	    Automation *a = track->automations[i]; */
/* 	    fprintf(stderr, "i: %d, index: %d, layout: %d\n",i, a->index, a->layout->index); */
/* 	} */
/* 	exit(1); */
/*     } */
/* } */

/* void TEST_kclipref_bounds(Automation *a) */
/* { */
/*     for (uint16_t i=0; i<a->num_kclips; i++) { */
/* 	KClipRef *kcr = a->kclips + i; */
/* 	if (kcr->last->pos < kcr->first->pos) { */
/* 	    fprintf(stderr, "ERROR\n"); */
/* 	    fprintf(stderr, "Kclip at index %d is f'd\n", i); */
/* 	    fprintf(stderr, "bounds: %d - %d\n", kcr->first->pos, kcr->last->pos); */
/* 	    exit(1); */
/* 	} */
	
/*     } */
/* } */

TEST_FN_DEF(automation_keyframe_order, {
	int32_t pos = automation->keyframes[0].pos;
	for (uint16_t i=1; i<automation->num_keyframes; i++) {
	    Keyframe *k = automation->keyframes + i;
	    if (k->pos <= pos) {
		return 1;
	    }
	    pos = k->pos;
	}
	return 0;
    }, Automation *automation);


TEST_FN_DEF(automation_index, {
	if (automation != automation->track->automations[automation->index])
	    return 1;
	return 0;
    }, Automation *automation);

TEST_FN_DEF(layout_num_children, {
	int ret = 0;
	for (uint8_t i=0; i<lt->num_children; i++) {
	    Layout *child = lt->children[i];
	    if (!child) {
		fprintf(stderr, "EXCESS children on layout %s: %lld\n", lt->name, lt->num_children);
		return 1;
	    }
	    ret += layout_num_children(child);
	}
	return ret;
    }, Layout *lt);



void automation_endpoint_write(Endpoint *ep, Value val, int32_t play_pos)
{
    /* automation_do_write(ep->automation, val, play_pos, play_pos + 10000, ep->automation->track->tl->session->playback.play_speed); */
}
