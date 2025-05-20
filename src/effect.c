/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

/*****************************************************************************************************************
    effect.c

    * abstract representation of effects
    * each effect contains a pointer to the specific effect object
 *****************************************************************************************************************/


#include "assets.h"
#include "color.h"
/* #include "eq.h" */
#include "delay_line.h"
#include "effect.h"
#include "fir_filter.h"
/* #include "geometry.h" */
#include "modal.h"
#include "page.h"
#include "project.h"
/* #include "waveform.h" */

extern Project *proj;
extern Window *main_win;

extern SDL_Color color_global_light_grey;

static const char *effect_type_strings[] = {
    "Equalizer",
    "FIR Filter",
    "Delay",
    "Saturation",
    "Compressor"
};

const char *effect_type_str(EffectType type)
{
    return effect_type_strings[type];
}


void effect_delete(Effect *e, bool from_undo);
static void effect_reinsert(Effect *e, int index);

NEW_EVENT_FN(undo_add_effect, "undo_add_effect")
    effect_delete(obj1, true);
}

NEW_EVENT_FN(redo_add_effect, "redo_add_effect")
    effect_reinsert(obj1, val1.int_v);
}

NEW_EVENT_FN(dispose_forward_add_effect, "")
    effect_destroy(obj1);
}

Effect *track_add_effect(Track *track, EffectType type)
{
    if (track->num_effects == MAX_TRACK_EFFECTS) {
	fprintf(stderr, "Error: track has maximum number of effects (%d)\n", MAX_TRACK_EFFECTS);
	return NULL;
    }
    Effect *e = calloc(1, sizeof(Effect));
    e->type = type;
    e->track = track;
    e->active = true;
    pthread_mutex_lock(&track->effect_chain_lock);
    track->effects[track->num_effects] = e;
    track->num_effects++;
    pthread_mutex_unlock(&track->effect_chain_lock);
   

    int num_effects_of_type = track->num_effects_per_type[type];
    if (num_effects_of_type == 0) {
	snprintf(e->name, MAX_NAMELENGTH, "%s", effect_type_str(type));
    } else {
	snprintf(e->name, MAX_NAMELENGTH, "%s %d", effect_type_str(type), num_effects_of_type + 1);
    }
    track->num_effects_per_type[type]++;
    /* fprintf(stderr, "Effect name: %s\n", e->name); */
    
    api_node_register(&e->api_node, &track->api_node, e->name);
    
    switch(type) {
    case EFFECT_EQ:
	e->obj = calloc(1, sizeof(EQ));
	((EQ *)e->obj)->effect = e;
	((EQ *)e->obj)->track = track;
	eq_init(e->obj);
	e->buf_apply = eq_buf_apply;
	break;
    case EFFECT_FIR_FILTER: {
	e->obj = calloc(1, sizeof(FIRFilter));
	((FIRFilter *)e->obj)->effect = e;
	int ir_len = track->tl->proj->fourier_len_sframes/4;
	int fr_len = track->tl->proj->fourier_len_sframes * 2;
	filter_init(e->obj, track, LOWPASS, ir_len, fr_len);
	e->buf_apply = filter_buf_apply;
    }
	break;
    case EFFECT_DELAY:
	e->obj = calloc(1, sizeof(DelayLine));
	((DelayLine *)e->obj)->effect = e;
	delay_line_init(e->obj, track, track->tl->proj->sample_rate);
	e->buf_apply = delay_line_buf_apply;
	e->operate_on_empty_buf = true;
	break;
    case EFFECT_SATURATION:
	e->obj = calloc(1, sizeof(Saturation));
	((Saturation *)e->obj)->effect = e;
	saturation_init(e->obj);
	e->buf_apply = saturation_buf_apply;
	break;
    case EFFECT_COMPRESSOR:
	e->obj = calloc(1, sizeof(Compressor));
	((Compressor *)e->obj)->effect = e;
	compressor_init(e->obj);
	e->buf_apply = compressor_buf_apply;
	e->operate_on_empty_buf = true;
	break;
    default:
	break;
    }

    endpoint_init(
	&e->active_ep,
	&e->active,
	JDAW_BOOL,
	"active",
	"Active",
	JDAW_THREAD_DSP,
	NULL, NULL, NULL,
	NULL, NULL, NULL, NULL);
    endpoint_set_default_value(&e->active_ep, (Value){.bool_v = true});
    api_endpoint_register(&e->active_ep, &e->api_node);

    user_event_push(
	&proj->history,
	undo_add_effect, redo_add_effect,
	NULL, dispose_forward_add_effect,
	e, NULL,
	(Value){0}, (Value){0},
	(Value){.int_v = track->num_effects - 1}, (Value){0},
	0,0,false,false);
	
    /* api_node_print_routes_with_values(&track->api_node); */
    
    return e;
}

