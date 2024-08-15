/*****************************************************************************************************************
  Jackdaw | a stripped-down, keyboard-focused Digital Audio Workstation | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023 Charlie Volow
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software iso
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
    waveform.c

    * algorithm to draw waveforms inside rectangles
    * incl. multi-channel audio
 *****************************************************************************************************************/

#include "color.h"
#include "textbox.h"
#include "waveform.h"
#include "SDL_render.h"
#include "window.h"
#include "project.h"

#define SFPP_THRESHOLD 15
#define SFPP_SAFE 200

#define FREQ_PLOT_MAX_TICS 255

extern Project *proj;
extern Window *main_win;

extern SDL_Color color_global_black;
extern SDL_Color color_global_white;

void waveform_update_logscale(struct logscale *la, double *array, int num_items, int step, SDL_Rect *container)
{
    la->container = container;
    la->array = array;
    la->num_items = num_items;
    la->step = step;
    if (la->x_pos_cache) free(la->x_pos_cache);
    la->x_pos_cache = malloc(sizeof(int) * num_items);
    double lognsub1 = log(num_items - 1);
    for (int i=0; i<la->num_items; i+=step) {
	double x = i==0? 0 : la->container->w * log(i) / lognsub1;
	/* fprintf(stdout, "X %d: %f\n", i, x); */
	la->x_pos_cache[i/step] = (int)round(x) + container->x;
    }
}


static struct logscale *waveform_create_logscale(double *array, int num_items, SDL_Rect *container, SDL_Color *color, int step)
{
    struct logscale *la = calloc(1, sizeof(struct logscale));
    la->color = color;
    waveform_update_logscale(la, array, num_items, step, container);
    return la;
}

void waveform_destroy_logscale(struct logscale *la)
{
    if (la->x_pos_cache) free(la->x_pos_cache);
    free(la);
}

/* Draw an array of floats (e.g. frequencies) on a log scale */
void waveform_draw_freq_domain(struct logscale *la)
{
    if (la->color) {
	SDL_SetRenderDrawColor(main_win->rend, sdl_colorp_expand(la->color));
    }
    /* static double epsilon = 1e-12; */
    /* double min_db = 20.0f * log10(epsilon); */
    /* double max_db = 20.0f * log10(1.0f); */
    /* double db_range = max_db - min_db; */
    double scale = 1.0f / log(2.0f);
    double btm_y = la->container->y + (double)la->container->h;
    /* double db = 20.0f * log10(epsilon + la->array[1]); */
    /* double last_y = btm_y - ((db - min_db) / db_range) * la->container->h; */
    double last_y = btm_y - log(1 + la->array[1]) * scale * la->container->h;
    double current_y = btm_y;
    for (int i=2; i<la->num_items; i+=la->step) {
	int last_x = la->x_pos_cache[i/la->step-1];
	/* db = 20.0f * log10(epsilon + la->array[i]); */
	/* current_y = btm_y - ((db - min_db) / db_range) * la->container->h; */
	current_y = btm_y - log(1 + la->array[i]) * scale * la->container->h;
	SDL_RenderDrawLine(main_win->rend, last_x, last_y, la->x_pos_cache[i/la->step], current_y);
	/* fprintf(stdout, "Draw %d %f %d %f\n", last_x, last_y, la->x_pos_cache[i/la->step], current_y); */
	last_y = current_y;
    }
}


