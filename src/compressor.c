/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

#include <stdio.h>
#include "compressor.h"
#include "consts.h"
#include "endpoint.h"
#include "endpoint_callbacks.h"
#include "geometry.h"
#include "project.h"
#include "session.h"
#include "window.h"

#define COMP_GRAPH_R 4

#define COMP_DEFAULT_ATTACK 5.0f
#define COMP_DEFAULT_RELEASE 50.0f
#define COMP_DEFAULT_RATIO 0.5f
#define COMP_DEFAULT_THRESHOLD 0.2f
#define COMP_MAX_MAKEUP_GAIN 10.0f

extern Window *main_win;

/* Per-sample operation */
/* static inline float compressor_sample_gain_reduction(Compressor *c, float in) */
/* { */
/*     float env = envelope_follower_sample(&c->ef, in); */
/*     float overshoot = env - c->threshold; */
/*     if (overshoot > 0.0f) { */
/* 	overshoot *= c->m; */
/* 	float gain_reduc = (c->threshold + overshoot) / env; */
/* 	return gain_reduc; */
/* 	/\* c->gain_reduc *\/	 */
/*     } */
/*     else return 1.0; */
/* } */

/* Type: `void (*)(Endpoint *) (aka void (*)(struct endpoint *))`   */


float compressor_buf_apply(void *compressor_v, float *buf, int len, int channel, float input_amp)
{
    Compressor *c = compressor_v;
    /* if (!c->active) return input_amp; */
    float amp_scalar;
    float env;
    float output_amp = 0.0f;
    for (int i=0; i<len; i++) {
	env = envelope_follower_sample(&c->ef[channel], buf[i]);
	float overshoot = env - c->threshold;
	if (overshoot > 0.0f) {
	    overshoot *= c->m;
	    amp_scalar = (c->threshold + overshoot) / env;
	    buf[i] *= amp_scalar;
	    buf[i] *= c->makeup_gain;
	} else {
	    buf[i] *= c->makeup_gain;
	}
	output_amp += fabs(buf[i]);
	
    }
    c->gain_scalar[channel] = amp_scalar;
    c->env[channel] = env;
    return output_amp;
    
}

void compressor_set_times_msec(Compressor *c, double attack_msec, double release_msec, double sample_rate)
{
    envelope_follower_set_times_msec(&c->ef[0], attack_msec, release_msec, sample_rate);
    envelope_follower_set_times_msec(&c->ef[1], attack_msec, release_msec, sample_rate);
}

void compressor_set_threshold(Compressor *c, float thresh)
{
    c->threshold = thresh;
}

void compressor_set_m(Compressor *c, float m)
{
    c->m = m;
}

