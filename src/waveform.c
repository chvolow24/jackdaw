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
#include "logscale.h"
#include "session.h"
#include "textbox.h"
#include "waveform.h"
#include "window.h"

/* Non-integral so that individual channel draws on a timeline fall
   reliably to one side or the other, despite float error */
#ifdef TESTBUILD
#define SFPP_SAFE 400
#else
#define SFPP_SAFE 1000
#endif

#define FREQ_PLOT_MAX_TICS 255


extern Window *main_win;
extern struct colors colors;

static const double alpha = 80.0;
double amp_to_logscaled(double amp)
{
    return log(1 + alpha * amp) / log(1 + alpha);
}

double amp_from_logscaled(double from)
{
    return (pow(1 + alpha, from) - 1) / alpha;
}

/* Draw an array (either float or double) at evenly- (linearly-) spaced frequencies */
void waveform_draw_logscale(struct freq_plot *fp, double *darray, float *farray, int len, SDL_Color *color)
{
    SDL_SetRenderDrawColor(main_win->rend, sdl_colorp_expand(color));
    int start_x = fp->container->rect.x;
    int start_y = fp->container->rect.y + fp->container->rect.h;
	
    for (int i=0; i<len; i++) {
	double sample;
	if (darray) sample = darray[i];
	else sample = farray[i];
	double prop = (double)i / len;
	double freq_hz = prop * fp->x_axis.max_scaled;
	int draw_x = logscale_x_abs(&fp->x_axis, freq_hz);
	if (draw_x == start_x) continue;
	int draw_y = fp->container->rect.y + fp->container->rect.h - amp_to_logscaled(sample) * fp->container->rect.h;
	if (draw_x <= start_x) {
	    start_y = draw_y;
	    continue;
	}
	SDL_RenderDrawLine(main_win->rend, start_x, start_y, draw_x, draw_y);
	start_x = draw_x;
	start_y = draw_y;
    }
    
    /* if (la->color) { */
    /* 	SDL_SetRenderDrawColor(main_win->rend, sdl_colorp_expand(la->color)); */
    /* } */
    /* double btm_y = la->container->y + (double)la->container->h; */
    /* double raw_amp = (la->array[0] - la->min) / la->range; */
    /* double scaled = amp_to_logscaled(raw_amp); */
    /* double last_y = btm_y - (amp_to_logscaled(raw_amp) * la->container->h); */
    /* int last_x = la->x_pos_cache[0]; */
    /* /\* fprintf(stderr, "START last y: %f, current_y: %f\n", ladoublest_y, current_y); *\/ */
    /* for (int i=la->step * 2; i<la->num_items; i+=la->step) { */
    /* 	/\* int last_x = la->x_pos_cache[i/la->step-1]; *\/ */
    /* 	raw_amp = (la->array[i] - la->min) / la->range; */
    /* 	scaled = amp_to_logscaled(raw_amp); */
    /* 	double current_y = btm_y - scaled * la->container->h; */
    /* 	int current_x = la->x_pos_cache[i/la->step]; */
    /* 	SDL_RenderDrawLine(main_win->rend, last_x, last_y, current_x, current_y); */
    /* 	last_x = current_x; */
    /* 	/\* fprintf(stdout, "Draw %d %f %d %f\n", last_x, last_y, la->x_pos_cache[i/la->step], current_y); *\/ */
    /* 	last_y = current_y; */
    /* } */
}


void waveform_reset_freq_plot(struct freq_plot *fp)
{
    for (int i=0; i<fp->num_labels; i++) {
	textbox_destroy(fp->labels[i]);
    }
    int tics[FREQ_PLOT_MAX_TICS];
    int num_tics = 0;
    double nyquist = (double)session_get_sample_rate() / 2;
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
	if (omega < fp->x_axis.min_scaled) goto increment_omega;
	tics[num_tics] = logscale_x_abs(&fp->x_axis, omega);
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
	    textbox_set_background_color(tb, &colors.black);
	    textbox_set_text_color(tb, &colors.white);
	    textbox_reset_full(tb);
	}
	num_tics++;
    increment_omega:
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

