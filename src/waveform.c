/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

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

/* Non-integral so that individual channel draws on a timeline fall
   reliably to one side or the other, despite float error */
#define SFPP_THRESHOLD 15.5
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
	/* double x = la->container->w * log(i + 1) / lognsub1; */

	/* x / la->container->w = log(base nsub1)(i); */
	/* xprop = log(base nsub1)(i); */
	/* nsub ^ xprop = i; */
	/* /\* fprintf(stdout, "X %d: %f\n", i, x); *\/ */
	la->x_pos_cache[i/step] = (int)round(x) + container->x;
    }
}


static struct logscale *waveform_create_logscale(double *array, int num_items, SDL_Rect *container, SDL_Color *color, int step)
{
    struct logscale *la = calloc(1, sizeof(struct logscale));
    la->color = color;
    la->min = 0.0;
    la->range = 1.0;
    waveform_update_logscale(la, array, num_items, step, container);
    return la;
}

void logscale_set_range(struct logscale *l, double min, double max)
{
    l->min = min;
    l->range = max - min;
}

void waveform_destroy_logscale(struct logscale *la)
{
    if (la->x_pos_cache) free(la->x_pos_cache);
    free(la);
}

static const double alpha = 80.0;
double amp_to_logscaled(double amp)
{
    return log(1 + alpha * amp) / log(1 + alpha);
}

double amp_from_logscaled(double from)
{
    return (pow(1 + alpha, from) - 1) / alpha;
}