void compressor_draw(Compressor *c, SDL_Rect *target)
{
    SDL_SetRenderDrawColor(main_win->rend, 0, 15, 20, 255);
    SDL_RenderFillRect(main_win->rend, target);
    SDL_SetRenderDrawColor(main_win->rend, 255, 255, 255, 255);
    SDL_RenderDrawRect(main_win->rend, target);

    /* int thresh_x = c->threshold * target->w + target->x; */
    /* int vertex_rel = thresh_x * target->w; */
    int vertex_rel = c->threshold * target->w;
    int x_vertex = target->x + vertex_rel;
    int y_vertex = target->y + target->h - vertex_rel;
    SDL_RenderDrawLine(main_win->rend, target->x, target->y + target->h, x_vertex, y_vertex);


    int end_y = y_vertex - (c->m * (target->w - vertex_rel));
    SDL_RenderDrawLine(main_win->rend, x_vertex, y_vertex, target->x + target->w, end_y);


    

    bool overdriven = false;
    float env = c->env[0];
    if (env > 1.0) {
	env = 1.0;
	overdriven = true;
    }
    int env_x_rel = env * target->w;
    int env_y;
    if (env > c->threshold) {
	env_y = y_vertex - (c->m * (env_x_rel - vertex_rel));
    } else {
	env_y = target->y + target->h - env_x_rel;
    }

    bool under_env = true;
    for (int x_rel=vertex_rel; x_rel<target->w; x_rel++) {

	int top_y = target->y + (target->h - x_rel);
	int btm_y = target->y + target->h - vertex_rel - (x_rel - vertex_rel) * c->m;
	
	if (under_env && x_rel > env_x_rel) {
	    under_env = false;
	    SDL_SetRenderDrawColor(main_win->rend, 245, 245, 255, 50);
	} else if (under_env) {
	    int green = 255 - 255 * ((float)btm_y - top_y) / target->w;
	    /* int green, red; */
	    /* float prop = ((float)btm_y - top_y) / target->h; */
	    /* if (prop > 0.5) { */
	    /* 	red = 255; */
	    /* 	green = 255 - 255 * (prop - 0.5f) * 2.0f; */
	    /* } else { */
	    /* 	green = 255; */
	    /* 	red = 255 * (prop * 2.0f); */
	    /* } */
	    /* int green = 255 - 255.0f *   */
	    SDL_SetRenderDrawColor(main_win->rend, 255, green, 0, 180);
	}
	/* int btm_y = target->y + (x_rel - vertex_rel) * c->m; */
	SDL_RenderDrawLine(main_win->rend, x_rel + target->x, top_y, x_rel + target->x, btm_y - 1);
    }

    
    if (overdriven) {
	SDL_SetRenderDrawColor(main_win->rend, 255, 100, 100, 255);
    } else {
	SDL_SetRenderDrawColor(main_win->rend, 150, 200, 255, 255);
    }
    geom_fill_circle(main_win->rend, env_x_rel + target->x - COMP_GRAPH_R * main_win->dpi_scale_factor, env_y - COMP_GRAPH_R * main_win->dpi_scale_factor, COMP_GRAPH_R * main_win->dpi_scale_factor);
    /* int prev_x = target->x; */
    /* int prev_y = target->y + target->h; */    
    /* for (int x=target->x + 1; x<target->x + target->w; x++) { */
    /* 	float xrel = ((float)x - target->x) / target->w; */
    /* 	if (xrel < c->threshold) { */
    /* 	    SDL_RenderDrawLine( */
    /* 	} */
    /* } */


}


extern Project *proj;
void comp_times_dsp_cb(Endpoint *ep)
{
    Session *session = session_get();
    Compressor *c = ep->xarg1;
    compressor_set_times_msec(c, c->attack_time, c->release_time, session->proj.sample_rate);
}

void comp_ratio_dsp_cb(Endpoint *ep)
{
    Compressor *c = ep->xarg1;
    c->m = 1.0f - c->ratio;
}

static void ratio_labelfn(char *dst, size_t dstsize, Value v, ValType type)
{
    float r = v.float_v;
    r = 1.0f-r;
    float num = 1.0f/r;
    snprintf(dst, dstsize, "%.2f:1",num);
}