static int add_effect_form(void *mod_v, void *nullarg);

/* Higher-level function, called from userfn to create modal */
void track_add_new_effect(Track *track)
{
    if (track->num_effects == MAX_TRACK_EFFECTS) {
	fprintf(stderr, "Track num effects: %d. Max: %d\n", track->num_effects, MAX_TRACK_EFFECTS);
	status_set_errstr("Error: reached maximum number of effects per track.");
	return;
    }
    Layout *lt = layout_add_child(track->tl->proj->layout);
    layout_set_default_dims(lt);
    Modal *m = modal_create(lt);
    modal_add_header(m, "Add effect to track", &color_global_light_grey, 4);

    static int effect_selection = 0;
    static Endpoint effect_selection_ep = {0};
    if (effect_selection_ep.local_id == NULL) {
	endpoint_init(
	    &effect_selection_ep,
	    &effect_selection,
	    JDAW_INT,
	    "",
	    "",
	    JDAW_THREAD_MAIN,
	    NULL, NULL, NULL,
	    track, NULL,NULL,NULL);
	effect_selection_ep.block_undo = true;
    }
    effect_selection_ep.xarg1 = (void *)track;
    
    modal_add_radio(
	m,
	&color_global_light_grey,
	/* (void *)track, */
	&effect_selection_ep,
	/* NULL, */
	effect_type_strings,
	/* AUTOMATION_LABELS, */	
	/* sizeof(AUTOMATION_LABELS) / sizeof(const char *)); */
	sizeof(effect_type_strings) / sizeof(const char *));
    
    modal_add_button(m, "Add", add_effect_form);
    m->submit_form = add_effect_form;
    m->stashed_obj = track;
    window_push_modal(main_win, m);
    modal_reset(m);
    modal_move_onto(m);
}

void user_tl_track_open_settings(void *nullarg);
static int add_effect_form(void *mod_v, void *nullarg)
{
    Modal *modal = (Modal *)mod_v;
    Track *track = modal->stashed_obj;
    ModalEl *radio_el = modal->els[1];
    RadioButton *b = radio_el->obj;
    EffectType t = b->selected_item;
    Effect *e = track_add_effect(track, t);
    window_pop_modal(main_win);
    if (e) {
	user_tl_track_open_settings(NULL);
	TabView *tv = main_win->active_tabview;
	if (tv) {
	    tabview_select_tab(tv, tv->num_tabs - 1);
	}
    }
    return 0;
}

static float effect_buf_apply(Effect *e, float *buf, int len, int channel, float input_amp)
{
    /* if (!e->active) return input_amp; */
    return e->buf_apply(e->obj, buf, len, channel, input_amp);
}

float effect_chain_buf_apply(Effect **effects, int num_effects, float *buf, int len, int channel, float input_amp)
{
    if (num_effects == 0) return input_amp;
    static float amp_epsilon = 1e-7f;
    float output = input_amp;
    pthread_mutex_lock(&(effects[0]->track->effect_chain_lock));
    for (int i=0; i<num_effects; i++) {
	Effect *e = effects[i];
	if (e->active && (e->operate_on_empty_buf || fabs(input_amp) > amp_epsilon)) {
	    output = effect_buf_apply(e, buf, len, channel, input_amp);
	}
    }
    pthread_mutex_unlock(&(effects[0]->track->effect_chain_lock));
    return output;
}