struct freq_plot *waveform_create_freq_plot(
    double **darrays,
    int *darray_lens,
    int num_darrays,
    float **farrays,
    int *farray_lens,
    int num_farrays,
    SDL_Color **darray_colors,
    SDL_Color **farray_colors,
    double min_freq_hz,
    double max_freq_hz,
    Layout *container)
{
    struct freq_plot *fp = calloc(1, sizeof(struct freq_plot));
    fp->container = container;
    if (num_darrays > 0) {
	fp->darrays = calloc(num_darrays, sizeof(double *));
	memcpy(fp->darrays, darrays, num_darrays * sizeof(double *));
	fp->darray_lens = calloc(num_darrays, sizeof(int));
	memcpy(fp->darray_lens, darray_lens, num_darrays * sizeof(int));
	fp->dcolors = calloc(num_darrays, sizeof(SDL_Color *));
        memcpy(fp->dcolors, darray_colors, num_darrays * sizeof(SDL_Color *));
    }
    if (num_farrays > 0) {
	fp->farrays = calloc(num_farrays, sizeof(float *));
	memcpy(fp->farrays, farrays, num_farrays * sizeof(float *));
	fp->farray_lens = calloc(num_darrays, sizeof(int));
	memcpy(fp->farray_lens, farray_lens, num_farrays * sizeof(int));
	fp->fcolors = calloc(num_farrays, sizeof(SDL_Color *));
        memcpy(fp->fcolors, farray_colors, num_farrays * sizeof(SDL_Color *));	    	
    }
    fp->num_darrays = num_darrays;
    fp->num_farrays = num_farrays;
    logscale_init(&fp->x_axis, &container->rect, min_freq_hz, max_freq_hz);
    waveform_reset_freq_plot(fp);
    return fp;
}

/* double get_sample_fn(struct freq_plot *fp, void *xarg); */



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
    if (fp->darrays) {
	free(fp->darrays);
	free(fp->darray_lens);
	free(fp->dcolors);
    }
    if (fp->farrays) {
	free(fp->farrays);
	free(fp->farray_lens);
	free(fp->fcolors);
    }
    for (int i=0; i<fp->num_labels; i++) {
	textbox_destroy(fp->labels[i]);
    }
    layout_destroy(fp->container);
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
}

void waveform_draw_freq_plot(struct freq_plot *fp)
{
    SDL_SetRenderDrawColor(main_win->rend, 0, 0, 0, 255);
    SDL_RenderFillRect(main_win->rend, &fp->container->rect);

    if (fp->related_obj_lock) {
	pthread_mutex_lock(fp->related_obj_lock);
    }
    for (int i=0; i<fp->num_darrays; i++) {
	waveform_draw_logscale(fp, fp->darrays[i], NULL, fp->darray_lens[i], fp->dcolors[i]);
    }
    for (int i=0; i<fp->num_farrays; i++) {
	waveform_draw_logscale(fp, NULL, fp->farrays[i], fp->farray_lens[i], fp->fcolors[i]);
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
	textbox_draw(fp->labels[i]);
    }
}

int waveform_freq_plot_y_abs_from_amp(struct freq_plot *fp, double amp, int arr_i, bool linear_plot)
{
    double min, range;
    if (linear_plot) {
	min = fp->linear_plot_mins[arr_i];
	range = fp->linear_plot_ranges[arr_i];
    } else {
	min = 0.0;
	range = 1.0;
	/* struct logscale *l = fp->plots[arr_i]; */
	/* min = l->min; */
	/* range = l->range; */
    }

    double yprop = amp_to_logscaled((amp - min) / range);
    return fp->container->rect.y + fp->container->rect.h - fp->container->rect.h * yprop;
}


double waveform_freq_plot_freq_from_x_rel(struct freq_plot *fp, int rel_x)
{
    /* double nsub1 = fp->num_items - 1; */
    /* double xprop = (double)rel_x / fp->container->rect.w; */
    /* return pow(nsub1, xprop) / nsub1; */
    double freq_hz = logscale_val_from_x_rel(&fp->x_axis, rel_x);
    return freq_hz / fp->x_axis.max_scaled;
}

int waveform_freq_plot_x_abs_from_freq(struct freq_plot *fp, double freq_raw)
{
    /* double xprop = log(freq_raw * (fp->num_items - 1)) / log(fp->num_items - 1); */
    /* return fp->container->rect.x + xprop * fp->container->rect.w; */
    double freq_hz = freq_raw * fp->x_axis.max_scaled;
    return logscale_x_abs(&fp->x_axis, freq_hz);
	
}
double waveform_freq_plot_freq_from_x_abs(struct freq_plot *fp, int abs_x)
{
    return waveform_freq_plot_freq_from_x_rel(fp, abs_x - fp->container->rect.x);
}

double waveform_freq_plot_amp_from_y_rel(struct freq_plot *fp, int rel_y, int arr_i, bool linear_plot)
{
    double min, range;
    if (linear_plot) {
	min = fp->linear_plot_mins[arr_i];
	range = fp->linear_plot_ranges[arr_i];
    } else {
	min = 0.0;
	range = 1.0;
    }
    double yprop = ((double)fp->container->rect.h - rel_y) / fp->container->rect.h;

    return min + range * amp_from_logscaled(yprop);
}