void compressor_init(Compressor *c)
{
    Session *session = session_get();
    c->attack_time = COMP_DEFAULT_ATTACK;
    c->release_time = COMP_DEFAULT_RELEASE;
    if (session->proj_initialized) {
	compressor_set_times_msec(c, c->attack_time, c->release_time, session->proj.sample_rate);
    } else {
	compressor_set_times_msec(c, c->attack_time, c->release_time, DEFAULT_SAMPLE_RATE);
    }
    c->makeup_gain = 1.0;
    c->ratio = COMP_DEFAULT_RATIO;
    c->m = 1.0 - c->ratio;
    c->threshold = COMP_DEFAULT_THRESHOLD;

    /* endpoint_init( */
    /* 	&c->active_ep, */
    /* 	&c->active, */
    /* 	JDAW_BOOL, */
    /* 	"compressor_active", */
    /* 	"Compressor active", */
    /* 	JDAW_THREAD_DSP, */
    /* 	track_settings_page_el_gui_cb, NULL, NULL, */
    /* 	c, NULL, &c->effect->page, "track_settings_comp_active_toggle"); */
    /* endpoint_set_default_value(&c->attack_time_ep, (Value){.float_v=}); */
    /* endpoint_set_label_fn(&c->attack_time_ep, label_msec); */
    /* api_endpoint_register(&c->active_ep, &c->effect->api_node); */
	
    
    endpoint_init(
	&c->attack_time_ep,
	&c->attack_time,
	JDAW_FLOAT,
	"attack_time",
	"Attack time",
	JDAW_THREAD_DSP,
	track_settings_page_el_gui_cb, NULL, comp_times_dsp_cb,
	c, NULL, &c->effect->page, "track_settings_comp_attack_slider");

    endpoint_set_allowed_range(&c->attack_time_ep, (Value){.float_v=0.0f}, (Value){.float_v=200.0f});
    endpoint_set_default_value(&c->attack_time_ep, (Value){.float_v=COMP_DEFAULT_ATTACK});
    endpoint_set_label_fn(&c->attack_time_ep, label_msec);
    api_endpoint_register(&c->attack_time_ep, &c->effect->api_node);

    endpoint_init(
	&c->release_time_ep,
	&c->release_time,
	JDAW_FLOAT,
	"release_time",
	"Release time",
	JDAW_THREAD_DSP,
	track_settings_page_el_gui_cb, NULL, comp_times_dsp_cb,
	c, NULL, &c->effect->page, "track_settings_comp_release_slider");
    endpoint_set_allowed_range(&c->release_time_ep, (Value){.float_v=0.0f}, (Value){.float_v=2000.0f});
    endpoint_set_default_value(&c->release_time_ep, (Value){.float_v=COMP_DEFAULT_RELEASE});
    endpoint_set_label_fn(&c->release_time_ep, label_msec);
    api_endpoint_register(&c->release_time_ep, &c->effect->api_node);


    endpoint_init(
	&c->threshold_ep,
	&c->threshold,
	JDAW_FLOAT,
	"threshold",
	"Threshold",
	JDAW_THREAD_DSP,
	track_settings_page_el_gui_cb, NULL, NULL,
	NULL, NULL, &c->effect->page, "track_settings_comp_threshold_slider");
    endpoint_set_allowed_range(&c->threshold_ep, (Value){.float_v=0.0f}, (Value){.float_v=1.0f});
    endpoint_set_default_value(&c->threshold_ep, (Value){.float_v=COMP_DEFAULT_THRESHOLD});
    endpoint_set_label_fn(&c->threshold_ep, label_amp_to_dbstr);
    api_endpoint_register(&c->threshold_ep, &c->effect->api_node);

    
    endpoint_init(
	&c->ratio_ep,
	&c->ratio,
	JDAW_FLOAT,
	"ratio",
	"Ratio",
	JDAW_THREAD_DSP,
	track_settings_page_el_gui_cb, NULL, comp_ratio_dsp_cb,
        c, NULL, &c->effect->page, "track_settings_comp_ratio_slider");
    endpoint_set_allowed_range(&c->ratio_ep, (Value){.float_v=0.0f}, (Value){.float_v=1.0f});
    endpoint_set_default_value(&c->ratio_ep, (Value){.float_v=COMP_DEFAULT_RATIO});
    endpoint_set_label_fn(&c->ratio_ep, ratio_labelfn);
    api_endpoint_register(&c->ratio_ep, &c->effect->api_node);

    endpoint_init(
	&c->makeup_gain_ep,
	&c->makeup_gain,
	JDAW_FLOAT,
	"makeup_gain",
	"Make-up gain",
	JDAW_THREAD_DSP,
	track_settings_page_el_gui_cb, NULL, NULL,
	NULL, NULL, &c->effect->page, "track_settings_makeup_gain_slider");
    endpoint_set_allowed_range(&c->makeup_gain_ep, (Value){.float_v=1.0f}, (Value){.float_v=COMP_MAX_MAKEUP_GAIN});
    endpoint_set_default_value(&c->makeup_gain_ep, (Value){.float_v=1.0f});
    endpoint_set_label_fn(&c->makeup_gain_ep, label_amp_to_dbstr);
    api_endpoint_register(&c->makeup_gain_ep, &c->effect->api_node);

}


/* envelope_follower_set_times_msec(&ef, 10.0, 200.0, session->proj.sample_rate); */
