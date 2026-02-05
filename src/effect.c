/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

#include "api.h"
#include "color.h"
#include "delay_line.h"
#include "effect.h"
#include "fir_filter.h"
#include "log.h"
#include "modal.h"
#include "page.h"
#include "project.h"
#include "session.h"

extern Window *main_win;

extern struct colors colors;

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

/* Effect *track_add_effect(Track *track, EffectType type) */
/* { */
/*     if (track->num_effects == MAX_TRACK_EFFECTS) { */
/* 	fprintf(stderr, "Error: track has maximum number of effects (%d)\n", MAX_TRACK_EFFECTS); */
/* 	return NULL; */
/*     } */
/*     Effect *e = calloc(1, sizeof(Effect)); */
/*     e->type = type; */
/*     /\* e->track = track; *\/ */
/*     /\* e->active = true;    *\/ */

/*     int num_effects_of_type = track->num_effects_per_type[type]; */
/*     if (num_effects_of_type == 0) { */
/* 	snprintf(e->name, MAX_NAMELENGTH, "%s", effect_type_str(type)); */
/*     } else { */
/* 	snprintf(e->name, MAX_NAMELENGTH, "%s %d", effect_type_str(type), num_effects_of_type + 1); */
/*     } */
/*     track->num_effects_per_type[type]++; */
/*     /\* fprintf(stderr, "Effect name: %s\n", e->name); *\/ */
    
/*     api_node_register(&e->api_node, &track->api_node, e->name, NULL); */
    
/*     switch(type) { */
/*     case EFFECT_EQ: */
/* 	e->obj = calloc(1, sizeof(EQ)); */
/* 	((EQ *)e->obj)->effect = e; */
/* 	((EQ *)e->obj)->track = track; */
/* 	eq_init(e->obj); */
/* 	e->buf_apply = eq_buf_apply; */
/* 	break; */
/*     case EFFECT_FIR_FILTER: { */
/* 	e->obj = calloc(1, sizeof(FIRFilter)); */
/* 	((FIRFilter *)e->obj)->effect = e; */
/* 	int ir_len = track->tl->proj->fourier_len_sframes/4; */
/* 	int fr_len = track->tl->proj->fourier_len_sframes * 2; */
/* 	filter_init(e->obj, track, LOWPASS, ir_len, fr_len); */
/* 	e->buf_apply = filter_buf_apply; */
/*     } */
/* 	break; */
/*     case EFFECT_DELAY: */
/* 	e->obj = calloc(1, sizeof(DelayLine)); */
/* 	((DelayLine *)e->obj)->effect = e; */
/* 	delay_line_init(e->obj, track, track->tl->proj->sample_rate); */
/* 	e->buf_apply = delay_line_buf_apply; */
/* 	e->operate_on_empty_buf = true; */
/* 	break; */
/*     case EFFECT_SATURATION: */
/* 	e->obj = calloc(1, sizeof(Saturation)); */
/* 	((Saturation *)e->obj)->effect = e; */
/* 	saturation_init(e->obj); */
/* 	e->buf_apply = saturation_buf_apply; */
/* 	break; */
/*     case EFFECT_COMPRESSOR: */
/* 	e->obj = calloc(1, sizeof(Compressor)); */
/* 	((Compressor *)e->obj)->effect = e; */
/* 	compressor_init(e->obj); */
/* 	e->buf_apply = compressor_buf_apply; */
/* 	e->operate_on_empty_buf = true; */
/* 	break; */
/*     default: */
/* 	break; */
/*     } */

/*     endpoint_init( */
/* 	&e->active_ep, */
/* 	&e->active, */
/* 	JDAW_BOOL, */
/* 	"active", */
/* 	"Active", */
/* 	JDAW_THREAD_DSP, */
/* 	NULL, NULL, NULL, */
/* 	NULL, NULL, NULL, NULL); */
/*     endpoint_set_default_value(&e->active_ep, (Value){.bool_v = true}); */
/*     api_endpoint_register(&e->active_ep, &e->api_node); */

/*     user_event_push( */
/* 	undo_add_effect, redo_add_effect, */
/* 	NULL, dispose_forward_add_effect, */
/* 	e, NULL, */
/* 	(Value){0}, (Value){0}, */
/* 	(Value){.int_v = track->num_effects}, (Value){0}, */
/* 	0,0,false,false); */

