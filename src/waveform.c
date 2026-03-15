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

#include "audio_clip.h"
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

void waveform_freq_plot_add_linear_plot(struct freq_plot *fp, int len, double *arr, SDL_Color *color)
{
    if (fp->num_linear_plots == MAX_LINEAR_PLOTS) {
	fprintf(stderr, "ERROR: already reached max num linear plots %d\n", MAX_LINEAR_PLOTS);
	return;
    }
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

double waveform_freq_plot_amp_from_y_abs(struct freq_plot *fp, int abs_y, int arr_i, bool linear_plot)
{
    return waveform_freq_plot_amp_from_y_rel(fp, abs_y - fp->container->rect.y, arr_i, linear_plot);
}

static bool check_clip(float *min, float *max)
{
    bool clipped;
    float *pts[2] = {min, max};
    for (int i=0; i<2; i++) {
	if (*pts[i] > 1.0f) {
	    *pts[i] = 1.0f;
	    clipped = true;
	} else if (*pts[i] < -1.0f) {
	    *pts[i] = -1.0f;
	    clipped = true;
	}
    }
    return clipped;
}

void waveform_draw_channel(float *buf, int32_t len, int start_x, int max_x, int channel_h, int center_y, double sfpp, SDL_Color *color, float gain)
{
    SDL_SetRenderDrawColor(main_win->rend, 60, 60, 60, 255);
    SDL_RenderDrawLine(main_win->rend, start_x, center_y, max_x, center_y);
    SDL_SetRenderDrawColor(main_win->rend, sdl_colorp_expand(color));
    double index_d = 0.0;
    for (int x = start_x; x <= max_x; x++) {
	int32_t index = floor(index_d);
	int32_t end_index = ceil(index_d + sfpp);
	float min = 1.0;
	float max = -1.0;
	while (index < end_index && index < len) {
	    min = fminf(min, buf[index]);
	    max = fmaxf(max, buf[index]);
	    index++;
	}
	min *= gain;
	max *= gain;
	bool clipped = check_clip(&min, &max);
	if (clipped) {
	    SDL_SetRenderDrawColor(main_win->rend, 255, 0, 0, 255);
	}
	SDL_RenderDrawLine(main_win->rend, x, center_y - max * channel_h / 2, x, center_y - min * channel_h / 2);
	if (clipped) {
	    SDL_SetRenderDrawColor(main_win->rend, 255, 0, 0, 100);
	    SDL_RenderDrawLine(main_win->rend, x, center_y - channel_h / 2, x, center_y + channel_h / 2);
	    SDL_SetRenderDrawColor(main_win->rend, sdl_colorp_expand(color));
	}
	index_d += sfpp;
    }
}

void waveform_draw_with_ck_data(WaveformData *wd, const int32_t start_in_clip, int32_t draw_len, SDL_Rect *waveform_container, double sfpp, SDL_Color *draw_color, float gain)
{
    SDL_SetRenderDrawColor(main_win->rend, sdl_colorp_expand(draw_color));
    /* fprintf(stderr, "Channel h (%d / %d) == %d\n", waveform_container->h, wd->num_channels, waveform_container->h / wd->num_channels); */
    int min_x = waveform_container->x;
    int max_x = waveform_container->x + waveform_container->w - 1;
    int channel_h = waveform_container->h / wd->num_channels;
    int center_y = waveform_container->y + channel_h / 2;
    int32_t index_divider;
    int32_t max_chunk;
    if (sfpp < 64) {
	waveform_draw_channel(wd->clip->L + start_in_clip, draw_len, min_x, max_x, channel_h, center_y, sfpp, draw_color, gain);
	if (wd->clip->R) {
	    waveform_draw_channel(wd->clip->R + start_in_clip, draw_len, min_x, max_x, channel_h, center_y + channel_h, sfpp, draw_color, gain);
	}
        /* waveform_draw_channel_generic(wd->clip->L + start_in_clip, JDAW_FLOAT, draw_len, min_x, max_x, channel_h, center_y, min_x, max_x, sfpp, draw_color, gain); */
	return;
    } else if (sfpp < 512) {
	index_divider = 64;
	max_chunk = wd->num_ck64 - 1;
    } else {
	index_divider = 512;
	max_chunk = wd->num_ck64 / 8 - 1;
    }

    pthread_mutex_lock(&wd->lock);
    int32_t chunks_per_pixel = ceil(sfpp / index_divider);
    WaveformChunk *sel_chunks_L = sfpp < 512 ? wd->ck64[0] : wd->ck512[0];
    WaveformChunk *sel_chunks_R = sfpp < 512 ? wd->ck64[1] : wd->ck512[1];
    double start_in_clip_d = start_in_clip;
    for (int x = min_x; x < max_x; x++) {
	int32_t first_chunk_index = start_in_clip_d / index_divider;
	float min = 1.0;
	float max = -1.0;
	for (int32_t i=0; i<chunks_per_pixel; i++) {
	    int32_t chunk_i = first_chunk_index + i;
	    if (chunk_i > max_chunk) break;
	    WaveformChunk ck = sel_chunks_L[chunk_i];
	    min = fminf(min, ck.min);
	    max = fmaxf(max, ck.max);
	}
	min *= gain;
	max *= gain;
	/* bool overshoot = false; */
	/* if (min < -1) { */
	/*     overshoot = true; */
	/*     SDL_SetRenderDrawColor(main_win->rend, 255, 0, 0, 255); */
	/*     min = -1; */
	/* } */
	/* if (max > 1) { */
	/*     overshoot = true; */
	/*     SDL_SetRenderDrawColor(main_win->rend, 255, 0, 0, 255); */
	/*     max = 1; */
	/* } */
	bool clipped = check_clip(&min, &max);
	if (clipped) {
	    SDL_SetRenderDrawColor(main_win->rend, 255, 0, 0, 255);
	}

	SDL_RenderDrawLine(main_win->rend, x, center_y - max * channel_h / 2, x, center_y - min * channel_h / 2);
	start_in_clip_d += sfpp;
	if (clipped) {
	    SDL_SetRenderDrawColor(main_win->rend, sdl_colorp_expand(draw_color));
	}
    }
    if (wd->num_channels > 1) {
	center_y += channel_h;
	start_in_clip_d = start_in_clip;
	for (int x = min_x; x < max_x; x++) {
	    int32_t first_chunk_index = start_in_clip_d / index_divider;
	    float min = 1.0;
	    float max = -1.0;
	    for (int32_t i=0; i<chunks_per_pixel; i++) {
		int32_t chunk_i = first_chunk_index + i;
		if (chunk_i > max_chunk) break;
		WaveformChunk ck = sel_chunks_R[chunk_i];
		min = fminf(min, ck.min);
		max = fmaxf(max, ck.max);
	    }
	    min *= gain;
	    max *= gain;
	    /* bool overshoot = false; */
	    /* if (min < -1) { */
	    /* 	overshoot = true; */
	    /* 	SDL_SetRenderDrawColor(main_win->rend, 255, 0, 0, 255); */
	    /* 	min = -1; */
	    /* } */
	    /* if (max > 1) { */
	    /* 	overshoot = true; */
	    /* 	SDL_SetRenderDrawColor(main_win->rend, 255, 0, 0, 255); */
	    /* 	max = 1; */
	    /* } */
	    bool clipped = check_clip(&min, &max);
	    if (clipped) {
		SDL_SetRenderDrawColor(main_win->rend, 255, 0, 0, 255);
	    }
	    SDL_RenderDrawLine(main_win->rend, x, center_y - max * channel_h / 2, x, center_y - min * channel_h / 2);
	    start_in_clip_d += sfpp;
	    if (clipped) {
		SDL_SetRenderDrawColor(main_win->rend, sdl_colorp_expand(draw_color));
	    }
	}
    }
    pthread_mutex_unlock(&wd->lock);
}
