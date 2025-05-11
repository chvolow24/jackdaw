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


Effect *track_add_effect(Track *track, EffectType type)
{

    if (track->num_effects == MAX_TRACK_EFFECTS) {
	fprintf(stderr, "Error: track has maximum number of effects (%d)\n", MAX_TRACK_EFFECTS);
	return NULL;
    }
    Effect *e = calloc(1, sizeof(Effect));
    e->type = type;
    e->track = track;
    track->effects[track->num_effects] = e;
    track->num_effects++;
   

    int num_effects_of_type = track->num_effects_per_type[type];
    if (num_effects_of_type == 0) {
	snprintf(e->name, MAX_NAMELENGTH, "%s", effect_type_str(type));
    } else {
	snprintf(e->name, MAX_NAMELENGTH, "%s %d", effect_type_str(type), num_effects_of_type + 1);
    }
    track->num_effects_per_type[type]++;
    fprintf(stderr, "Effect name: %s\n", e->name);
    
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
	break;
    default:
	break;
    }

    api_node_print_routes_with_values(&track->api_node);
    
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

    
    /* APINode node = track->api_node; */
    /* Endpoint *items[node.num_endpoints]; */
    /* const char *item_labels[node.num_endpoints]; */
    /* for (int i=0; i<node.num_endpoints; i++) { */
    /* 	items[i] = node.endpoints[i]; */
    /* 	item_labels[i] = node.endpoints[i]->display_name; */
    /* } */
    
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
    }
    return 0;
}

static float effect_buf_apply(Effect *e, float *buf, int len, int channel, float input_amp)
{
    return e->buf_apply(e->obj, buf, len, channel, input_amp);
}

float effect_chain_buf_apply(Effect **effects, int num_effects, float *buf, int len, int channel, float input_amp)
{
    static float amp_epsilon = 1e-7f;
    float output = input_amp;
    for (int i=0; i<num_effects; i++) {
	Effect *e = effects[i];
	if (e->operate_on_empty_buf || fabs(input_amp) > amp_epsilon) {
	    output = effect_buf_apply(e, buf, len, channel, input_amp);
	}
    }
    return output;
}