/*     pthread_mutex_lock(&track->effect_chain_lock); */
/*     track->effects[track->num_effects] = e; */
/*     track->num_effects++; */
/*     e->active = true; */
/*     pthread_mutex_unlock(&track->effect_chain_lock); */

/*     /\* endpoint_write(&e->active_ep, (Value){.bool_v = true}, true, true, true, false); *\/ */

	
/*     /\* api_node_print_routes_with_values(&track->api_node); *\/ */
    
/*     return e; */
/* } */

/* Always call before adding any effects */
void effect_chain_init(EffectChain *ec, Project *proj, APINode *parent_api_node, const char *obj_name, int32_t chunk_len_sframes)
{
    bool already_init = false;
    if (ec->effects) {
	already_init = true;
	return;
    }
    ec->proj = proj;
    ec->obj_name = obj_name;
    ec->effects_alloc_len = 0;
    ec->num_effects = 0;
    if (!already_init) {
	api_node_register(&ec->api_node, parent_api_node, NULL, "Effects");
    } else {
	if (ec->api_node.parent != parent_api_node) {
	    log_tmp(LOG_ERROR, "In redundant call to effect_chain_init, api_node is not equal to (new) parent\n");
	}
    }
    /* ec->api_node = api_node; */
    ec->chunk_len_sframes = chunk_len_sframes;
    memset(ec->num_effects_per_type, 0, sizeof(int) * NUM_EFFECT_TYPES);
    pthread_mutex_init(&ec->effect_chain_lock, NULL);
}

void effect_chain_block_type(EffectChain *ec, EffectType type)
{
    ec->blocked_types[type] = true;
}

/* Destroys all included effects */
void effect_chain_deinit(EffectChain *ec)
{
    pthread_mutex_lock(&ec->effect_chain_lock);
    ec->num_effects = 0;
    pthread_mutex_unlock(&ec->effect_chain_lock);
    for (int i=0; i<ec->num_effects; i++) {
	effect_destroy(ec->effects[i]);
    }
    if (ec->effects) {
	free(ec->effects);
    }
    ec->effects = NULL;
    pthread_mutex_destroy(&ec->effect_chain_lock);
}

/*------ Higher-level functions; modals and tabview ------------------*/

/* assume 1 effect is being added */
static void effect_chain_realloc_maybe(EffectChain *ec)
{
    if (ec->effects_alloc_len == 0) {
	if (ec->effects) {
	    log_tmp(LOG_ERROR, "Effect chain (%p) has effects_alloc_len==0, but effects=%p\n", ec, ec->effects);
	}
	ec->effects_alloc_len = 4;
	ec->effects = malloc(ec->effects_alloc_len * sizeof(Effect *));
    }
    if (ec->num_effects == ec->effects_alloc_len) {
	ec->effects_alloc_len *= 2;
	ec->effects = realloc(ec->effects, ec->effects_alloc_len * sizeof(Effect *));
    }

}