void waveform_reset_freq_plot(struct freq_plot *fp)
{

    for (int i=0; i<fp->num_plots; i++) {
	struct logscale *ls;
	if ((ls = fp->plots[i])) waveform_destroy_logscale(ls);
	fp->plots[i] = waveform_create_logscale(fp->arrays[i], fp->num_items, &fp->container->rect, fp->colors[i], fp->steps[i]);
    }
    for (int i=0; i<fp->num_labels; i++) {
	textbox_destroy(fp->labels[i]);
    }
    int tics[FREQ_PLOT_MAX_TICS];
    int num_tics = 0;
    double nyquist = (double)proj->sample_rate / 2;
    double lognyq  = log(nyquist);
    int left_x = fp->container->rect.x;
    int w = fp->container->rect.w;
    double lognsub1 = log(fp->num_items - 1);
    double omega = 50.0f;
    static const char *freq_labels[] = {
	"60 Hz",
	"100 Hz",
	"200 Hz",
	"1 KHz",
	"2 KHz",
	"10 KHz"
    };
    fp->num_labels = 0;
    while (1) {

	tics[num_tics] = left_x + w * (1.0f + (log(omega) - lognyq) / lognsub1);
	if (omega == 60 || omega == 100 || omega == 200 || omega == 1000 || omega == 2000 || omega == 10000) {
	    Layout *tb_lt = layout_add_child(fp->container);
	    layout_set_default_dims(tb_lt);
	    layout_reset(tb_lt);
	    /* fprintf(stdout, "OK %d %d %d %d\n", tb_lt->rect.x, tb_lt->rect.y, tb_lt->rect.w, tb_lt->rect.h); */
	    const char *str = freq_labels[omega == 60 ? 0 : omega == 100 ? 1 : omega == 200 ? 2 : omega == 1000 ? 3 : omega == 2000 ? 4 : 5];
	    Textbox *tb = textbox_create_from_str((char *)str, tb_lt, main_win->mono_font, 14, main_win);
	    fp->labels[fp->num_labels] = tb;
	    fp->num_labels++;
	    /* textbox_reset_full(fp->labels[fp->num_labels]); */
	    textbox_pad(tb, 0);
	    tb_lt->rect.x = tics[num_tics] - tb_lt->rect.w / 2;
	    tb_lt->rect.y = fp->container->rect.y;
	    layout_set_values_from_rect(tb_lt);
	    textbox_set_background_color(tb, &color_global_black);
	    textbox_set_text_color(tb, &color_global_white);
	    textbox_reset_full(tb);
	}
	num_tics++;
	/* fprintf(stdout, "omega : %f\n", omega); */
	if (omega < 100) {
	    omega += 10;
	} else if (omega < 1000) {
	    omega += 100;
	} else if (omega < 10000) {
	    omega += 1000;
	} else if (omega + 10000 < nyquist) {
	    omega += 10000;
	} else {
	    break;
	}	
    }
    if (fp->tic_cache) free(fp->tic_cache);
    fp->tic_cache = malloc(sizeof(int) * num_tics);
    memcpy(fp->tic_cache, tics, sizeof(int) * num_tics);
    fp->num_tics = num_tics;

}

struct freq_plot *waveform_create_freq_plot(double **arrays, int num_plots, SDL_Color **colors, int *steps, int num_items, Layout *container)
{
    struct freq_plot *fp = calloc(1, sizeof(struct freq_plot));
    fp->container = container;
    fp->arrays = calloc(num_plots, sizeof(double *));
    fp->colors = calloc(num_plots, sizeof(SDL_Color *));
    fp->steps = calloc(num_plots, sizeof(int));
    memcpy(fp->arrays, arrays, num_plots * sizeof(double *));
    memcpy(fp->colors, colors, num_plots * sizeof(SDL_Color *));
    memcpy(fp->steps, steps, num_plots * sizeof(int));
    fp->num_plots = num_plots;
    fp->num_items = num_items;
    fp->plots = calloc(num_plots, sizeof(struct logscale *));
    waveform_reset_freq_plot(fp);
    return fp;
}

void waveform_destroy_freq_plot(struct freq_plot *fp)
{
    if (fp->tic_cache) {
	free(fp->tic_cache);
    }
    for (int i=0; i<fp->num_plots; i++) {
	waveform_destroy_logscale(fp->plots[i]);

    }
    for (int i=0; i<fp->num_labels; i++) {
	textbox_destroy(fp->labels[i]);
    }
    free(fp->arrays);
    free(fp->colors);
    free(fp->steps);
    layout_destroy(fp->container);
    free(fp->plots);
    free(fp);
}