static void effect_reinsert(Effect *e, int index)
{
    Track *track = e->track;

    int num_to_move = track->num_effects - index;
    
    pthread_mutex_lock(&track->effect_chain_lock);
    memmove(track->effects + index + 1, track->effects + index, num_to_move * sizeof(Effect *));
    track->effects[index] = e;
    track->num_effects++;
    pthread_mutex_unlock(&track->effect_chain_lock);
    
    TabView *tv;
    if ((tv = main_win->active_tabview) && strcmp(tv->title, "Track Effects") == 0) {
	/* tabview_close(tv); */
	user_tl_track_open_settings(NULL);
	user_tl_track_open_settings(NULL);
    }
    api_node_reregister(&e->api_node);
    /* undelete_related_automations(&e->api_node); */
}

NEW_EVENT_FN(undo_effect_delete, "undo delete effect")
    effect_reinsert(obj1, val1.int_v);
    /* Automation **related_automations = obj2; */

    /* fprintf(stderr, "LIST IN UNDO:\n\t"); */
    /* for (int i=0; i<val2.int_v; i++) { */
    /* 	fprintf(stderr, "%p, ", related_automations[i]); */
    /* } */
    /* fprintf(stderr, "\n"); */

    /* for (int i=0; i<val2.int_v; i++) { */
    /* 	Automation *a = related_automations[i]; */
    /* 	if (!a->deleted) */
    /* 	    automation_reinsert(a); */
    /* } */
}

NEW_EVENT_FN(redo_effect_delete, "redo delete effect")
    effect_delete(obj1, true);
    /* for (int i=0; i<val2.int_v; i++) { */
    /* 	Automation *a = related_automations[i]; */
    /* } */

}

NEW_EVENT_FN(dispose_effect_delete, "")
    effect_destroy(obj1);
}


/* static void delete_related_automations(APINode *node) */
/* { */
/*     for (int i=0; i<node->num_endpoints; i++) { */
/* 	Endpoint *ep = node->endpoints[i]; */
/* 	if (ep->automation) { */
/* 	    automation_remove(ep->automation); */
/* 	} */
/*     } */
/*     for (int i=0; i<node->num_children; i++) { */
/* 	delete_related_automations(node->children[i]); */
/*     } */
/* } */

/* static void undelete_related_automations(APINode *node) */
/* { */
/*     for (int i=0; i<node->num_endpoints; i++) { */
/* 	Endpoint *ep = node->endpoints[i]; */
/* 	if (ep->automation) { */
/* 	    automation_reinsert(ep->automation); */
/* 	} */
/*     } */
/*     for (int i=0; i<node->num_children; i++) { */
/* 	undelete_related_automations(node->children[i]); */
/*     } */
/* } */

/* int get_related_automations(APINode *node, Automation ***list_loc, int *arr_size, int num_items) */
/* { */
/*     if (*arr_size < 4) *arr_size = 4; */
/*     if (!*list_loc) { */
/* 	(*list_loc) = calloc(*arr_size, sizeof(Automation *)); */
/*     } */
/*     for (int i=0; i<node->num_endpoints; i++) { */
/* 	Endpoint *ep = node->endpoints[i]; */
/* 	if (ep->automation) { */
/* 	    (*list_loc)[num_items] = ep->automation; */
/* 	    num_items++; */
/* 	    /\* (*num_items)++; *\/ */
/* 	    if (num_items == *arr_size) { */
/* 		*arr_size *= 2; */
/* 		*list_loc = realloc(*list_loc, *arr_size * sizeof(Automation *)); */
/* 	    } */
/* 	    /\* automation_reinsert(ep->automation); *\/ */
/* 	} */
/*     } */
/*     /\* fprintf(stderr, "LIST BEFORE RECURSIVE CALL, arrlen: %d, (%lu bytes, %lu pointers):\n\t", *arr_size, sizeof(*list_loc), sizeof(*list_loc) / sizeof(Automation *)); *\/ */
/*     fprintf(stderr, "Array members before arr_size access:\n\t"); */
/*     for (int i=0; i<num_items; i++) { */
/* 	fprintf(stderr, "%p, ", (*list_loc)[i]); */
/*     } */
/*     fprintf(stderr, "\n"); */
/*     fprintf(stderr, "Array members after arr_size access: (arr_size = %d)\n\t", *arr_size); */
/*     for (int i=0; i<num_items; i++) { */
/* 	fprintf(stderr, "%p, ", (*list_loc)[i]); */
/*     } */
/*     fprintf(stderr, "\n"); */