Effect *effect_chain_add_effect(EffectChain *ec, EffectType type)
{
    if (ec->blocked_types[type]) {
	status_set_errstr("%s not allowed here", effect_type_strings[type]);
	return NULL;
    }
    /* if (ec->effects_alloc_len == 0) { */
    /* 	if (ec->effects) { */
    /* 	    log_tmp(LOG_ERROR, "Effect chain (%p) has effects_alloc_len==0, but effects=%p\n", ec, ec->effects); */
    /* 	} */
    /* 	ec->effects_alloc_len = 4; */
    /* 	ec->effects = malloc(ec->effects_alloc_len * sizeof(Effect *)); */
    /* } */
    /* if (ec->num_effects == ec->effects_alloc_len) { */
    /* 	ec->effects_alloc_len *= 2; */
    /* 	ec->effects = realloc(ec->effects, ec->effects_alloc_len * sizeof(Effect *)); */
    /* } */
    effect_chain_realloc_maybe(ec);
    Effect *e = calloc(1, sizeof(Effect));
    e->type = type;
    e->effect_chain = ec;
    int num_effects_of_type = ec->num_effects_per_type[type];
    if (num_effects_of_type == 0) {
	snprintf(e->name, MAX_NAMELENGTH, "%s", effect_type_str(type));
    } else {
	snprintf(e->name, MAX_NAMELENGTH, "%s %d", effect_type_str(type), num_effects_of_type + 1);
    }
    ec->num_effects_per_type[type]++;
    api_node_register(&e->api_node, &ec->api_node, e->name, NULL);

    switch(type) {
    case EFFECT_EQ:
	e->obj = calloc(1, sizeof(EQ));
	((EQ *)e->obj)->effect = e;
	/* ((EQ *)e->obj)->track = track; */
	eq_init(e->obj);
	e->buf_apply = eq_buf_apply;
	break;
    case EFFECT_FIR_FILTER: {
	e->obj = calloc(1, sizeof(FIRFilter));
	((FIRFilter *)e->obj)->effect = e;
	/* int ir_len = track->tl->proj->fourier_len_sframes/4; */
	/* int fr_len = track->tl->proj->fourier_len_sframes * 2; */
	int ir_len = ec->chunk_len_sframes;
	filter_init(e->obj, LOWPASS, ir_len, ec->proj->fourier_len_sframes * 2, ec->chunk_len_sframes);
	e->buf_apply = filter_buf_apply;
    }
	break;
    case EFFECT_DELAY:
	e->obj = calloc(1, sizeof(DelayLine));
	((DelayLine *)e->obj)->effect = e;
	delay_line_init(e->obj, ec->proj->sample_rate);
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
	undo_add_effect, redo_add_effect,
	NULL, dispose_forward_add_effect,
	e, NULL,
	(Value){0}, (Value){0},
	(Value){.int_v = ec->num_effects}, (Value){0},
	0,0,false,false);

    pthread_mutex_lock(&ec->effect_chain_lock);
    ec->effects[ec->num_effects] = e;
    ec->num_effects++;
    e->active = true;
    pthread_mutex_unlock(&ec->effect_chain_lock);
    return e;
}

static int add_effect_form(void *mod_v, void *nullarg);

/* Higher-level function, called from userfn to create modal */
void effect_add(EffectChain *ec, const char *obj_name)
{
    Session *session = session_get();
    Layout *lt = layout_add_child(session->gui.layout);
    layout_set_default_dims(lt);
    Modal *m = modal_create(lt);
    char buf[256];
    if (obj_name) {
	snprintf(buf, 256, "Add effect to %s", obj_name);
    } else {
	snprintf(buf, 256, "Add effect");
    }
    modal_add_header(m, buf, &colors.light_grey, 4);

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
	    NULL, NULL,NULL,NULL);
	effect_selection_ep.block_undo = true;
    }
    /* effect_selection_ep.xarg1 = (void *)track; */
    
    ModalEl *el = modal_add_radio(
	m,
	&colors.light_grey,
	/* (void *)track, */
	&effect_selection_ep,
	/* NULL, */
	effect_type_strings,
	/* AUTOMATION_LABELS, */	
	/* sizeof(AUTOMATION_LABELS) / sizeof(const char *)); */
	sizeof(effect_type_strings) / sizeof(const char *));

    for (int i=0; i<NUM_EFFECT_TYPES; i++) {
	if (ec->blocked_types[i]) {
	    radio_grey_item(el->obj, i);
	}
    }
    modal_add_button(m, "Add", add_effect_form);
    m->submit_form = add_effect_form;
    m->stashed_obj = ec;
    /* m->stashed_obj = track; */
    window_push_modal(main_win, m);
    modal_reset(m);
    modal_move_onto(m);
}

void effect_chain_open_tabview(EffectChain *ec);