void waveform_draw_freq_plot(struct freq_plot *fp)
{
    SDL_SetRenderDrawColor(main_win->rend, 0, 0, 0, 255);
    SDL_RenderFillRect(main_win->rend, &fp->container->rect);
    
    for (int i=0; i<fp->num_plots; i++) {
	waveform_draw_freq_domain(fp->plots[i]);
    }

    SDL_SetRenderDrawColor(main_win->rend, 255, 255, 255, 70);
    int top_y = fp->container->rect.y;
    int btm_y = top_y + fp->container->rect.h;
    for (int i=0; i<fp->num_tics; i++) {
	SDL_RenderDrawLine(main_win->rend, fp->tic_cache[i], top_y, fp->tic_cache[i], btm_y);
    }
    for (int i=0; i<fp->num_labels; i++) {
	/* fprintf(stdout, "TB draw %d\n", i); */
	/* SDL_Rect r= fp->labels[i]->layout->rect; */
	/* fprintf(stdout, "%d %d %d %d\n", r.x, r.y, r.w, r.h);  */
	textbox_draw(fp->labels[i]);
    }
}


static void waveform_draw_channel(float *channel, uint32_t buflen, int start_x, int w, int amp_h_max, int center_y)
{
    float sfpp = (double) buflen / w;
    
    if (sfpp <= 0) {
	fprintf(stderr, "Error in waveform_draw_channel: sfpp<=0\n");
	return;
    }

    if (sfpp > SFPP_THRESHOLD) {
	float avg_amp_neg, avg_amp_pos;
	int x = start_x;
	/* int amp_y = center_y; */
	float sample_i = 0.0f;
	while (x < start_x + w && sample_i + sfpp < buflen) {
	    /* Early exit conditions (offscreen) */
	    if (x < 0) {
		sample_i+=sfpp*(0-x);
		x=0;
		continue;
	    } else if (x > main_win->w_pix) {
		break;
	    }
	    
	    /* Get avg amplitude value */
	    avg_amp_neg = 0;
	    avg_amp_pos = 0;
	    int sfpp_safe = (int)sfpp < SFPP_SAFE ? (int)sfpp : SFPP_SAFE;
	    for (int i=0; i<sfpp_safe; i++) {
		if (sample_i + i >= buflen) {
		    break;
		}
		float sample;
		if ((sample = channel[(int)sample_i + i]) < 0) {
		    avg_amp_neg += sample;
		} else {
		    avg_amp_pos += sample;
		}
	    }
	    avg_amp_neg /= sfpp_safe;
	    avg_amp_pos /= sfpp_safe;
	    /* for (int i=0; i<(int)sfpp; i++) { */
	    /* 	if (sample_i + i >= buflen) { */
	    /* 	    break; */
	    /* 	} */
	    /* 	float sample; */
	    /* 	/\* fprintf(stdout, "wav i=%d\n", i); *\/ */
	    /* 	if ((sample = channel[(int)sample_i + i]) < 0) { */
	    /* 	    avg_amp_neg += sample; */
	    /* 	} else { */
	    /* 	    avg_amp_pos += sample; */
	    /* 	} */
	    /* 	/\* avg_amp += fabs(channel[(int)sample_i + i]); *\/ */
	    /* } */
	    /* avg_amp_neg /= sfpp; */
	    /* avg_amp_pos /= sfpp; */
	    if (avg_amp_neg < -1.0f || avg_amp_pos > 1.0f) {
		SDL_SetRenderDrawColor(main_win->rend, 255, 0, 0, 255);
		SDL_RenderDrawLine(main_win->rend, x, center_y - amp_h_max, x, center_y + amp_h_max);
		SDL_SetRenderDrawColor(main_win->rend, 0, 0, 0, 255);
	    } else {
		int y_offset_pos = avg_amp_pos * amp_h_max;
		int y_offset_neg = avg_amp_neg * amp_h_max;
		SDL_RenderDrawLine(main_win->rend, x, center_y + y_offset_neg, x, center_y + y_offset_pos);
	    }
	    sample_i+=sfpp;
	    x++;
	}
    } else {
	float avg_amp = 0;
	int x = start_x;
	/* int sample_y = center_y; */
	int last_sample_y = center_y;
	float sample_i = 0.0f;
	while (x < start_x + w && sample_i + sfpp < buflen) {
	    avg_amp = 0;
	    if (x < 0) {
		sample_i+=sfpp;
		x++;
		continue;
	    } else if (x > main_win->w_pix) {
		break;
	    }
	    if (sfpp > 1) {
		for (int i=0; i<(int)sfpp; i++) {
		    if (sample_i + i >= buflen) {
			break;
		    }
		    avg_amp += channel[(int)sample_i + i];
		    /* fprintf(stdout, "\t->avg amp + %f\n", channel[(int)(sample_i) + i]); */
		}
		avg_amp /= sfpp;
	    } else {
		avg_amp = channel[(int)sample_i];
	    }
	    int sample_y = center_y + avg_amp * amp_h_max;
	    if (x == start_x) {last_sample_y = sample_y;}
	    if (fabs(avg_amp) > 1.0f) {
		SDL_SetRenderDrawColor(main_win->rend, 255, 0, 0, 255);
		SDL_RenderDrawLine(main_win->rend, x-1, center_y - amp_h_max, x-1, center_y + amp_h_max);
		SDL_RenderDrawLine(main_win->rend, x, center_y - amp_h_max, x, center_y + amp_h_max);
		SDL_SetRenderDrawColor(main_win->rend, 0, 0, 0, 255);
		last_sample_y = avg_amp > 0 ? center_y + amp_h_max : center_y - amp_h_max;
	    } else {
		/* if (channel == proj->output_L && sample_i < 10) { */
		/*     fprintf(stdout, "\t->sfpp: %f, avgamp: %f, y_off: %d\n", sfpp, avg_amp, (int)(avg_amp * amp_h_max)); */
		/* } */
		SDL_RenderDrawLine(main_win->rend, x-1, last_sample_y, x, sample_y);
		last_sample_y = sample_y;
	    }
	    sample_i+=sfpp;
	    x++;
	}
	
    }
}