/*     breakfn(); */

/*     for (int i=0; i<node->num_children; i++) { */
/* 	num_items = get_related_automations(node->children[i], list_loc, arr_size, num_items); */
/*     } */
/*     fprintf(stderr, "LIST BEFORE RETURN, arrlen: %d, (%lu bytes, %lu pointers):\n\t", *arr_size, sizeof(*list_loc), sizeof(*list_loc) / sizeof(Automation *)); */
/*     for (int i=0; i<num_items; i++) { */
/* 	fprintf(stderr, "%p, ", (*list_loc)[i]); */
/*     } */
/*     fprintf(stderr, "\n"); */

/*     return num_items; */

/* } */



void effect_delete(Effect *e, bool from_undo)
{
    Track *track = e->track;
    bool displace = false;
    int index;
    for (int i=0; i<track->num_effects; i++) {
	if (e == track->effects[i]) {
	    displace = true;
	    index = i;
	} else if (displace) {
	    track->effects[i - 1] = track->effects[i];
	}
    }
    
    track->num_effects--;
    /* Automation **related_automations = NULL; */
    /* int arr_size = 4; */
    /* int num_related_automations = get_related_automations(&e->api_node, &related_automations, &arr_size, 0); */

    /* fprintf(stderr, "LIST IN DELETE:\n\t"); */
    /* for (int i=0; i<num_related_automations; i++) { */
    /* 	fprintf(stderr, "%p, ", related_automations[i]); */
    /* } */
    /* fprintf(stderr, "\n"); */

    /* /\* breakfn(); *\/ */
    /* /\* fprintf(stderr, "%d related automations\n", num_related_automations); *\/ */
    /* for (int i=0; i<num_related_automations; i++) { */
    /* 	Automation *a = related_automations[i]; */
    /* 	automation_remove(a); */
    /* } */
    /* fprintf(stderr, "LIST AFTER REMOVING RETURN:\n\t"); */
    /* for (int i=0; i<num_related_automations; i++) { */
    /* 	fprintf(stderr, "%p, ", related_automations[i]); */
    /* } */
    /* fprintf(stderr, "\n"); */

    void *related_automations = NULL;
    int num_related_automations = 0;
    if (!from_undo) {
	user_event_push(
	    &proj->history,
	    undo_effect_delete,
	    redo_effect_delete,
	    dispose_effect_delete,
	    NULL,
	    e, related_automations,
	    (Value){.int_v = index}, (Value){.int_v = num_related_automations},
	    (Value){.int_v = index}, (Value){.int_v = num_related_automations},
	    0, 0, false, false);
    }
    api_node_deregister(&e->api_node);

    /* delete_related_automations(&e->api_node); */
    TabView *tv;
    if ((tv = main_win->active_tabview) && strcmp(tv->title, "Track Effects") == 0) {
	/* tabview_close(tv); */
	user_tl_track_open_settings(NULL);
	if (track->num_effects > 0) 
	    user_tl_track_open_settings(NULL);
    }

}

void effect_destroy(Effect *e)
{
    switch(e->type) {
    case EFFECT_EQ:
	eq_deinit((EQ *)e->obj);
	break;
    case EFFECT_FIR_FILTER:
	filter_deinit((FIRFilter *)e->obj);
	break;
    case EFFECT_DELAY:
	delay_line_deinit((DelayLine *)e->obj);
	break;
    case EFFECT_SATURATION:
	/* saturation_deinit(e->obj); */
	break;
    case EFFECT_COMPRESSOR:
	/* compressor_deinit(((Compressor *)e->obj); */
	break;
    default:
	break;
    }
	free(e->obj);
	free(e);
}