/* void user_tl_track_open_settings(void *nullarg); */
static int add_effect_form(void *mod_v, void *nullarg)
{
    Modal *modal = (Modal *)mod_v;
    /* Track *track = modal->stashed_obj; */
    ModalEl *radio_el = modal->els[1];
    RadioButton *b = radio_el->obj;
    EffectType t = b->selected_item;
    EffectChain *ec = modal->stashed_obj;
    Effect *e = effect_chain_add_effect(ec, t);
    Session *session = session_get();
    /* If effect is being added to a currently-monitored synth, make sure it has the right owner! */
    if (session->midi_io.monitoring && session->midi_io.monitor_synth && &session->midi_io.monitor_synth->effect_chain == ec) {
	Synth *synth = session->midi_io.monitor_synth;
	pthread_mutex_lock(&synth->audio_proc_lock);
	synth_close_all_notes(synth);
	api_node_set_owner(&e->api_node, JDAW_THREAD_PLAYBACK);
	pthread_mutex_unlock(&synth->audio_proc_lock);
    }

    /* Effect *e = track_add_effect(track, t); */
    window_pop_modal(main_win);
    if (e) {
	effect_chain_open_tabview(ec);
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

float effect_chain_buf_apply(EffectChain *ec, float *buf, int len, int channel, float input_amp)
{
    static float amp_epsilon = 1e-7f;
    float output = input_amp;
    if (len > ec->chunk_len_sframes) {
	int index = 0;
	while (index < len) {
	    output = effect_chain_buf_apply(ec, buf + index, ec->chunk_len_sframes, channel, output);
	    index += ec->chunk_len_sframes;
	}
	return output;
	
    }
    pthread_mutex_lock(&ec->effect_chain_lock);
    for (int i=0; i<ec->num_effects; i++) {
	Effect *e = ec->effects[i];
	if (e->active && (e->operate_on_empty_buf || fabs(input_amp) > amp_epsilon)) {
	    output = effect_buf_apply(e, buf, len, channel, input_amp);
	}
    }
    pthread_mutex_unlock(&ec->effect_chain_lock);
    return output;
}

static void effect_silence(Effect *e)
{
    switch(e->type) {
    case EFFECT_EQ:
	eq_clear(e->obj);
	break;
    case EFFECT_DELAY:
	delay_line_clear(e->obj);
	break;
    default:
	break;
    }
}

void effect_chain_silence(EffectChain *ec)
{
    for (int i=0; i<ec->num_effects; i++) {
	effect_silence(ec->effects[i]);
    }
}


static void effect_reinsert(Effect *e, int index)
{
    EffectChain *ec = e->effect_chain;
    effect_chain_realloc_maybe(ec);
    
    int num_to_move = ec->num_effects - index;    
    pthread_mutex_lock(&ec->effect_chain_lock);
    memmove(ec->effects + index + 1, ec->effects + index, num_to_move * sizeof(Effect *));
    ec->effects[index] = e;
    ec->num_effects++;
    pthread_mutex_unlock(&ec->effect_chain_lock);

    TabView *tv;
    if ((tv = main_win->active_tabview) && strncmp(tv->title, "Effects", 7) == 0) {
	
	tabview_close(tv);
	effect_chain_open_tabview(ec);
	/* user_tl_track_open_settings(NULL); */
	/* user_tl_track_open_settings(NULL); */
    }
    api_node_reregister(&e->api_node);
    /* undelete_related_automations(&e->api_node); */
}

NEW_EVENT_FN(undo_effect_delete, "undo delete effect")
    effect_reinsert(obj1, val1.int_v);
}

NEW_EVENT_FN(redo_effect_delete, "redo delete effect")
    effect_delete(obj1, true);
}

NEW_EVENT_FN(dispose_effect_delete, "")
    effect_destroy(obj1);
}

void effect_delete(Effect *e, bool from_undo)
{
    EffectChain *ec = e->effect_chain;
    bool displace = false;
    int index = -1;
    for (int i=0; i<ec->num_effects; i++) {
	if (e == ec->effects[i]) {
	    displace = true;
	    index = i;
	} else if (displace) {
	    ec->effects[i - 1] = ec->effects[i];
	}
    }
    if (index < 0) {
	log_tmp(LOG_ERROR, "In effect delete: effect not found in chain\n");
	return;
    }
    
    ec->num_effects--;

    void *related_automations = NULL;
    int num_related_automations = 0;
    if (!from_undo) {
	user_event_push(
	    
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
    if ((tv = main_win->active_tabview) && strncmp(tv->title, "Effects", 7) == 0) {
	tabview_close(tv);
	/* ec->proj->timelines[ec->proj->active_tl_index]->needs_redraw = true; */
	if (ec->num_effects > 0) {
	    effect_chain_open_tabview(ec);
	} else {
	    ec->proj->timelines[ec->proj->active_tl_index]->needs_redraw = true;
	}
	/* if (strncmp(ec->obj_name, "Synth", 5) == 0) { */
	/* } */
	
	/* user_tl_track_open_settings(NULL); */
	/* if (ec->num_effects > 0)  */
	/*     user_tl_track_open_settings(NULL); */
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

/* Legacy function: included for backward compatibility (in .jdaw) */
Effect *track_add_effect(Track *t, EffectType type)
{
    return effect_chain_add_effect(&t->effect_chain, type);
}
