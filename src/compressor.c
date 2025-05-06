/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

/*****************************************************************************************************************
    compressor.c

    * basic dynamic range compression
 *****************************************************************************************************************/


#include <stdio.h>
#include "compressor.h"
#include "geometry.h"
#include "window.h"

#define COMP_GRAPH_R 4

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

float compressor_buf_apply(void *compressor_v, float *buf, int len, int channel, float input_amp)
{
    Compressor *c = compressor_v;
    float gain_reduc;
    float env;
    float output_amp = 0.0f;
    for (int i=0; i<len; i++) {
	env = envelope_follower_sample(&c->ef, buf[i]);
	float overshoot = env - c->threshold;
	if (overshoot > 0.0f) {
	    overshoot *= c->m;
	    gain_reduc = (c->threshold + overshoot) / env;
	    buf[i] *= gain_reduc;
	    buf[i] *= c->makeup_gain;
	} else {
	    buf[i] *= c->makeup_gain;
	}
	output_amp += fabs(buf[i]);
	
    }
    c->gain_reduction = gain_reduc;
    c->env = env;
    return output_amp;
    
}

void compressor_set_times_msec(Compressor *c, double attack_msec, double release_msec, double sample_rate)
{
    envelope_follower_set_times_msec(&c->ef, attack_msec, release_msec, sample_rate);
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
    SDL_SetRenderDrawColor(main_win->rend, 0, 0, 10, 255);
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
    float env = c->env;
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
/* envelope_follower_set_times_msec(&ef, 10.0, 200.0, proj->sample_rate); */