/* TODO: rename function and deprecate old one */
static void waveform_draw_channel_generic(float *channel, ValType type, uint32_t buflen, int start_x, int w, int amp_h_max, int center_y)
{
    float sfpp = (double) buflen / w;
    
    if (sfpp <= 0) {
	fprintf(stderr, "Error in waveform_draw_channel: sfpp<=0\n");
	return;
    }

    if (sfpp > SFPP_THRESHOLD) {
	float avg_amp_neg, avg_amp_pos;
	int x = start_x;
	/* int amp_y = center_y; */
	float sample_i = 0.0f;
	while (x < start_x + w && sample_i + sfpp < buflen) {
	    /* Early exit conditions (offscreen) */
	    if (x < 0) {
		sample_i+=sfpp*(0-x);
		x=0;
		continue;
	    } else if (x > main_win->w_pix) {
		break;
	    }
	    
	    /* Get avg amplitude value */
	    avg_amp_neg = 0;
	    avg_amp_pos = 0;
	    int sfpp_safe = (int)sfpp < SFPP_SAFE ? (int)sfpp : SFPP_SAFE;
	    for (int i=0; i<sfpp_safe; i++) {
		if (sample_i + i >= buflen) {
		    break;
		}
		float sample;
		if ((sample = channel[(int)sample_i + i]) < 0) {
		    avg_amp_neg += sample;
		} else {
		    avg_amp_pos += sample;
		}
	    }
	    avg_amp_neg /= sfpp_safe;
	    avg_amp_pos /= sfpp_safe;
	    /* for (int i=0; i<(int)sfpp; i++) { */
	    /* 	if (sample_i + i >= buflen) { */
	    /* 	    break; */
	    /* 	} */
	    /* 	float sample; */
	    /* 	/\* fprintf(stdout, "wav i=%d\n", i); *\/ */
	    /* 	if ((sample = channel[(int)sample_i + i]) < 0) { */
	    /* 	    avg_amp_neg += sample; */
	    /* 	} else { */
	    /* 	    avg_amp_pos += sample; */
	    /* 	} */
	    /* 	/\* avg_amp += fabs(channel[(int)sample_i + i]); *\/ */
	    /* } */
	    /* avg_amp_neg /= sfpp; */
	    /* avg_amp_pos /= sfpp; */
	    if (avg_amp_neg < -1.0f || avg_amp_pos > 1.0f) {
		SDL_SetRenderDrawColor(main_win->rend, 255, 0, 0, 255);
		SDL_RenderDrawLine(main_win->rend, x, center_y - amp_h_max, x, center_y + amp_h_max);
		SDL_SetRenderDrawColor(main_win->rend, 0, 0, 0, 255);
	    } else {
		int y_offset_pos = avg_amp_pos * amp_h_max;
		int y_offset_neg = avg_amp_neg * amp_h_max;
		SDL_RenderDrawLine(main_win->rend, x, center_y + y_offset_neg, x, center_y + y_offset_pos);
	    }
	    sample_i+=sfpp;
	    x++;
	}
    } else {
	float avg_amp = 0;
	int x = start_x;
	/* int sample_y = center_y; */
	int last_sample_y = center_y;
	float sample_i = 0.0f;
	while (x < start_x + w && sample_i + sfpp < buflen) {
	    avg_amp = 0;
	    if (x < 0) {
		sample_i+=sfpp;
		x++;
		continue;
	    } else if (x > main_win->w_pix) {
		break;
	    }
	    if (sfpp > 1) {
		for (int i=0; i<(int)sfpp; i++) {
		    if (sample_i + i >= buflen) {
			break;
		    }
		    avg_amp += channel[(int)sample_i + i];
		    /* fprintf(stdout, "\t->avg amp + %f\n", channel[(int)(sample_i) + i]); */
		}
		avg_amp /= sfpp;
	    } else {
		avg_amp = channel[(int)sample_i];
	    }
	    int sample_y = center_y + avg_amp * amp_h_max;
	    if (x == start_x) {last_sample_y = sample_y;}
	    if (fabs(avg_amp) > 1.0f) {
		SDL_SetRenderDrawColor(main_win->rend, 255, 0, 0, 255);
		SDL_RenderDrawLine(main_win->rend, x-1, center_y - amp_h_max, x-1, center_y + amp_h_max);
		SDL_RenderDrawLine(main_win->rend, x, center_y - amp_h_max, x, center_y + amp_h_max);
		SDL_SetRenderDrawColor(main_win->rend, 0, 0, 0, 255);
		last_sample_y = avg_amp > 0 ? center_y + amp_h_max : center_y - amp_h_max;
	    } else {
		/* if (channel == proj->output_L && sample_i < 10) { */
		/*     fprintf(stdout, "\t->sfpp: %f, avgamp: %f, y_off: %d\n", sfpp, avg_amp, (int)(avg_amp * amp_h_max)); */
		/* } */
		SDL_RenderDrawLine(main_win->rend, x-1, last_sample_y, x, sample_y);
		last_sample_y = sample_y;
	    }
	    sample_i+=sfpp;
	    x++;
	}
	
    }
}