/* int waveform_freq_plot_y_abs_from_amp(struct freq_plot *fp, double amp, int arr_i, bool linear_plot) */
/* { */
/*     double min, range; */
/*     if (linear_plot) { */
/* 	min = fp->linear_plot_mins[arr_i]; */
/* 	range = fp->linear_plot_ranges[arr_i]; */
/*     } else { */
/* 	struct logscale *l = fp->plots[arr_i]; */
/* 	min = l->min; */
/* 	range = l->range; */
/*     } */

/*     double yprop = amp_to_logscaled((amp - min) / range); */
/*     return fp->container->rect.y + fp->container->rect.h - fp->container->rect.h * yprop; */
/* } */

double waveform_freq_plot_amp_from_y_abs(struct freq_plot *fp, int abs_y, int arr_i, bool linear_plot)
{
    return waveform_freq_plot_amp_from_y_rel(fp, abs_y - fp->container->rect.y, arr_i, linear_plot);
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
static void waveform_draw_channel_generic(float *channel, ValType type, uint32_t buflen, int start_x, int w, int amp_h_max, int center_y, int min_x, int max_x, double sfpp, SDL_Color *color, float gain)
{
    /* float sfpp = (double) buflen / w; */
    if (sfpp <= 0) {
	fprintf(stderr, "Error in waveform_draw_channel: sfpp<=0\n");
	breakfn();
	return;
    }
    /* Session *session = session_get(); */
    /* sfpp = ACTIVE_TL->timeview.sample_frames_per_pixel; */

    SDL_SetRenderDrawColor(main_win->rend, sdl_colorp_expand(color));
    if (sfpp > SFPP_THRESHOLD) {
	float max_amp_neg, max_amp_pos;
	int x = start_x;
	/* int amp_y = center_y; */
	/* float sample_i = 0.0f; */
	double sample_i = 0.0;
	/* fprintf(stderr, "OVER THRESH, sfpp = %f, start x  = %d\n", sfpp, start_x); */
	/* int iters = -1; */
	while (x < start_x + w && sample_i + sfpp < buflen) {
	    /* N.B.: the below v computation of the sample index was designed to
	       avoid cumulative fp error resulting from repeatedly incrementing
	       the index by sfpp. However, this resulted in waveforms that morphed
	       oddly as the clip end bound changed (or when cutting a clip, etc.)
	    */
	    /* sample_i = buflen * ((double)(x - start_x) / w); */
	    
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
	    /* int sfpp_safe = round(sfpp); */
	    /* iters++; */
	    /* if (iters < 3) { */
	    /* 	fprintf(stderr, "\tsample_i: %f, sfpp safe %d\n", sample_i, sfpp_safe); */
	    /* } */

	    for (int i=0; i<sfpp_safe; i++) {
		int sample_i_rounded = (int)round(sample_i) + i;
		if (sample_i_rounded >= buflen) {
		    break;
		}
		float sample = gain * channel[sample_i_rounded];
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
		SDL_SetRenderDrawColor(main_win->rend, sdl_colorp_expand(color));
		/* SDL_SetRenderDrawColor(main_win->rend, 0, 0, 0, 255); */
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
	    avg_amp *= gain;
	    int sample_y = center_y + avg_amp * amp_h_max;
	    /* int sample_y; */
	    if (x == start_x) {last_sample_y = sample_y;}
	    if (fabs(avg_amp) > 1.0f) {
		SDL_SetRenderDrawColor(main_win->rend, 255, 0, 0, 255);
		SDL_RenderDrawLine(main_win->rend, x-1, center_y - amp_h_max, x-1, center_y + amp_h_max);
		SDL_RenderDrawLine(main_win->rend, x, center_y - amp_h_max, x, center_y + amp_h_max);
		SDL_SetRenderDrawColor(main_win->rend, sdl_colorp_expand(color));
		/* SDL_SetRenderDrawColor(main_win->rend, 0, 0, 0, 255); */
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
void waveform_draw_all_channels_generic(void **channels, ValType type, uint8_t num_channels, uint32_t buflen, SDL_Rect *rect, int min_x, int max_x, double sfpp, SDL_Color *color, float gain)
{
    int channel_h = rect->h / num_channels;
    int center_y = rect->y + channel_h / 2;
    for (uint8_t i=0; i<num_channels; i++) {
	waveform_draw_channel_generic(channels[i], type,  buflen, rect->x, rect->w, channel_h / 2, center_y, min_x, max_x, sfpp, color, gain);
	center_y += channel_h;
    }
}