/* Draw an array of floats (e.g. frequencies) on a log scale */
void waveform_draw_freq_domain(struct logscale *la)
{
    if (la->color) {
	SDL_SetRenderDrawColor(main_win->rend, sdl_colorp_expand(la->color));
    }
    double btm_y = la->container->y + (double)la->container->h;
    double raw_amp = (la->array[0] - la->min) / la->range;
    double scaled = amp_to_logscaled(raw_amp);
    double last_y = btm_y - (amp_to_logscaled(raw_amp) * la->container->h);
    int last_x = la->x_pos_cache[0];
    /* fprintf(stderr, "START last y: %f, current_y: %f\n", ladoublest_y, current_y); */
    for (int i=la->step * 2; i<la->num_items; i+=la->step) {
	/* int last_x = la->x_pos_cache[i/la->step-1]; */
	raw_amp = (la->array[i] - la->min) / la->range;
	scaled = amp_to_logscaled(raw_amp);
	double current_y = btm_y - scaled * la->container->h;
	int current_x = la->x_pos_cache[i/la->step];
	SDL_RenderDrawLine(main_win->rend, last_x, last_y, current_x, current_y);
	last_x = current_x;
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
	/* tics[num_tics] = left_x + w * (log(omega / nyquist) / lognsub1); */
	/* double omega_raw = omega / nyquist; */
	/* double xprop = log((fp->num_items - 1) * omega_raw) / lognsub1; */
	/* tics[num_tics] = left_x + w * xprop; */
	tics[num_tics] = left_x + w * (1.0f + (log(omega) - lognyq) / lognsub1);
	if (omega == 60 || omega == 100 || omega == 200 || omega == 1000 || omega == 2000 || omega == 10000) {
	    Layout *tb_lt = layout_add_child(fp->container);
	    layout_set_default_dims(tb_lt);
	    layout_reset(tb_lt);
	    const char *str = freq_labels[omega == 60 ? 0 : omega == 100 ? 1 : omega == 200 ? 2 : omega == 1000 ? 3 : omega == 2000 ? 4 : 5];
	    Textbox *tb = textbox_create_from_str((char *)str, tb_lt, main_win->mono_font, 10, main_win);
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

double get_sample_fn(struct freq_plot *fp, void *xarg);



/* void waveform_freq_plot_update_linear_plot(struct freq_plot *fp, double calculate_point(double input, void *xarg), void *xarg) */
/* { */
/*     double *plot = fp->linear_plots[0]; */
/*     int len = fp->linear_plot_lens[0]; */
/*     int nsub1 = fp->num_items - 1; */
/*     for (int i=0; i<len; i++) { */
/* 	double prop = (double)i/len; */
/* 	double input = pow(nsub1, prop) / nsub1; */
/* 	plot[i] = calculate_point(input, xarg); */
/*     } */
 
/* } */
void waveform_freq_plot_add_linear_plot(struct freq_plot *fp, int len, double *arr, SDL_Color *color)
{
    if (fp->num_linear_plots == MAX_LINEAR_PLOTS) {
	fprintf(stderr, "ERROR: already reached max num linear plots %d\n", MAX_LINEAR_PLOTS);
	return;
    }
    /* int nsub1 = fp->num_items - 1; */
    /* double *plot = malloc(len * sizeof(double)); */
    /* for (int i=0; i<len; i++) { */
    /* 	double prop = (double)i/len; */
    /* 	double input = pow(nsub1, prop) / nsub1; */
    /* 	plot[i] = calculate_point(input, xarg); */
    /* } */
    fp->linear_plots[fp->num_linear_plots] = arr;
    fp->linear_plot_lens[fp->num_linear_plots] = len;
    fp->linear_plot_colors[fp->num_linear_plots] = color;
    fp->linear_plot_mins[fp->num_linear_plots] = 0.0;
    fp->linear_plot_ranges[fp->num_linear_plots] = 1.0;
    fp->num_linear_plots++;
}

void waveform_destroy_freq_plot(struct freq_plot *fp)
{
    if (fp->tic_cache) {
	free(fp->tic_cache);
    }
    for (int i=0; i<fp->num_plots; i++) {
	waveform_destroy_logscale(fp->plots[i]);
    }
    /* for (int i=0; i<fp->num_linear_plots; i++) { */
    /* 	free(fp->linear_plots[i]); */
    /* }	 */
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

static void waveform_draw_linear_plot(struct freq_plot *fp, int index)
{
    double *plot = fp->linear_plots[index];
    int len = fp->linear_plot_lens[index];
    SDL_Color *color = fp->linear_plot_colors[index];
    SDL_SetRenderDrawColor(main_win->rend, sdl_colorp_expand(color));

    double min = fp->linear_plot_mins[index];
    double range = fp->linear_plot_ranges[index];
    
    int btm_y = fp->container->rect.y + fp->container->rect.h;
    int h = fp->container->rect.h;
    int left_x = fp->container->rect.x;
    int w = fp->container->rect.w;

    int last_y;
    int last_x;
    int y;
    int x;
    
    for (int i=0; i<len; i++) {
	if (i==0) {
	    last_x = left_x;
	    last_y = btm_y - h * amp_to_logscaled((plot[0] - min) / range);
	    continue;
	}
	x = left_x + (double)i * w / (len - 1);
	y = btm_y - h * amp_to_logscaled((plot[i] - min) / range);
	SDL_RenderDrawLine(main_win->rend, x, y, last_x, last_y);
	last_x = x;
	last_y = y;
    }
    /* int i=0; */
    /* int last_x = left_x; */
    /* int last_y = (plot[0] - min) */
    /* do { */
    /* 	int x = left_x + (double)(i+1) * w / len; */
    /* 	double raw_amp = (plot[i] - min) / range; */
    /* 	/\* fprintf(stderr, "%d: %f\n", i, raw_amp); *\/ */
    /* 	int y = btm_y - h * (amp_to_logscaled(raw_amp)); */
    /* 	SDL_RenderDrawLine(main_win->rend, x, y, last_x, last_y); */
    /* 	last_x = x; */
    /* 	last_y = y; */
    /* 	i++; */
    /* } while (i < len); */
}

void waveform_draw_freq_plot(struct freq_plot *fp)
{
    SDL_SetRenderDrawColor(main_win->rend, 0, 0, 0, 255);
    SDL_RenderFillRect(main_win->rend, &fp->container->rect);

    if (fp->related_obj_lock) {
	pthread_mutex_lock(fp->related_obj_lock);
    }
    for (int i=0; i<fp->num_plots; i++) {
	waveform_draw_freq_domain(fp->plots[i]);
    }
    for (int i=0; i<fp->num_linear_plots; i++) {
	waveform_draw_linear_plot(fp, i);
    }
    if (fp->related_obj_lock) {
	pthread_mutex_unlock(fp->related_obj_lock);
    }


    SDL_SetRenderDrawColor(main_win->rend, 200, 200, 255, 80);
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

double waveform_freq_plot_freq_from_x_rel(struct freq_plot *fp, int rel_x)
{
    double nsub1 = fp->num_items - 1;
    double xprop = (double)rel_x / fp->container->rect.w;
    return pow(nsub1, xprop) / nsub1;
}

int waveform_freq_plot_x_abs_from_freq(struct freq_plot *fp, double freq_raw)
{
    
    double xprop = log(freq_raw * (fp->num_items - 1)) / log(fp->num_items - 1);
    return fp->container->rect.x + xprop * fp->container->rect.w;
	
}
double waveform_freq_plot_freq_from_x_abs(struct freq_plot *fp, int abs_x)
{
    return waveform_freq_plot_freq_from_x_rel(fp, abs_x - fp->container->rect.x);
}

double waveform_freq_plot_amp_from_x_rel(struct freq_plot *fp, int rel_y, int arr_i, bool linear_plot)
{
    double min, range;
    if (linear_plot) {
	min = fp->linear_plot_mins[arr_i];
	range = fp->linear_plot_ranges[arr_i];
    } else {
	struct logscale *l = fp->plots[arr_i];
	min = l->min;
	range = l->range;
    }
    double yprop = ((double)fp->container->rect.h - rel_y) / fp->container->rect.h;

    return min + range * amp_from_logscaled(yprop);    
}

int waveform_freq_plot_y_abs_from_amp(struct freq_plot *fp, double amp, int arr_i, bool linear_plot)
{
    double min, range;
    if (linear_plot) {
	min = fp->linear_plot_mins[arr_i];
	range = fp->linear_plot_ranges[arr_i];
    } else {
	struct logscale *l = fp->plots[arr_i];
	min = l->min;
	range = l->range;
    }

    double yprop = amp_to_logscaled((amp - min) / range);
    return fp->container->rect.y + fp->container->rect.h - fp->container->rect.h * yprop;
}

double waveform_freq_plot_amp_from_x_abs(struct freq_plot *fp, int abs_y, int arr_i, bool linear_plot)
{
    return waveform_freq_plot_amp_from_x_rel(fp, abs_y - fp->container->rect.y, arr_i, linear_plot);
}

static void waveform_draw_channel(float *channel, uint32_t buflen, int start_x, int w, int amp_h_max, int center_y)
{
    float sfpp = (double) buflen / w;
    
    if (sfpp <= 0) {
	fprintf(stderr, "Error in waveform_draw_channel: sfpp<=0\n");
	breakfn();
	return;
    }

    if (sfpp > SFPP_THRESHOLD) {
	/* float avg_amp_neg, avg_amp_pos; */
	float max_amp_neg, max_amp_pos;
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
	    max_amp_neg = 0;
	    max_amp_pos = 0;
	    int sfpp_safe = (int)sfpp < SFPP_SAFE ? (int)sfpp : SFPP_SAFE;
	    for (int i=0; i<sfpp_safe; i++) {
		if (sample_i + i >= buflen) {
		    break;
		}
		float sample = channel[(int)sample_i + i];
		if (sample > max_amp_pos) {
		    max_amp_pos = sample;
		} else if (sample < max_amp_neg) {
		    max_amp_neg = sample;
		}
	    }
	    /* avg_amp_neg /= sfpp_safe; */
	    /* avg_amp_pos /= sfpp_safe; */
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
	    if (max_amp_neg < -1.0f || max_amp_pos > 1.0f) {
		SDL_SetRenderDrawColor(main_win->rend, 255, 0, 0, 255);
		SDL_RenderDrawLine(main_win->rend, x, center_y - amp_h_max, x, center_y + amp_h_max);
		SDL_SetRenderDrawColor(main_win->rend, 0, 0, 0, 255);
	    } else {
		int y_offset_pos = max_amp_pos * amp_h_max;
		int y_offset_neg = max_amp_neg * amp_h_max;
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
static void waveform_draw_channel_generic(float *channel, ValType type, uint32_t buflen, int start_x, int w, int amp_h_max, int center_y, int min_x, int max_x)
{
    float sfpp = (double) buflen / w;
    if (sfpp <= 0) {
	fprintf(stderr, "Error in waveform_draw_channel: sfpp<=0\n");
	breakfn();
	return;
    }

    if (sfpp > SFPP_THRESHOLD) {
	float max_amp_neg, max_amp_pos;
	int x = start_x;
	/* int amp_y = center_y; */
	/* float sample_i = 0.0f; */
	double sample_i = 0.0;
	while (x < start_x + w && sample_i + sfpp < buflen) {
	    sample_i = buflen * ((double)(x - start_x) / w);
	    /* sample_i = buflen * ((double)x / w); */
	    /* Early exit conditions (offscreen) */
	    if (x < min_x) {
		sample_i+=sfpp*(0-x);
		x=min_x;
		continue;
	    } else if (x > max_x) {
		break;
	    }
	    
	    max_amp_neg = 0;
	    max_amp_pos = 0;
	    
	    int sfpp_safe = round(sfpp) < SFPP_SAFE ? round(sfpp) : SFPP_SAFE;
	    for (int i=0; i<sfpp_safe; i++) {
		int sample_i_rounded = (int)round(sample_i) + i;
		if (sample_i_rounded >= buflen) {
		    break;
		}
		float sample = channel[sample_i_rounded];
		if (sample > max_amp_pos) {
		    max_amp_pos = sample;
		} else if (sample < max_amp_neg) {
		    max_amp_neg = sample;
		}
	    }
	    /* avg_amp_neg /= sfpp_safe; */
	    /* avg_amp_pos /= sfpp_safe; */
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
	    if (max_amp_neg < -1.0f || max_amp_pos > 1.0f) {
		SDL_SetRenderDrawColor(main_win->rend, 255, 0, 0, 255);
		SDL_RenderDrawLine(main_win->rend, x, center_y - amp_h_max, x, center_y + amp_h_max);
		SDL_SetRenderDrawColor(main_win->rend, 0, 0, 0, 255);
	    } else {
		int y_offset_pos = max_amp_pos * amp_h_max;
		int y_offset_neg = max_amp_neg * amp_h_max;
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
	double sample_i = 0.0;
	while (x < start_x + w && sample_i + sfpp < buflen) {
	    sample_i = buflen * ((double)(x - start_x) / w);
	    avg_amp = 0;
	    if (x < min_x) {
		sample_i+=sfpp;
		x++;
		continue;
	    } else if (x > max_x) {
		break;
	    }
	    if (sfpp > 1) {
		for (int i=0; i<(int)round(sfpp); i++) {
		    if (sample_i + i >= buflen) {
			break;
		    }
		    avg_amp += channel[(int)round(sample_i) + i];
		    /* fprintf(stdout, "\t->avg amp + %f\n", channel[(int)(sample_i) + i]); */
		}
		avg_amp /= round(sfpp);
	    } else {
		avg_amp = channel[(int)round(sample_i)];
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
	    /* sample_i+=sfpp; */
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
void waveform_draw_all_channels_generic(void **channels, ValType type, uint8_t num_channels, uint32_t buflen, SDL_Rect *rect, int min_x, int max_x)
{
    int channel_h = rect->h / num_channels;
    int center_y = rect->y + channel_h / 2;
    for (uint8_t i=0; i<num_channels; i++) {
	waveform_draw_channel_generic(channels[i], type,  buflen, rect->x, rect->w, channel_h / 2, center_y, min_x, max_x);
	center_y += channel_h;
    }
}