void waveform_draw_all_channels(float **channels, uint8_t num_channels, uint32_t buflen, SDL_Rect *rect)
{
    int channel_h = rect->h / num_channels;
    int center_y = rect->y + channel_h / 2;
    for (uint8_t i=0; i<num_channels; i++) {
	/* fprintf(stdout, "DRAW ALL CHANNELS: channels? %p, %p\n", channels[0], channels[1]); */
	/* fprintf(stdout, "Drawing channel w %d, x %d, buflen %ul\n", rect->w, rect->x, buflen); */
	waveform_draw_channel(channels[i], buflen, rect->x, rect->w, channel_h / 2, center_y);
	/* if (proj && channels == &proj->output_L) { */
	/*     fprintf(stdout, "rect h? %d, maxamp h? %d\n", rect->h, channel_h / 2); */
	/* } */
	center_y += channel_h;
    }
}

/* TODO: rename and deprecate old function */
void waveform_draw_all_channels_generic(void **channels, ValType type, uint8_t num_channels, uint32_t buflen, SDL_Rect *rect)
{
    int channel_h = rect->h / num_channels;
    int center_y = rect->y + channel_h / 2;
    for (uint8_t i=0; i<num_channels; i++) {
	waveform_draw_channel_generic(channels[i], type,  buflen, rect->x, rect->w, channel_h / 2, center_y);
	center_y += channel_h;
    }
}
