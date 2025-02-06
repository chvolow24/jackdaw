/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
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
    project.c

    * functions modifying project-related structures that do not belong elsewhere
    * timeline.c contains operations related to visualizing time onscreen
 *****************************************************************************************************************/

#include <errno.h>
#include <pthread.h>
#include <string.h>
#include <semaphore.h>
#include <fcntl.h>
#include "assets.h"
#include "audio_connection.h"
#include "color.h"
#include "components.h"
#include "dsp.h"
#include "endpoint.h"
#include "endpoint_callbacks.h"
#include "input.h"
#include "layout.h"
#include "layout_xml.h"
#include "menu.h"
#include "page.h"
#include "panel.h"
#include "project.h"
#include "status.h"
#include "text.h"
#include "textbox.h"
#include "timeline.h"
#include "transport.h"
#include "userfn.h"
#include "waveform.h"
#include "window.h"

#define DEFAULT_SFPP 600
#define CR_RECT_V_PAD (4 * main_win->dpi_scale_factor)
#define NUM_TRACK_COLORS 7

#define CLIPREF_NAMELABEL_H 20
#define CLIPREF_NAMELABEL_H_PAD 8
#define CLIPREF_NAMELABEL_V_PAD 2

#define TRACK_NAME_H_PAD 3
#define TRACK_NAME_V_PAD 1
#define TRACK_CTRL_SLIDER_H_PAD 7
#define TRACK_CTRL_SLIDER_V_PAD 5

#define STATUS_BAR_H (14 * main_win->dpi_scale_factor)
#define STATUS_BAR_H_PAD (10 * main_win->dpi_scale_factor);

#define SEM_NAME_UNPAUSE "/tl_%d_unpause_sem"
#define SEM_NAME_WRITABLE_CHUNKS "/tl_%d_writable_chunks"
#define SEM_NAME_READABLE_CHUNKS "/tl_%d_readable_chunks"

#define PLAYSPEED_ADJUST_SCALAR_LARGE 0.1f
#define PLAYSPEED_ADJUST_SCALAR_SMALL 0.015f

#define PLAYHEAD_ADJUST_SCALAR_LARGE 0.001f;
#define PLAYHEAD_ADJUST_SCALAR_SMALL 0.00015f

extern Window *main_win;
extern Project *proj;
extern SDL_Color color_global_black;
extern SDL_Color color_global_white;
extern SDL_Color color_global_clear;
extern SDL_Color color_global_red;
extern SDL_Color color_global_green;
extern SDL_Color color_global_light_grey;
extern SDL_Color color_global_quickref_button_blue;
extern SDL_Color color_global_grey;

extern Symbol *SYMBOL_TABLE[];

SDL_Color color_mute_solo_grey = {210, 210, 210, 255};

SDL_Color timeline_label_txt_color = {0, 200, 100, 255};

SDL_Color freq_L_color = {130, 255, 255, 255};
SDL_Color freq_R_color = {255, 255, 130, 220};


/* Alternating bright colors to easily distinguish tracks */
uint8_t track_color_index = 0;
SDL_Color track_colors[7] = {
    {5, 100, 115, 255},
    {38, 125, 240, 255},
    /* {70, 30, 130, 255}, */
    {100, 60, 130, 255},
    /* {171, 38, 38, 255}, */
    {171, 70, 70, 255},
    {224, 142, 74, 255},
    {250, 110, 100, 255},
    {60, 200, 150, 255}
};

uint8_t project_add_timeline(Project *proj, char *name)
{
    if (proj->num_timelines == MAX_PROJ_TIMELINES) {
	fprintf(stderr, "Error: project has max num timlines\n:");
	return proj->active_tl_index;
    }
    Timeline *new_tl = calloc(1, sizeof(Timeline));
    if (!new_tl) {
	fprintf(stderr, "Error: unable to allocate space for timeline.\n");
	exit(1);
    }
    if (strlen(name) > MAX_NAMELENGTH - 1) {
	fprintf(stderr, "Error: timeline name exceeds max len (%d)\n", MAX_NAMELENGTH);
	exit(1);
    }
    strcpy(new_tl->name, name);
    new_tl->proj = proj;
    new_tl->index = proj->num_timelines;
    Layout *tl_lt = layout_get_child_by_name_recursive(proj->layout, "timeline");
    if (new_tl->index != 0) {
	Layout *main_lt_fresh = layout_read_from_xml(MAIN_LT_PATH);
	Layout *new_tl_lt = layout_get_child_by_name_recursive(main_lt_fresh, "timeline");
	Layout *cpy = layout_copy(new_tl_lt, tl_lt->parent);
	layout_destroy(main_lt_fresh);
	/* Layout *cpy = layout_copy(tl_lt, tl_lt->parent); */
	new_tl->layout = cpy;
    } else {
	new_tl->layout = tl_lt;
    }
    layout_reset(new_tl->layout);
    new_tl->track_area = layout_get_child_by_name_recursive(new_tl->layout, "tracks_area");

    /* Clear tracks in old timeline layout before copying */
    for (int i=0; i<new_tl->track_area->num_children; i++) {
	layout_destroy_no_offset(new_tl->track_area->children[i]);
    }
    new_tl->track_area->num_children = 0;

    
    /* new_tl->layout = layout_get_child_by_name_recursive(proj->layout, "timeline"); */
    
    new_tl->sample_frames_per_pixel = DEFAULT_SFPP;
    strcpy(new_tl->timecode.str, "+00:00:00:00000");
    Layout *tc_lt = layout_get_child_by_name_recursive(new_tl->layout, "timecode");
    new_tl->timecode_tb = textbox_create_from_str(
	new_tl->timecode.str,
	tc_lt,
	main_win->mono_bold_font,
	16,
	main_win);
    textbox_set_background_color(new_tl->timecode_tb, &color_global_black);
    textbox_set_text_color(new_tl->timecode_tb, &color_global_white);
    textbox_set_trunc(new_tl->timecode_tb, false);

    Layout *ruler_lt = layout_get_child_by_name_recursive(new_tl->layout, "ruler");
    Layout *lemniscate_lt = layout_add_child(ruler_lt);
    lemniscate_lt->h.type = SCALE;
    lemniscate_lt->h.value = 1.0;
    lemniscate_lt->w.type = ABS;
    lemniscate_lt->w.value = 500.0;
    lemniscate_lt->x.type = ABS;
    lemniscate_lt->x.value = 0.0;
    layout_force_reset(lemniscate_lt);
    new_tl->loop_play_lemniscate = textbox_create_from_str(
	"∞",
	lemniscate_lt,
	main_win->std_font,
	24,
	main_win);
    textbox_set_background_color(new_tl->loop_play_lemniscate, &color_global_clear);
    textbox_set_text_color(new_tl->loop_play_lemniscate, &color_global_white);
    textbox_reset_full(new_tl->loop_play_lemniscate);
    
    new_tl->buf_L = calloc(1, sizeof(float) * proj->fourier_len_sframes * RING_BUF_LEN_FFT_CHUNKS);
    new_tl->buf_R = calloc(1, sizeof(float) * proj->fourier_len_sframes * RING_BUF_LEN_FFT_CHUNKS);
    new_tl->buf_write_pos = 0;
    new_tl->buf_read_pos = 0;
    char buf[128];
    snprintf(buf, 128, SEM_NAME_UNPAUSE, new_tl->index);
    bool retry = false;
retry1:
    if ((new_tl->unpause_sem = sem_open(buf, O_CREAT | O_EXCL, 0666, 0)) == SEM_FAILED) {
	if (errno != EEXIST) {
	    perror("Error opening unpause sem");
	}
	sem_unlink(buf);
	if (!retry) {
	    goto retry1;
	    retry = true;
	} else {
	    fprintf(stderr, "Fatal error: retry failed\n");
	    exit(1);
	}
	/* exit(1); */
	
    }
retry2:
    snprintf(buf, 128, SEM_NAME_READABLE_CHUNKS, new_tl->index);
    if ((new_tl->readable_chunks = sem_open(buf, O_CREAT | O_EXCL, 0666, 0)) == SEM_FAILED) {
	if (errno != EEXIST) {
	    perror("Error opening readable chunks sem");
	}
	sem_unlink(buf);
	if (!retry) {
	    goto retry2;
	    retry = true;
	} else {
	    fprintf(stderr, "Fatal error: retry failed\n");
	    exit(1);
	}

	/* exit(1); */
    }
retry3:
    snprintf(buf, 128, SEM_NAME_WRITABLE_CHUNKS, new_tl->index);
    int init_writable_chunks = proj->fourier_len_sframes * RING_BUF_LEN_FFT_CHUNKS / proj->chunk_size_sframes;
    if ((new_tl->writable_chunks = sem_open(buf, O_CREAT | O_EXCL, 0666, init_writable_chunks)) == SEM_FAILED) {
	if (errno != EEXIST) {
	    perror("Error opening writable chunks sem");
	}
	sem_unlink(buf);
	if (!retry) {
	    goto retry3;
	    retry = true;
	} else {
	    fprintf(stderr, "Fatal error: retry failed\n");
	    exit(1);
	}
	/* exit(1); */
    }
    new_tl->needs_redraw = true;
    proj->timelines[proj->num_timelines] = new_tl;
    proj->num_timelines++;

    api_node_register(&new_tl->api_node, &new_tl->proj->server.api_root, new_tl->name);
    
    return proj->num_timelines - 1; /* Return the new timeline index */
}

static void timeline_destroy(Timeline *tl, bool displace_in_proj)
{
    for (uint8_t i=0; i<tl->num_tracks; i++) {
	track_destroy(tl->tracks[i], false);
    }
    for (uint8_t i=0; i<tl->num_click_tracks; i++) {
	click_track_destroy(tl->click_tracks[i]);
    }
    /* tl->proj->num_timelines--; */
    if (displace_in_proj) {
	bool displace = false;
	for (uint8_t i=0; i<proj->num_timelines; i++) {
	    Timeline *test = proj->timelines[i];
	    if (test == tl) displace = true;
	    else if (displace && i > 0) {
		proj->timelines[i - 1] = proj->timelines[i];
	    }
	    proj->num_timelines--;
	}
    }
    if (tl->buf_L) free(tl->buf_L);
    if (tl->buf_R) free(tl->buf_R);

    if (tl->timecode_tb) textbox_destroy(tl->timecode_tb);

    if (sem_close(tl->unpause_sem) != 0) perror("Sem close");
    if (sem_close(tl->writable_chunks) != 0) perror("Sem close");
    if (sem_close(tl->readable_chunks) != 0) perror("Sem close");

    char buf[128];
    snprintf(buf, 128, SEM_NAME_UNPAUSE, tl->index);
    if (sem_unlink(buf) != 0 && errno != ENOENT) {
	perror("Error in sem unlink");
    }
    snprintf(buf, 128, SEM_NAME_WRITABLE_CHUNKS, tl->index);
    if (sem_unlink(buf) != 0 && errno != ENOENT) {
	perror("Error in sem unlink");
    }
    snprintf(buf, 128, SEM_NAME_READABLE_CHUNKS, tl->index);
    if (sem_unlink(buf) != 0 && errno != ENOENT) {
	perror("Error in sem unlink");
    }

    layout_destroy(tl->layout);
    free(tl);
}

static void clip_destroy_no_displace(Clip *clip);
void project_destroy(Project *proj)
{
    if (proj->recording) {
	transport_stop_recording();
    } else {
	transport_stop_playback();
    }

    /* api_quit(proj); */
    user_event_history_destroy(&proj->history);
    /* fprintf(stdout, "PROJECT_DESTROY num tracks: %d\n", proj->timelines[0]->num_tracks); */
    for (uint16_t i=0; i<proj->num_clips; i++) {
	clip_destroy_no_displace(proj->clips[i]);
    }

    for (uint8_t i=0; i<proj->num_timelines; i++) {
	timeline_destroy(proj->timelines[i], false);
    }

    for (uint8_t i=0; i<proj->num_record_conns; i++) {
	audioconn_destroy(proj->record_conns[i]);
    }
    for (uint8_t i=0; i<proj->num_playback_conns; i++) {
	audioconn_destroy(proj->playback_conns[i]);
    }

    free(proj->output_L);
    free(proj->output_R);
    free(proj->output_L_freq);
    free(proj->output_R_freq);

    textbox_destroy(proj->timeline_label);

    panel_area_destroy(proj->panels);
    
    if (proj->status_bar.call) textbox_destroy(proj->status_bar.call);
    if (proj->status_bar.dragstat) textbox_destroy(proj->status_bar.dragstat);
    if (proj->status_bar.error) textbox_destroy(proj->status_bar.error);

    project_destroy_animations(proj);
    project_destroy_metronomes(proj);

    free(proj);
}

void project_reset_tl_label(Project *proj)
{
    Timeline *tl = proj->timelines[proj->active_tl_index];
    snprintf(proj->timeline_label_str, PROJ_TL_LABEL_BUFLEN, "Timeline %d: \"%s\"\n", proj->active_tl_index + 1, tl->name);
    textbox_reset_full(proj->timeline_label);
}

static void project_init_panels(Project *proj, Layout *lt);
void project_init_audio_conns(Project *proj);
/* void project_init_quickref(Project *proj, Layout *control_bar_layout); */
Project *project_create(
    char *name,
    uint8_t channels,
    uint32_t sample_rate,
    SDL_AudioFormat fmt,
    uint16_t chunk_size_sframes,
    uint16_t fourier_len_sframes
    )
{
    Project *proj = calloc(1, sizeof(Project));
    if (!proj) {
	fprintf(stderr, "Error: unable to allocate space for project.\n");
	exit(1);
    }
    if (strlen(name) > MAX_NAMELENGTH - 1) {
	fprintf(stderr, "Error: project name exceeds max len (%d)\n", MAX_NAMELENGTH);
	exit(1);
    }
    
    window_set_layout(main_win, layout_create_from_window(main_win));
    layout_read_xml_to_lt(main_win->layout, MAIN_LT_PATH);
    
    strcpy(proj->name, name);
    char win_title_buf[MAX_NAMELENGTH];
    snprintf(win_title_buf, MAX_NAMELENGTH, "Jackdaw - %s", proj->name);
    
    SDL_SetWindowTitle(main_win->win, win_title_buf);

    proj->channels = channels;
    proj->sample_rate = sample_rate;
    proj->fmt = fmt;
    proj->chunk_size_sframes = chunk_size_sframes;
    proj->fourier_len_sframes = fourier_len_sframes;
    proj->layout = main_win->layout;
    proj->audio_rect = &(layout_get_child_by_name_recursive(proj->layout, "audio_rect")->rect);
    proj->console_column_rect = &(layout_get_child_by_name_recursive(proj->layout, "console_column")->rect);


    Layout *control_bar_layout = layout_get_child_by_name_recursive(proj->layout, "control_bar");

    /* project_init_quickref(proj, control_bar_layout); */
    proj->control_bar_rect = &control_bar_layout->rect;
    proj->ruler_rect = &(layout_get_child_by_name_recursive(proj->layout, "ruler")->rect);
    /* Layout *source_lt = layout_get_child_by_name_recursive(proj->layout, "source_area"); */
    /* proj->source_rect = &source_lt->rect; */
    /* proj->source_clip_rect = &(layout_get_child_by_name_recursive(source_lt, "source_clip")->rect); */
    /* Layout *src_name_lt = layout_get_child_by_name_recursive(source_lt, "source_clip_name"); */
    /* proj->source_name_tb = textbox_create_from_str( */
    /* 	"(no source clip)", */
    /* 	src_name_lt, */
    /* 	main_win->std_font, */
    /* 	14, */
    /* 	main_win */
    /* 	); */

    /* textbox_set_align(proj->source_name_tb, CENTER_LEFT); */
    /* textbox_set_background_color(proj->source_name_tb, &color_global_clear); */
    /* textbox_set_text_color(proj->source_name_tb, &color_global_white); */

    project_add_timeline(proj, "Main");
    Layout *timeline_label_lt = layout_get_child_by_name_recursive(proj->layout, "timeline_label");
    strcpy(proj->timeline_label_str, "Timeline 1: \"Main\"");
    proj->timeline_label = textbox_create_from_str(
	proj->timeline_label_str,
	timeline_label_lt,
	main_win->bold_font,
	12,
	main_win
	);
    textbox_set_text_color(proj->timeline_label, &timeline_label_txt_color);
    textbox_set_background_color(proj->timeline_label, &color_global_clear);
    textbox_set_align(proj->timeline_label, CENTER_LEFT);
    project_reset_tl_label(proj);
    /* Initialize output */
    /* proj->output_len = chunk_size_sframes; */
    proj->output_L = malloc(sizeof(float) * fourier_len_sframes);
    proj->output_R = malloc(sizeof(float) * fourier_len_sframes);
    proj->output_L_freq = malloc(sizeof(double) * fourier_len_sframes);
    proj->output_R_freq  = malloc(sizeof(double) * fourier_len_sframes);
    memset(proj->output_L, '\0', sizeof(float) * fourier_len_sframes);
    memset(proj->output_R, '\0', sizeof(float) * fourier_len_sframes);


    project_init_audio_conns(proj);
    project_init_metronomes(proj);

    Layout *panels_layout = layout_get_child_by_name_recursive(control_bar_layout, "panel_area");
    project_init_panels(proj, panels_layout);
    
    Layout *status_bar_lt = layout_get_child_by_name_recursive(proj->layout, "status_bar");
    Layout *draglt = layout_add_child(status_bar_lt);
    Layout *calllt = layout_add_child(status_bar_lt);
    Layout *errlt = layout_add_child(status_bar_lt);
    proj->status_bar.layout = status_bar_lt;

    draglt->h.type = SCALE;
    draglt->h.value = 1.0f;
    draglt->y.value = 0.0f;
    draglt->x.type = REL;
    draglt->x.value = STATUS_BAR_H_PAD;
    draglt->w.value = 0.0f;

    calllt->h.type = SCALE;
    calllt->h.value = 1.0f;
    calllt->y.value = 0.0f;
    calllt->x.type = STACK;
    calllt->x.value = STATUS_BAR_H_PAD;
    calllt->w.value = 500.0f;


    errlt->h.type = SCALE;
    errlt->h.value = 1.0f;
    errlt->y.value = 0.0f;
    errlt->x.type = REVREL;
    errlt->x.value = STATUS_BAR_H_PAD;
    errlt->w.value = 500.0f;

    /* strcpy(proj->status_bar.errstr, "Uh oh! this is an call!\n"); */
    /* strcpy(proj->status_bar.errorstr, "Ok error str\n"); */
    proj->status_bar.call = textbox_create_from_str(proj->status_bar.callstr, calllt, main_win->mono_bold_font, 14, main_win);
    textbox_set_trunc(proj->status_bar.call, false);
    textbox_set_text_color(proj->status_bar.call, &color_global_light_grey);
    textbox_set_background_color(proj->status_bar.call, &color_global_clear);
    textbox_set_align(proj->status_bar.call, CENTER_LEFT);

    proj->status_bar.dragstat = textbox_create_from_str(proj->status_bar.dragstr, draglt, main_win->mono_bold_font, 14, main_win);
    textbox_set_trunc(proj->status_bar.dragstat, false);
    textbox_set_text_color(proj->status_bar.dragstat, &color_global_green);
    textbox_set_background_color(proj->status_bar.dragstat, &color_global_clear);
    textbox_set_align(proj->status_bar.dragstat, CENTER_LEFT);
    
    proj->status_bar.error = textbox_create_from_str(proj->status_bar.errstr, errlt, main_win->mono_bold_font, 14, main_win);
    textbox_set_trunc(proj->status_bar.error, false);
    textbox_set_text_color(proj->status_bar.error, &color_global_red);
    textbox_set_background_color(proj->status_bar.error, &color_global_clear);
    textbox_set_align(proj->status_bar.error, CENTER_LEFT);
    /* textbox_size_to_fit(proj->status_bar.call, 0, 0); */
    /* textbox_size_to_fit(proj->status_bar.error, 0, 0); */
    textbox_reset_full(proj->status_bar.error);
    textbox_reset_full(proj->status_bar.dragstat);
    textbox_reset_full(proj->status_bar.call);

    Layout *hamburger_lt = layout_get_child_by_name_recursive(control_bar_layout, "hamburger");

    proj->hamburger = &hamburger_lt->rect;
    proj->bun_patty_bun[0] = &hamburger_lt->children[0]->children[0]->rect;
    proj->bun_patty_bun[1] = &hamburger_lt->children[1]->children[0]->rect;
    proj->bun_patty_bun[2] = &hamburger_lt->children[2]->children[0]->rect;


    
    int err;
    if ((err = pthread_mutex_init(&proj->animation_lock, NULL)) != 0) {
	fprintf(stderr, "Error initializing animation mutex: %s\n", strerror(err));
	exit(1);
    }

    if ((err = pthread_mutex_init(&proj->queued_val_changes_lock, NULL)) != 0) {
	fprintf(stderr, "Error initializing queued val changes mutex: %s\n", strerror(err));
	exit(1);
    }
    if ((err = pthread_mutex_init(&proj->queued_callback_lock, NULL)) != 0) {
	fprintf(stderr, "Error initializing queued callback mutex: %s\n", strerror(err));
	exit(1);
    }
    if ((err = pthread_mutex_init(&proj->ongoing_changes_lock, NULL)) != 0) {
	fprintf(stderr, "Error initializing ongoing changes mutex: %s\n", strerror(err));
	exit(1);
    }
    /* Initialize endpoints */
    endpoint_init(
	&proj->play_speed_ep,
	&proj->play_speed,
	JDAW_FLOAT,
	"play_speed",
	"undo/redo play speed adj",
	JDAW_THREAD_MAIN,
	play_speed_gui_cb, NULL, NULL,
	NULL, NULL, NULL, NULL);
    
    api_endpoint_register(&proj->play_speed_ep, &proj->server.api_root);			  
    api_start_server(proj, 9302);

    return proj;
}

static int quickref_add_track(void *self_v, void *target)
{
    user_tl_add_track(NULL);
    return 0;
}

static int quickref_record(void *self_v, void *target)
{
    user_tl_record(NULL);
    return 0;
}

static int quickref_left(void *self_v, void *target)
{
    user_tl_move_left(NULL);
    return 0;
}

static int quickref_right(void *self_v, void *target)
{
    user_tl_move_right(NULL);
    return 0;
}

static int quickref_rewind(void *self_v, void *target)
{
    user_tl_rewind(NULL);
    return 0;
}


static int quickref_pause(void *self_v, void *target)
{
    user_tl_pause(NULL);
    return 0;
}

static int quickref_play(void *self_v, void *target)
{
    user_tl_play(NULL);
    return 0;
}

static int quickref_next(void *self_v, void *target)
{
    user_tl_track_selector_down(NULL);
    return 0;
}

static int quickref_previous(void *self_v, void *target)
{
    user_tl_track_selector_up(NULL);
    return 0;
}

static int quickref_zoom_in(void *self_v, void *target)
{
    user_tl_zoom_in(NULL);
    return 0;
}

static int quickref_zoom_out(void *self_v, void *target)
{
    user_tl_zoom_out(NULL);
    return 0;
}

static int quickref_open_file(void *self_v, void *target)
{
    user_global_open_file(NULL);
    return 0;
}

static int quickref_save(void *self_v, void *target)
{
    user_global_save_project(NULL);
    return 0;
}

static int quickref_export_wav(void *self_v, void *target)
{
    user_tl_write_mixdown_to_wav(NULL);
    return 0;
}

static int quickref_track_settings(void *self_v, void *target)
{
    user_tl_track_open_settings(NULL);
    return 0;
}

static Layout *create_quickref_button_lt(Layout *row)
{
    Layout *ret = layout_add_child(row);
    ret->h.type = SCALE;
    ret->h.value = 0.8f;
    ret->x.type = STACK;
    ret->x.value = 10.0f;
    return ret;
}

typedef int (*ComponentFn)(void *self, void *target);

int set_output_compfn(void *self, void *target)
{
    project_set_default_out(proj);
    return 0;
}
static inline void project_init_output_panel(Page *output, Project *proj)
{

    PageElParams p;
    p.textbox_p.win = output->win;
    p.textbox_p.font = main_win->bold_font;
    p.textbox_p.text_size = 12;
    p.textbox_p.set_str = "Default Out:";

    PageEl *el = page_add_el(
	output,
	EL_TEXTBOX,
	p,
	"panel_out_label",
	"default_out_label");
    /* textbox_set_align(proj->tb_out_label, CENTER_LEFT); */
    /* textbox_set_background_color(proj->tb_out_label, &color_global_clear); */
    /* textbox_set_text_color(proj->tb_out_label, &color_global_white); */

    textbox_set_align((Textbox *)el->component, CENTER_LEFT);
    p.button_p.text_color = &color_global_white;
    p.button_p.background_color = &color_global_quickref_button_blue;
    p.button_p.text_size = 12;
    p.button_p.font = main_win->std_font;
    p.button_p.set_str = (char *)proj->playback_conn->name;
    p.button_p.win = output->win;
    p.button_p.target = NULL;
    p.button_p.action = set_output_compfn;
    el = page_add_el(
	output,
	EL_BUTTON,
	p,
	"panel_out_value",
	"default_out_value");
    textbox_set_trunc(((Button *)el->component)->tb, true);
    textbox_set_style(((Button *)el->component)->tb, BUTTON_DARK);
    
    void **output_L, **output_R;
    output_L = (void *)&(proj->output_L);
    output_R = (void *)&(proj->output_R);
    p.waveform_p.background_color = &color_global_black;
    p.waveform_p.plot_color = &color_global_white;
    p.waveform_p.num_channels = 1;
    p.waveform_p.len = proj->fourier_len_sframes;
    p.waveform_p.type = JDAW_FLOAT;
    p.waveform_p.channels = output_L;
    page_add_el(
	output,
	EL_WAVEFORM,
	p,
	"panel_out_wav_left",
	"out_waveform_left");

    p.waveform_p.channels = output_R;
    page_add_el(
	output,
	EL_WAVEFORM,
	p,
	"panel_out_wav_right",
	"out_waveform_right");
    
    
        /* textbox_size_to_fit(proj->tb_out_value, 2, 1); */
    /* textbox_set_fixed_w(proj->tb_out_value, saved_w - 10); */
    /* textbox_set_border(proj->tb_out_value, &color_global_black, 1); */

    
    /* proj->tb_out_value = textbox_create_from_str( */
    /* 	(char *)proj->playback_conn->name, */
    /* 	out_value_lt, */
    /* 	main_win->std_font, */
    /* 	12, */
    /* 	main_win); */
    /* textbox_set_align(proj->tb_out_value, CENTER_LEFT); */
    /* proj->tb_out_value->corner_radius = 6; */
    /* /\* textbox_set_align(proj->tb_out_value, CENTER); *\/ */
    /* int saved_w = proj->tb_out_value->layout->rect.w / main_win->dpi_scale_factor; */
    /* textbox_size_to_fit(proj->tb_out_value, 2, 1); */
    /* textbox_set_fixed_w(proj->tb_out_value, saved_w - 10); */
    /* textbox_set_border(proj->tb_out_value, &color_global_black, 1); */

    /* Layout *output_l_lt, *output_r_lt; */
    /* output_l_lt = layout_get_child_by_name_recursive(proj->layout, "out_waveform_left"); */
    /* output_r_lt = layout_get_child_by_name_recursive(proj->layout, "out_waveform_right"); */

    /* proj->outwav_l_rect = &output_l_lt->rect; */
    /* proj->outwav_r_rect = &output_r_lt->rect; */

}

static inline void project_init_quickref_panels(Page *quickref1, Page *quickref2)
{
    PageElParams p;
    /* Layout *quickref_lt = layout_read_from_xml(QUICKREF_LT_PATH); */
    /* layout_reparent(quickref_lt, quickref1->layout); */
    Layout *quickref_lt = quickref1->layout;
    Layout *row1 = quickref_lt->children[0];
    Layout *row2 = quickref_lt->children[1];
    Layout *row3 = quickref_lt->children[2];
    
    p.button_p.font = main_win->symbolic_font;
    p.button_p.background_color = &color_global_quickref_button_blue;
    p.button_p.text_color = &color_global_white;
    p.button_p.set_str = "C-t (add track)";
    p.button_p.action = quickref_add_track;
    p.button_p.target = NULL;
    p.button_p.text_size = 14;
    /* q->add_track = create_button_from_params(button_lt, b); */
    
    PageEl *el = page_add_el_custom_layout(
	quickref1,
	EL_BUTTON,
	p,
	"panel_quickref_add_track",
	row1,
	"add_track",
	create_quickref_button_lt);
    textbox_set_style(((Button *)el->component)->tb, BUTTON_DARK);
    /* textbox_set_trunc((Textbox *)((Button *)el->component)->tb, false); */

    p.button_p.action = quickref_record;
    p.button_p.set_str = "r ⏺";
    /* q->record = create_button_from_params(button_lt, b); */
    el = page_add_el_custom_layout(
	quickref1,
	EL_BUTTON,
	p,
	"panel_quickref_record",
	row1,
	"record",
	create_quickref_button_lt);
    textbox_set_style(((Button *)el->component)->tb, BUTTON_DARK);
    /* textbox_set_trunc((Textbox *)((Button *)el->component)->tb, false); */

    /* /\* ROW 2 *\/ */
    /* button_lt = create_quickref_button_lt(row2); */
    p.button_p.font = main_win->mono_font;
    p.button_p.action = quickref_left;
    p.button_p.set_str = "h ←";
    el = page_add_el_custom_layout(
	quickref1,
	EL_BUTTON,
	p,
	"panel_quickref_left",
	row2,
	"left",
	create_quickref_button_lt);
    textbox_set_style(((Button *)el->component)->tb, BUTTON_DARK);
    /* textbox_set_trunc((Textbox *)((Button *)el->component)->tb, false); */
    /* q->left = create_button_from_params(button_lt, b); */

    /* button_lt = create_quickref_button_lt(row2); */
    p.button_p.font = main_win->symbolic_font;
    p.button_p.action = quickref_rewind;
    p.button_p.set_str = "j ⏴";
    el = page_add_el_custom_layout(
	quickref1,
	EL_BUTTON,
	p,
	"panel_quickref_rewind",
	row2,
	"rewind",
	create_quickref_button_lt);
    textbox_set_style(((Button *)el->component)->tb, BUTTON_DARK);
    
    /* textbox_set_trunc((Textbox *)((Button *)el->component)->tb, false); */
    /* q->rewind = create_button_from_params(button_lt, b); */

    /* button_lt = create_quickref_button_lt(row2); */
    p.button_p.action = quickref_pause;
    p.button_p.set_str = "k ⏸";
    el = page_add_el_custom_layout(
	quickref1,
	EL_BUTTON,
	p,
	"panel_quickref_pause",
	row2,
	"pause",
	create_quickref_button_lt);
    textbox_set_style(((Button *)el->component)->tb, BUTTON_DARK);
    /* textbox_set_trunc((Textbox *)((Button *)el->component)->tb, false); */
    /* q->pause = create_button_from_params(button_lt, b); */

    /* button_lt = create_quickref_button_lt(row2); */
    p.button_p.action = quickref_play;
    p.button_p.set_str = "l ⏵";

    el = page_add_el_custom_layout(
	quickref1,
	EL_BUTTON,
	p,
	"panel_quickref_play",
	row2,
	"play",
	create_quickref_button_lt);
    textbox_set_style(((Button *)el->component)->tb, BUTTON_DARK);
    /* textbox_set_trunc((Textbox *)((Button *)el->component)->tb, false); */
    /* Q->play = create_button_from_params(button_lt, b); */

    /* button_lt = create_quickref_button_lt(row2); */
    p.button_p.font = main_win->mono_font;
    p.button_p.action = quickref_right;
    p.button_p.set_str = "; →";

    el = page_add_el_custom_layout(
	quickref1,
	EL_BUTTON,
	p,
	"panel_quickref_right",
	row2,
	"right",
	create_quickref_button_lt);
    textbox_set_style(((Button *)el->component)->tb, BUTTON_DARK);
    /* textbox_set_trunc((Textbox *)((Button *)el->component)->tb, false); */
    /* q->right = create_button_from_params(button_lt, b); */


    /* /\* ROW 3 *\/ */

    /* button_lt = create_quickref_button_lt(row3); */
    p.button_p.action = quickref_next;
    p.button_p.set_str = "n ↓";

    el = page_add_el_custom_layout(
	quickref1,
	EL_BUTTON,
	p,
	"panel_quickref_next",
	row3,
	"next",
	create_quickref_button_lt);
    textbox_set_style(((Button *)el->component)->tb, BUTTON_DARK);
    /* textbox_set_trunc((Textbox *)((Button *)el->component)->tb, false); */
    /* q->next = create_button_from_params(button_lt, b); */

    /* button_lt = create_quickref_button_lt(row3); */
    p.button_p.action = quickref_previous;
    p.button_p.set_str = "p ↑";

    el = page_add_el_custom_layout(
	quickref1,
	EL_BUTTON,
	p,
	"panel_quickref_previous",
	row3,
	"previous",
	create_quickref_button_lt);
    textbox_set_style(((Button *)el->component)->tb, BUTTON_DARK);
    /* textbox_set_trunc((Textbox *)((Button *)el->component)->tb, false); */
    /* q->previous = create_button_from_params(button_lt, b); */


    /* button_lt = create_quickref_button_lt(row3); */
    p.button_p.font = main_win->symbolic_font;
    p.button_p.action = quickref_zoom_out;
    p.button_p.set_str = ", 🔍-";
    el = page_add_el_custom_layout(
	quickref1,
	EL_BUTTON,
	p,
	"panel_quickref_zoom_out",
	row3,
	"zoom_out",
	create_quickref_button_lt);
    textbox_set_style(((Button *)el->component)->tb, BUTTON_DARK);
    /* textbox_set_trunc((Textbox *)((Button *)el->component)->tb, false); */
    /* q->zoom_out = create_button_from_params(button_lt, b); */

    /* button_lt = create_quickref_button_lt(row3); */
    p.button_p.action = quickref_zoom_in;
    p.button_p.set_str = ". 🔍+";
    el = page_add_el_custom_layout(
	quickref1,
	EL_BUTTON,
	p,
	"panel_quickref_zoom_in",
	row3,
	"zoom_in",
	create_quickref_button_lt);
    textbox_set_style(((Button *)el->component)->tb, BUTTON_DARK);
    /* textbox_set_trunc((Textbox *)((Button *)el->component)->tb, false); */

    
    /* q->zoom_in = create_button_from_params(button_lt, b); */

    page_reset(quickref1);
    layout_size_to_fit_children_h(row1, true, 0);
    layout_size_to_fit_children_h(row2, true, 0);
    layout_size_to_fit_children_h(row3, true, 0);

    /* COL2 */

    /* quickref_lt = layout_read_from_xml(QUICKREF_LT_PATH); */
    /* layout_reparent(quickref_lt, quickref2->layout); */
    quickref_lt = quickref2->layout;
    row1 = quickref_lt->children[0];
    row2 = quickref_lt->children[1];
    row3 = quickref_lt->children[2];
    
    /* p.button_p.font = main_win->symbolic_font; */
    /* p.button_p.background_color = &color_global_quickref_button_blue; */
    /* p.button_p.text_color = &color_global_white; */
    /* p.button_p.set_str = "C-t (add track)"; */
    /* p.button_p.action = quickref_add_track; */
    /* p.button_p.target = NULL; */
    /* p.button_p.text_size = 14; */


    /* button_lt = create_quickref_button_lt(row1); */
    /* /\* p.button_p.font = main_win->mono_bold_font; *\/ */
    p.button_p.action = quickref_open_file;
    p.button_p.text_size = 12;
    p.button_p.set_str = "Open file (C-o)";
    el = page_add_el_custom_layout(
	quickref2,
	EL_BUTTON,
	p,
	"panel_quickref_open_file",
	row1,
	"open_file",
	create_quickref_button_lt);
    textbox_set_style(((Button *)el->component)->tb, BUTTON_DARK);
    /* textbox_set_trunc((Textbox *)((Button *)el->component)->tb, false); */


    p.button_p.action = quickref_save;
    p.button_p.set_str = "Save (C-s)";
    el = page_add_el_custom_layout(
	quickref2,
	EL_BUTTON,
	p,
	"panel_quickref_save_file",
	row1,
	"save",
	create_quickref_button_lt);
    textbox_set_style(((Button *)el->component)->tb, BUTTON_DARK);
    /* textbox_set_trunc((Textbox *)((Button *)el->component)->tb, false); */


    p.button_p.action = quickref_export_wav;
    p.button_p.set_str = "Export wav (S-w)";
    
    el = page_add_el_custom_layout(
	quickref2,
	EL_BUTTON,
	p,
	"panel_quickref_export_wav",
	row2,
	"export_wav",
	create_quickref_button_lt);
    textbox_set_style(((Button *)el->component)->tb, BUTTON_DARK);
    /* textbox_set_trunc((Textbox *)((Button *)el->component)->tb, false); */

    p.button_p.action = quickref_track_settings;
    p.button_p.set_str = "Track settings (S-t)";

    el = page_add_el_custom_layout(
	quickref2,
	EL_BUTTON,
	p,
	"panel_quickref_track_settings",
	row3,
	"track_settings",
	create_quickref_button_lt);
    textbox_set_style(((Button *)el->component)->tb, BUTTON_DARK);
	/* textbox_set_trunc((Textbox *)((Button *)el->component)->tb, false); */
}

extern SDL_Color source_mode_bckgrnd;
extern SDL_Color clip_ref_home_bckgrnd;
extern SDL_Color timeline_marked_bckgrnd;
struct source_area_draw_arg {
    SDL_Rect *source_area_rect;
    SDL_Rect *source_clip_rect;
};

static void source_area_draw(void *arg1, void *arg2)
{
    struct source_area_draw_arg *arg = (struct source_area_draw_arg *)arg1;
    SDL_Rect *source_clip_rect = arg->source_clip_rect;
    /* SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(source_mode_bckgrnd)); */
    /* SDL_RenderFillRect(main_win->rend, source_rect); */
    /* SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(color_global_white)); */
    /* SDL_RenderDrawRect(main_win->rend, source_rect); */
    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(clip_ref_home_bckgrnd));
    SDL_RenderDrawRect(main_win->rend, source_clip_rect);
    if (proj->src_clip) {
	SDL_RenderFillRect(main_win->rend, source_clip_rect);

	/* Draw src clip waveform */
	SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(color_global_black));
	uint8_t num_channels = proj->src_clip->channels;
	float *channels[num_channels];
	channels[0] = proj->src_clip->L;
	if (num_channels > 1) {
	    channels[1] = proj->src_clip->R;
	}
	waveform_draw_all_channels(channels, num_channels, proj->src_clip->len_sframes, source_clip_rect);
	/* draw_waveform_to_rect(proj->src_clip->L, proj->src_clip->len_sframes, proj->source_clip_rect); */
	
	/* Draw play head line */
	SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(color_global_white));
	int ph_y = source_clip_rect->y;
	/* int tri_y = tl->proj->ruler_rect->y; */
	int play_head_x = source_clip_rect->x + source_clip_rect->w * proj->src_play_pos_sframes / proj->src_clip->len_sframes;
	SDL_RenderDrawLine(main_win->rend, play_head_x, ph_y, play_head_x, ph_y + source_clip_rect->h);

	/* /\* Draw play head triangle *\/ */
	int tri_x1 = play_head_x;
	int tri_x2 = play_head_x;
	/* int ph_y = tl->rect.y; */
	for (int i=0; i<PLAYHEAD_TRI_H; i++) {
	    SDL_RenderDrawLine(main_win->rend, tri_x1, ph_y, tri_x2, ph_y);
	    ph_y -= 1;
	    tri_x2 += 1;
	    tri_x1 -= 1;
	}

	/* draw mark in */
	int in_x, out_x = -1;

	in_x = source_clip_rect->x + source_clip_rect->w * proj->src_in_sframes / proj->src_clip->len_sframes;
	int i_tri_x2 = in_x;
	ph_y = source_clip_rect->y;
	for (int i=0; i<PLAYHEAD_TRI_H; i++) {
	    SDL_RenderDrawLine(main_win->rend, in_x, ph_y, i_tri_x2, ph_y);
	    ph_y -= 1;
	    i_tri_x2 += 1;
	}

	/* draw mark out */

	out_x = source_clip_rect->x + source_clip_rect->w * proj->src_out_sframes / proj->src_clip->len_sframes;
	int o_tri_x2 = out_x;
	ph_y = source_clip_rect->y;
	for (int i=0; i<PLAYHEAD_TRI_H; i++) {
	    SDL_RenderDrawLine(main_win->rend, out_x, ph_y, o_tri_x2, ph_y);
	    ph_y -= 1;
	    o_tri_x2 -= 1;
	}

	    
	if (in_x < out_x && out_x != 0) {
	    SDL_Rect in_out = (SDL_Rect) {in_x, source_clip_rect->y, out_x - in_x, source_clip_rect->h};
	    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(timeline_marked_bckgrnd));
	    SDL_RenderFillRect(main_win->rend, &(in_out));
	}
    }

}
static inline void project_init_source_area(Page *source_area, Project *proj)
{
    PageElParams p;

    // source_clip_name
    p.textbox_p.font = main_win->std_font;
    p.textbox_p.text_size = 14;
    p.textbox_p.win = main_win;
    p.textbox_p.set_str = "(no source clip)";

    PageEl *el = page_add_el(
	source_area,
	EL_TEXTBOX,
	p,
	"panel_source_clip_name_tb",
	"source_clip_name");
    Textbox *tb = (Textbox *)el->component;
    textbox_set_align(tb, CENTER_LEFT);
    textbox_set_background_color(tb, NULL);
    /* textbox_set_text_color(proj->source_name_tb,  */

    static struct source_area_draw_arg draw_arg;
    p.canvas_p.draw_fn = source_area_draw;
    p.canvas_p.draw_arg1 = &draw_arg;
    el = page_add_el(
	source_area,
	EL_CANVAS,
	p,
	"panel_source_canvas",
	"source_area");
    Layout *source_area_lt = el->layout;
    Layout *clip_lt = layout_get_child_by_name_recursive(source_area_lt, "source_clip");
    draw_arg.source_area_rect = &source_area_lt->rect;
    draw_arg.source_clip_rect = &clip_lt->rect;

}

static inline void project_init_output_spectrum(Page *output_spectrum, Project *proj_loc)
{
    double *arrays[] = {
	proj_loc->output_L_freq,
	proj_loc->output_R_freq
    };
    int steps[] = {1, 1};
    SDL_Color *plot_colors[] = {&freq_L_color, &freq_R_color, &color_global_white};
    PageElParams p;
    p.freqplot_p.arrays = arrays;
    p.freqplot_p.steps = steps;
    p.freqplot_p.num_arrays = 2;
    p.freqplot_p.num_items = proj_loc->fourier_len_sframes / 2;
    p.freqplot_p.colors = plot_colors;
    Project *saved_glob_proj = proj;
    proj = proj_loc; /* Not great */
    page_add_el(
	output_spectrum,
	EL_FREQ_PLOT,
	p,
	"panel_output_freqplot",
	"freqplot");
    proj=saved_glob_proj; /* Not great */
}

static void project_init_panels(Project *proj, Layout *lt)
{
    PanelArea *pa = panel_area_create(lt, main_win);
    proj->panels = pa;
    panel_area_add_panel(pa);
    panel_area_add_panel(pa);
    panel_area_add_panel(pa);
    panel_area_add_panel(pa);

    Page *output_panel = panel_area_add_page(
	pa,
	"Output",
	OUTPUT_PANEL_LT_PATH,
	NULL,
	&color_global_white,
	main_win);
    
    Page *quickref1 = panel_area_add_page(
	pa,
	"Quickref 1",
	QUICKREF_LT_PATH,
	NULL,
	&color_global_white,
	main_win);
    Page *quickref2 = panel_area_add_page(
	pa,
	"Quickref 2",
	QUICKREF_LT_PATH,
	NULL,
	&color_global_white,
	main_win);

    Page *source_area = panel_area_add_page(
	pa,
	"Sample source",
	SOURCE_AREA_LT_PATH,
	NULL,
	&color_global_white,
	main_win);

    Page *output_spectrum = panel_area_add_page(
	pa,
	"Output spectrum",
	OUTPUT_SPECTRUM_LT_PATH,
	NULL,
	&color_global_white,
	main_win);


    project_init_quickref_panels(quickref1, quickref2);
    project_init_output_panel(output_panel, proj);
    project_init_source_area(source_area, proj);
    project_init_output_spectrum(output_spectrum, proj);
    panel_select_page(pa, 0, 0);
    panel_select_page(pa, 1, 1);
    panel_select_page(pa, 2, 2);
    panel_select_page(pa, 3, 3);
}


int audioconn_open(Project *proj, AudioConn *dev);
void project_init_audio_conns(Project *proj)
{
    proj->num_playback_conns = query_audio_connections(proj, 0);
    proj->num_record_conns = query_audio_connections(proj, 1);
    proj->playback_conn = proj->playback_conns[0];
    if (proj->playback_conn->available && audioconn_open(proj, proj->playback_conn) != 0) {
	fprintf(stderr, "Error: failed to open default audio conn \"%s\". More info: %s\n", proj->playback_conn->name, SDL_GetError());
    }
}


/* static float amp_to_db(float amp) */
/* { */
/*     return (20.0f * log10(amp)); */
/* } */


/* /\* Type: `void (*)(char *, size_t, ValType, void *)`   *\/ */

/* static void slider_label_plain_str(char *dst, size_t dstsize, void *value, ValType type) */
/* { */
/*     double value_d = type == JDAW_DOUBLE ? *(double *)value : *(float *)value; */
/*     snprintf(dst, dstsize, "%f", value_d); */

/* } */


static NEW_EVENT_FN(undo_rename_object, "undo rename object")
    Text *txt = (Text *)obj1;
    if (txt->cached_value) {
        free(txt->cached_value);
    }
    txt->cached_value = strdup(txt->value_handle);
    txt_set_value(txt, (char *)obj2);
}

static NEW_EVENT_FN(redo_rename_object, "redo rename object")
    Text *txt = (Text *)obj1;
    txt_set_value(txt, txt->cached_value);
}

/* static NEW_EVENT_FN(undo_clipref_rename, "undo rename clipref") */
/*     Text *txt = (Text *)obj1; */
/*     if (txt->cached_value) { */
/*         free(txt->cached_value); */
/*     } */
/*     txt->cached_value = strdup(txt->value_handle); */
/*     txt_set_value(txt, (char *)obj2); */
/* } */

/* static NEW_EVENT_FN(redo_clipref_rename, "redo rename clipref") */
/*     Text *txt = (Text *)obj1; */
/*     txt_set_value(txt, txt->cached_value); */
/* } */
    

static int name_completion(Text *txt, void *obj)
{   
    Value nullval = {.int_v = 0};
    char *old_value = strdup(txt->cached_value);

    if (obj) {
	APINode *node = (APINode *)obj;
	api_node_renamed(node);
    }
    
    user_event_push(
	&proj->history,
	undo_rename_object,
	redo_rename_object,
	NULL, NULL,
	(void *)txt,
	old_value,
	nullval, nullval, nullval, nullval,
	0, 0, false, true);
    return 0;
}

/* static int clipref_name_completion(Text *txt) */
/* { */
/*     Value nullval = {.int_v = 0}; */
/*     char *old_value = strdup(txt->cached_value); */
/*     user_event_push( */
/* 	&proj->history, */
/* 	undo_rename_object, */
/* 	redo_rename_object, */
/* 	NULL, NULL, */
/* 	(void *)txt, */
/* 	old_value, */
/* 	nullval, nullval, nullval, nullval, */
/* 	0, 0, false, true); */
/*     return 0; */
/* } */


void timeline_rectify_track_area(Timeline *tl)
{
    layout_force_reset(tl->track_area);
    layout_size_to_fit_children_v(tl->track_area, true, 0);
}

void timeline_rectify_track_indices(Timeline *tl)
{
    if (tl->layout_selector >= tl->track_area->num_children) {
	tl->layout_selector = tl->track_area->num_children - 1;
    }
    if (tl->layout_selector < 0 && tl->track_area->num_children > 0) {
	tl->layout_selector = 0;
    }

    /* fprintf(stderr, "Rectify\n"); */
    int click_track_index = 0;
    int track_index = 0;
    
    Track *track_stack[tl->num_tracks];
    ClickTrack *click_track_stack[tl->num_click_tracks];
    
    for (int i=0; i<tl->track_area->num_children; i++) {
	Layout *lt = tl->track_area->children[i];
	bool is_selected_lt = i == tl->layout_selector;
	for (uint8_t j=0; j<tl->num_tracks; j++) {
	    Track *track = tl->tracks[j];
	    if (track->layout == lt) {
		track_stack[track_index] = track;
		track->tl_rank = track_index;
		if (is_selected_lt) {
		    tl->track_selector = track_index;
		    tl->click_track_selector = -1;
		}
		track_index++;
		break;
	    }
	}
	for (uint8_t j=0; j<tl->num_click_tracks; j++) {
	    ClickTrack *tt = tl->click_tracks[j];
	    if (tt->layout == lt) {
		click_track_stack[click_track_index] = tt;
		tt->index = click_track_index;
		if (is_selected_lt) {
		    tl->click_track_selector = click_track_index;
		    tl->track_selector = -1;
		}
		click_track_index++;
		break;
	    }
	}
    }
    /* fprintf(stderr, "->LT selector: %d\n", tl->layout_selector); */
    /* fprintf(stderr, "->track selector: %d\n", tl->track_selector); */
    /* fprintf(stderr, "->tt selector: %d\n", tl->click_track_selector); */
    memcpy(tl->tracks, track_stack, sizeof(Track *) * tl->num_tracks);
    memcpy(tl->click_tracks, click_track_stack, sizeof(ClickTrack *) * tl->num_click_tracks);
    tl->needs_redraw = true;
}

Track *timeline_selected_track(Timeline *tl)
{
    if (tl->num_tracks == 0 || tl->track_selector < 0) {
	return NULL;
    } else if (tl->track_selector > tl->num_tracks - 1) {
	fprintf(stderr, "ERROR: track selector points past end of tracks arr\n");
	exit(1);
    } else {
	return tl->tracks[tl->track_selector];
    }
}

ClickTrack *timeline_selected_click_track(Timeline *tl)
{
    if (tl->num_click_tracks == 0 || tl->click_track_selector < 0) {
	return NULL;
    } else if (tl->click_track_selector > tl->num_click_tracks - 1) {
	fprintf(stderr, "ERROR: tempo track selector points past end of tracks arr\n");
	exit(1);
    } else {
	return tl->click_tracks[tl->click_track_selector];
    }
}

Layout *timeline_selected_layout(Timeline *tl)
{
    if (tl->layout_selector >= 0 && tl->layout_selector < tl->track_area->num_children) {
	return tl->track_area->children[tl->layout_selector];
    }
    return NULL;
}

static void track_reset_full(Track *track);


static int auto_dropdown_action(void *self, void *xarg)
{
    Track *t = (Track *)xarg;
    if (t->num_automations == 0) {
	track_add_new_automation(t);
	return 0;
    }
    bool some_shown = false;
    for (uint8_t i=0; i<t->num_automations; i++) {
	if (t->automations[i]->shown) some_shown = true;
    }
    if (some_shown) {
	track_automations_hide_all(t);
    } else {
	track_automations_show_all(t);
    }
    return 0;
}

Track *timeline_add_track(Timeline *tl)
{
    Track *track = calloc(1, sizeof(Track));
    tl->tracks[tl->num_tracks] = track;
    track->tl_rank = tl->num_tracks++;
    track->tl = tl;
    snprintf(track->name, sizeof(track->name), "Track %d", track->tl_rank + 1);

    track->channels = tl->proj->channels;

    track->input = tl->proj->record_conns[0];
    track->color = track_colors[track_color_index];
    if (track_color_index < NUM_TRACK_COLORS -1) {
	track_color_index++;
    } else {
	track_color_index = 0;
    }

    /* API */

    api_node_register(&track->api_node, &track->tl->api_node, track->name);
    
    endpoint_init(
	&track->vol_ep,
	&track->vol,
	JDAW_FLOAT,
	"vol",
	"undo/redo adjust track volume",
	JDAW_THREAD_DSP,
	track_slider_cb,
	NULL, NULL,
	&track->vol_ctrl, track->tl,
	NULL, NULL);
    endpoint_set_allowed_range(
	&track->vol_ep,
	(Value){.float_v=0.0},
	(Value){.float_v=TRACK_VOL_MAX});


    endpoint_init(
	&track->pan_ep,
	&track->pan,
	JDAW_FLOAT,
	"pan",
	"undo/redo adj track pan",
	JDAW_THREAD_DSP,
	track_slider_cb,
	NULL, NULL,
	&track->pan_ctrl, track->tl,
	NULL, NULL);
    endpoint_set_allowed_range(
	&track->pan_ep,
	(Value){.float_v = 0.0},
	(Value){.float_v = 1.0});
    
    /* Layout *track_area = layout_get_child_by_name_recursive(tl->layout, "tracks_area"); */
    Layout *track_template = layout_read_xml_to_lt(tl->track_area, TRACK_LT_PATH);
    track->layout = track_template;
    
    track->inner_layout = track_template->children[0];
    timeline_rectify_track_area(tl);
    /* fprintf(stdout, "tracks area h: %d/%d\n", tl->track_area->h.value.intval, tl->track_area->rect.h); */
    Layout *name, *mute, *solo, *vol_label, *pan_label, *in_label, *in_value;

    Layout *track_name_row = layout_get_child_by_name_recursive(track->inner_layout, "name_value");
    name = layout_get_child_by_name_recursive(track_name_row, "name_value");
    mute = layout_get_child_by_name_recursive(track->inner_layout, "mute_button");
    solo = layout_get_child_by_name_recursive(track->inner_layout, "solo_button");
    vol_label = layout_get_child_by_name_recursive(track->inner_layout, "vol_label");
    pan_label = layout_get_child_by_name_recursive(track->inner_layout, "pan_label");
    in_label = layout_get_child_by_name_recursive(track->inner_layout, "in_label");
    in_value = layout_get_child_by_name_recursive(track->inner_layout, "in_value");

    /* Force layout reset before creating gui components */
    layout_force_reset(track->layout);

    
    track->tb_vol_label = textbox_create_from_str(
	"Vol:",
	vol_label,
	main_win->bold_font,
	12,
	main_win);
    textbox_set_background_color(track->tb_vol_label, &color_global_clear);
    textbox_set_align(track->tb_vol_label, CENTER_LEFT);
    textbox_set_pad(track->tb_vol_label, TRACK_NAME_H_PAD, 0);
    textbox_set_trunc(track->tb_vol_label, false);

    track->tb_pan_label = textbox_create_from_str(
	"Pan:",
	pan_label,
	main_win->bold_font,
	12,
	main_win);
    textbox_set_background_color(track->tb_pan_label, &color_global_clear);
    textbox_set_align(track->tb_pan_label, CENTER_LEFT);
    textbox_set_pad(track->tb_pan_label, TRACK_NAME_H_PAD, 0);
    textbox_set_trunc(track->tb_pan_label, false);

    track->tb_name = textentry_create(
	name,
	track->name,
	MAX_NAMELENGTH,
	main_win->bold_font,
	14,
	NULL,
	name_completion,
	&track->api_node,
	main_win);
    /* track->tb_name = textbox_create_from_str( */
    /*     track->name, */
    /* 	name, */
    /* 	main_win->bold_font, */
    /* 	14, */
    /* 	main_win); */
    Textbox *track_tb = track->tb_name->tb;

    textbox_set_align(track_tb, CENTER_LEFT);
    textbox_set_pad(track_tb, 4, 0);
    textbox_set_border(track_tb, &color_global_black, 1);
    textbox_set_trunc(track->tb_vol_label, false);
    /* textbox_reset_full(track->tb_name); */
    /* track->tb_name->text->validation = txt_name_validation; */
    /* track->tb_name->text->completion = name_completion; */

    
    track->tb_input_label = textbox_create_from_str(
	"In: ",
	in_label,
	main_win->bold_font,
	12,
	main_win);
    textbox_set_background_color(track->tb_input_label, &color_global_clear);
    textbox_set_align(track->tb_input_label, CENTER_LEFT);
    textbox_set_pad(track->tb_input_label, TRACK_NAME_H_PAD, 0);


    track->tb_input_name = textbox_create_from_str(
	(char *)track->input->name,
	in_value,
	main_win->std_font,
	12,
	main_win);
    track->tb_input_name->corner_radius = BUBBLE_CORNER_RADIUS;
    /* textbox_set_align(track->tb_input_name, CENTER_); */
    textbox_set_pad(track->tb_input_name, 4, 0);
    /* int saved_w = track->tb_input_name->layout->rect.w / main_win->dpi_scale_factor; */
    /* textbox_size_to_fit(track->tb_input_name, 10, 0); */
    /* textbox_set_trunc(track->tb_input_name, true); */
    /* textbox_set_fixed_w(track->tb_input_name, saved_w); */
    textbox_set_border(track->tb_input_name, &color_global_black, 1);
    textbox_set_style(track->tb_input_name, BUTTON_CLASSIC);

    
    track->tb_mute_button = textbox_create_from_str(
	"M",
	mute,
	main_win->bold_font,
	14,
	main_win);
    track->tb_mute_button->corner_radius = MUTE_SOLO_BUTTON_CORNER_RADIUS;
    textbox_set_border(track->tb_mute_button, &color_global_black, 1);
    textbox_set_background_color(track->tb_mute_button, &color_mute_solo_grey);
    textbox_set_style(track->tb_mute_button, BUTTON_CLASSIC);
    /* textbox_reset_full(track->tb_mute_button); */

    track->tb_solo_button = textbox_create_from_str(
	"S",
	solo,
	main_win->bold_font,
	14,
	main_win);
    track->tb_solo_button->corner_radius = MUTE_SOLO_BUTTON_CORNER_RADIUS;
    textbox_set_border(track->tb_solo_button, &color_global_black, 1);
    textbox_set_background_color(track->tb_solo_button, &color_mute_solo_grey);
    textbox_set_style(track->tb_solo_button, BUTTON_CLASSIC);
    /* textbox_reset_full(track->tb_solo_button); */


    Layout *vol_ctrl_row = layout_get_child_by_name_recursive(track->inner_layout, "vol_slider");
    Layout *vol_ctrl_lt = layout_add_child(vol_ctrl_row);

    layout_pad(vol_ctrl_lt, TRACK_CTRL_SLIDER_H_PAD, TRACK_CTRL_SLIDER_V_PAD);
    /* vol_ctrl_lt->x.value.intval = TRACK_CTRL_SLIDER_H_PAD; */
    /* vol_ctrl_lt->y.value.intval = TRACK_CTRL_SLIDER_V_PAD; */
    /* vol_ctrl_lt->w.value.intval = vol_ctrl_row->w.value.intval - TRACK_CTRL_SLIDER_H_PAD * 2; */
    /* vol_ctrl_lt->h.value.intval = vol_ctrl_row->h.value.intval - TRACK_CTRL_SLIDER_V_PAD * 2; */
    /* layout_set_values_from_rect(vol_ctrl_lt); */
    track->vol = 1.0f;

    track->vol_ctrl = slider_create(
	vol_ctrl_lt,
	&track->vol_ep,
	(Value){.float_v = 0.0f},
	(Value){.float_v = TRACK_VOL_MAX},
	SLIDER_HORIZONTAL,
	SLIDER_FILL,
	&label_amp_to_dbstr,
	&tl->proj->dragged_component);
    /* track->vol_ep.xarg1 = track->vol_ctrl; */
    /* track->vol_ctrl = slider_create( */
    /* 	vol_ctrl_lt, */
    /* 	(void *)(&track->vol), */
    /* 	JDAW_FLOAT, */
    /* 	SLIDER_HORIZONTAL, */
    /* 	SLIDER_FILL, */
    /* 	&slider_label_amp_to_dbstr, */
    /* 	NULL, */
    /* 	NULL, */
    /* 	&tl->proj->dragged_component); */
    /* SliderStrFn t; */
    /* Value min, max; */
    /* min.float_v = 0.0f; */
    /* max.float_v = TRACK_VOL_MAX; */
    /* slider_set_range(track->vol_ctrl, min, max); */

    Layout *pan_ctrl_row = layout_get_child_by_name_recursive(track->inner_layout, "pan_slider");
    Layout *pan_ctrl_lt = layout_add_child(pan_ctrl_row);

    layout_pad(pan_ctrl_lt, TRACK_CTRL_SLIDER_H_PAD, TRACK_CTRL_SLIDER_V_PAD);
    /* pan_ctrl_lt->x.value.intval = TRACK_CTRL_SLIDER_H_PAD; */
    /* pan_ctrl_lt->y.value.intval = TRACK_CTRL_SLIDER_V_PAD; */
    /* pan_ctrl_lt->w.value.intval = pan_ctrl_row->w.value.intval - TRACK_CTRL_SLIDER_H_PAD * 2; */
    /* pan_ctrl_lt->h.value.intval = pan_ctrl_row->h.value.intval - TRACK_CTRL_SLIDER_V_PAD * 2; */
    /* /\* layout_set_values_from_rect(pan_ctrl_lt); *\/ */
    track->pan = 0.5f;
    /* track->pan_ctrl = slider_create( */
    /* 	pan_ctrl_lt, */
    /* 	&track->pan, */
    /* 	JDAW_FLOAT, */
    /* 	SLIDER_HORIZONTAL, */
    /* 	SLIDER_TICK, */
    /* 	slider_label_pan, */
    /* 	NULL, */
    /* 	NULL, */
    /* 	&tl->proj->dragged_component); */
    track->pan_ctrl = slider_create(
	pan_ctrl_lt,
	&track->pan_ep,
	(Value){.float_v = 0.0f},
	(Value){.float_v = 1.0f},
	SLIDER_HORIZONTAL,
	SLIDER_TICK,
	label_pan,
	&tl->proj->dragged_component);
    track->pan_ctrl->disallow_unsafe_mode = true;
    /* track->pan_ep.xarg1 = track->pan_ctrl; */

    /* slider_reset(track->vol_ctrl); */
    /* slider_reset(track->pan_ctrl); */
    Layout *auto_dropdown_lt = layout_get_child_by_name_recursive(track->inner_layout, "automation_dropdown");
    track->automation_dropdown = symbol_button_create(
	auto_dropdown_lt,
	SYMBOL_TABLE[SYMBOL_DROPDOWN],
	auto_dropdown_action,
	(void *)track,
	NULL);
    track->automation_dropdown->background_color = &color_global_grey;
    auto_dropdown_lt->w.value = (float)track->automation_dropdown->symbol->x_dim_pix / main_win->dpi_scale_factor;
    auto_dropdown_lt->h.value = (float)track->automation_dropdown->symbol->y_dim_pix / main_win->dpi_scale_factor;
    auto_dropdown_lt->x.value += 4.0f;
    layout_reset(auto_dropdown_lt);
    
    track->selected_automation = -1;
    track->console_rect = &(layout_get_child_by_name_recursive(track->inner_layout, "track_console")->rect);
    track->colorbar = &(layout_get_child_by_name_recursive(track->inner_layout, "colorbar")->rect);


    track_reset_full(track);
    if (tl->layout_selector < 0) tl->layout_selector = 0;
    timeline_rectify_track_indices(tl);

    api_endpoint_register(&track->vol_ep, &track->api_node);
    api_endpoint_register(&track->pan_ep, &track->api_node);
    return track;
}


void project_clear_active_clips()
{
    proj->active_clip_index = proj->num_clips;
}

Clip *project_add_clip(AudioConn *conn, Track *target)
{
    if (proj->num_clips == MAX_PROJ_CLIPS) {
	return NULL;
    }
    Clip *clip = calloc(1, sizeof(Clip));

    if (conn) {
	clip->recorded_from = conn;
    }
    if (target) {
	/* fprintf(stdout, "\t->target? %p\n", clip->target); */
	clip->target = target;
    }
    
    if (!target && conn) {
	snprintf(clip->name, sizeof(clip->name), "%s_rec_%d", conn->name, proj->num_clips); /* TODO: Fix this */
    } else if (target) {
	snprintf(clip->name, sizeof(clip->name), "%s take %d", target->name, target->num_takes + 1);
	target->num_takes++;
    } else {
	snprintf(clip->name, sizeof(clip->name), "anonymous");
    }
    clip->channels = proj->channels;
    proj->clips[proj->num_clips] = clip;
    proj->num_clips++;
    return clip;
}

/* int32_t clip_ref_len(ClipRef *cr) */
/* { */
/*     if (cr->out_mark_sframes <= cr->in_mark_sframes) { */
/* 	return cr->clip->len_sframes; */
/*     } else { */
/* 	return cr->out_mark_sframes - cr->in_mark_sframes; */
/*     } */
/* } */


void clipref_reset(ClipRef *cr, bool rescaled)
{

    cr->layout->rect.x = timeline_get_draw_x(cr->track->tl, cr->pos_sframes);
    uint32_t cr_len = cr->in_mark_sframes >= cr->out_mark_sframes
	? cr->clip->len_sframes
	: cr->out_mark_sframes - cr->in_mark_sframes;
    cr->layout->rect.w = timeline_get_draw_w(cr->track->tl, cr_len);
    cr->layout->rect.y = cr->track->inner_layout->rect.y + CR_RECT_V_PAD;
    cr->layout->rect.h = cr->track->inner_layout->rect.h - 2 * CR_RECT_V_PAD;
    layout_set_values_from_rect(cr->layout);
    layout_reset(cr->layout);
    textbox_reset_full(cr->label);
    /* if (rescaled) { */
	cr->waveform_redraw = true;
    /* } */
    /* cr->rect.x = timeline_get_draw_x(cr->pos_sframes); */
    /* uint32_t cr_len = cr->in_mark_sframes >= cr->out_mark_sframes */
    /* 	? cr->clip->len_sframes */
    /* 	: cr->out_mark_sframes - cr->in_mark_sframes; */
    /* cr->rect.w = timeline_get_draw_w(cr_len); */
    /* cr->rect.y = cr->track->inner_layout->rect.y + CR_RECT_V_PAD; */
    /* cr->rect.h = cr->track->layout->rect.h - 2 * CR_RECT_V_PAD; */
}

static void clipref_remove_from_track(ClipRef *cr)
{
    bool displace = false;
    Track *track = cr->track;
    for (uint16_t i=0; i<track->num_clips; i++) {
	ClipRef *test = track->clips[i];
	if (test == cr) {
	    displace = true;
	} else if (displace && i > 0) {
	    track->clips[i - 1] = test;
	    
	}
    }
    if (displace) track->num_clips--; /* else not found! */
}

void clipref_displace(ClipRef *cr, int displace_by)
{
    Track *track = cr->track;
    Timeline *tl = track->tl;
    int new_rank = track->tl_rank + displace_by;
    if (new_rank >= 0 && new_rank < tl->num_tracks) {
	Track *new_track = tl->tracks[new_rank];
	clipref_remove_from_track(cr);
	new_track->clips[new_track->num_clips] = cr;
	new_track->num_clips++;
	cr->track = new_track;
	/* fprintf(stdout, "ADD CLIP TO TRACK %s, which has %d clips now\n", new_track->name, new_track->num_clips); */
	
    }
    timeline_reset(tl, false);
}

static void clipref_insert_on_track(ClipRef *cr, Track *target)
{
    target->clips[target->num_clips] = cr;
    target->num_clips++;
    cr->track = target;
}

void clipref_move_to_track(ClipRef *cr, Track *target)
{
    Track *prev_track = cr->track;
    Timeline *tl = prev_track->tl;
    clipref_remove_from_track(cr);
    clipref_insert_on_track(cr, target);
    timeline_reset(tl, false);
}

static void track_reset_full(Track *track)
{
    textbox_reset_full(track->tb_name->tb);
    textbox_reset_full(track->tb_input_label);
    textbox_reset_full(track->tb_mute_button);
    textbox_reset_full(track->tb_solo_button);
    textbox_reset_full(track->tb_vol_label);
    textbox_reset_full(track->tb_pan_label);
    textbox_reset_full(track->tb_input_label);
    textbox_reset_full(track->tb_input_name);
    for (uint16_t i=0; i<track->num_clips; i++) {
	clipref_reset(track->clips[i], true);
    }
    slider_reset(track->vol_ctrl);
    slider_reset(track->pan_ctrl);
}
    

static void track_reset(Track *track, bool rescaled)
{
    for (uint16_t i=0; i<track->num_clips; i++) {
	clipref_reset(track->clips[i], rescaled);
    }
    for (uint8_t i=0; i<track->num_automations; i++) {
	Automation *a = track->automations[i];
	/* automation_clear_cache(track->automations[i]); */
	/* a->current = NULL; */
	if (a->shown) {
	    automation_reset_keyframe_x(a);
	}
    }
}

ClipRef *track_create_clip_ref(Track *track, Clip *clip, int32_t record_from_sframes, bool home)
{

    if (track->num_clips == MAX_TRACK_CLIPS) {
	char errstr[MAX_STATUS_STRLEN];
	snprintf(errstr, MAX_STATUS_STRLEN, "Track cannot have more than %d clips", MAX_TRACK_CLIPS);
	status_set_errstr(errstr);
	return NULL;
    }
    if (clip->num_refs == MAX_CLIP_REFS) {
	char errstr[MAX_STATUS_STRLEN];
	snprintf(errstr, MAX_STATUS_STRLEN, "Audio clip cannot have more than %d references", MAX_TRACK_CLIPS);
	status_set_errstr(errstr);
	return NULL;
    }
    /* fprintf(stdout, "track %s create clipref\n", track->name); */
    ClipRef *cr = calloc(1, sizeof(ClipRef));
    cr->layout = layout_add_child(track->inner_layout);
    cr->layout->h.type = SCALE;
    cr->layout->h.value = 0.96;
    if (clip->num_refs > 0) {
	snprintf(cr->name, MAX_NAMELENGTH, "%s ref %d", clip->name, clip->num_refs);
    } else {
	snprintf(cr->name, MAX_NAMELENGTH, "%s", clip->name);
    }
    pthread_mutex_init(&cr->lock, NULL);
    /* cr->lock = SDL_CreateMutex(); */
    /* SDL_LockMutex(cr->lock); */
    pthread_mutex_lock(&cr->lock);
    track->clips[track->num_clips] = cr;
    track->num_clips++;
    cr->pos_sframes = record_from_sframes;
    cr->clip = clip;
    cr->track = track;
    cr->home = home;

    Layout *label_lt = layout_add_child(cr->layout);
    label_lt->x.value = CLIPREF_NAMELABEL_H_PAD;
    label_lt->y.value = CLIPREF_NAMELABEL_V_PAD;
    label_lt->w.type = SCALE;
    label_lt->w.value = 0.8f;
    label_lt->h.value = CLIPREF_NAMELABEL_H;
    cr->label = textbox_create_from_str(cr->name, label_lt, main_win->mono_bold_font, 12, main_win);
    cr->label->text->validation = txt_name_validation;
    cr->label->text->completion = name_completion;

    textbox_set_align(cr->label, CENTER_LEFT);
    textbox_set_background_color(cr->label, NULL);
    textbox_set_text_color(cr->label, &color_global_light_grey);
	/* textbox_size_to_fit(cr->label, CLIPREF_NAMELABEL_H_PAD, CLIPREF_NAMELABEL_V_PAD); */

    /* fprintf(stdout, "Clip num refs: %d\n", clip->num_refs); */
    clip->refs[clip->num_refs] = cr;
    clip->num_refs++;
    pthread_mutex_unlock(&cr->lock);
    return cr;
}

void timeline_reset_loop_play_lemniscate(Timeline *tl)
{
    if (tl->in_mark_sframes >= tl->out_mark_sframes) return;
    int in_x = timeline_get_draw_x(tl, tl->in_mark_sframes);
    int out_x = timeline_get_draw_x(tl, tl->out_mark_sframes);
    layout_reset(tl->loop_play_lemniscate->layout);

    tl->loop_play_lemniscate->layout->rect.x = in_x;
    tl->loop_play_lemniscate->layout->rect.w = out_x - in_x;
    layout_set_values_from_rect(tl->loop_play_lemniscate->layout);
    textbox_reset(tl->loop_play_lemniscate);
    tl->needs_redraw = true;
    
}

void timeline_reset_full(Timeline *tl)
{
    for (int i=0; i<tl->num_tracks; i++) {
	track_reset_full(tl->tracks[i]);
    }
    layout_reset(tl->layout);
    if (tl->proj->loop_play) {
	timeline_reset_loop_play_lemniscate(tl);
    }

    tl->needs_redraw = true;

}

void timeline_reset(Timeline *tl, bool rescaled)
{
    layout_reset(tl->layout);
    for (int i=0; i<tl->num_tracks; i++) {
	track_reset(tl->tracks[i], rescaled);
    }

    layout_reset(tl->layout);
    if (tl->proj->loop_play) {
	timeline_reset_loop_play_lemniscate(tl);
    }

    tl->needs_redraw = true;
}

void track_increment_vol(Track *track)
{
    track->vol += TRACK_VOL_STEP;
    if (track->vol > track->vol_ctrl->max.float_v) {
	track->vol = track->vol_ctrl->max.float_v;
    }
    /* slider_edit_made(track->vol_ctrl); */
    slider_reset(track->vol_ctrl);
}
void track_decrement_vol(Track *track)
{
    track->vol -= TRACK_VOL_STEP;
    if (track->vol < 0.0f) {
	track->vol = 0.0f;
    }
    /* slider_edit_made(track->vol_ctrl); */
    slider_reset(track->vol_ctrl);
}

void track_increment_pan(Track *track)
{
    track->pan += TRACK_PAN_STEP;
    if (track->pan > 1.0f) {
	track->pan = 1.0f;
    }
    /* slider_edit_made(track->pan_ctrl); */
    slider_reset(track->pan_ctrl);
}

void track_decrement_pan(Track *track)
{
    track->pan -= TRACK_PAN_STEP;
    if (track->pan < 0.0f) {
	track->pan = 0.0f;
    }
    /* slider_edit_made(track->pan_ctrl); */
    slider_reset(track->pan_ctrl);
}

/* SDL_Color mute_red = {255, 0, 0, 100}; */
/* SDL_Color solo_yellow = {255, 200, 0, 130}; */

/* SDL_Color mute_red = {255, 0, 0, 130}; */
/* SDL_Color solo_yellow = {255, 200, 0, 130}; */

SDL_Color mute_red = {240, 80, 80, 255};
SDL_Color solo_yellow = {255, 200, 50, 255};

extern SDL_Color textbox_default_bckgrnd_clr;

bool track_mute(Track *track)
{
    track->muted = !track->muted;
    if (track->muted) {
	textbox_set_background_color(track->tb_mute_button, &mute_red);
    } else {
	textbox_set_background_color(track->tb_mute_button, &color_mute_solo_grey);
    }
    track->tl->needs_redraw = true;
    return track->muted;
}

bool track_solo(Track *track)
{
    track->solo = !track->solo;
    if (track->solo) {
	if (track->muted) {
	    track->muted = false;
	    textbox_set_background_color(track->tb_mute_button, &color_mute_solo_grey);
	}
	track->solo_muted = false;
	textbox_set_background_color(track->tb_solo_button, &solo_yellow);
    } else {
	textbox_set_background_color(track->tb_solo_button, &color_mute_solo_grey);

    }
    track->tl->needs_redraw = true;
    return track->solo;
}

void track_solomute(Track *track)
{
    track->solo_muted = true;
    textbox_set_background_color(track->tb_solo_button, &mute_red);
}
void track_unsolomute(Track *track)
{
    track->solo_muted = false;
    textbox_set_background_color(track->tb_solo_button, &color_mute_solo_grey);
}



static void rectify_solomute(Timeline *tl, int solo_count)
{
    Track *track;
    if (solo_count > 0) {
	for (uint8_t i=0; i<tl->num_tracks; i++) {
	    track = tl->tracks[i];
	    if (!track->solo) {
		track_solomute(track);
	    }
	}
    } else {
	for (uint8_t i=0; i<tl->num_tracks; i++) {
	    track = tl->tracks[i];
	    if (!track->solo) {
		track_unsolomute(track);
	    }
	}
    }
}


static NEW_EVENT_FN(undo_redo_tracks_mute, "undo/redo mute track")
    Track **tracks = (Track **)obj1;
    uint8_t num_tracks = val1.uint8_v;
    for (uint8_t i=0; i<num_tracks; i++) {
	track_mute(tracks[i]);
    }
}

static NEW_EVENT_FN(undo_redo_tracks_solo, "undo/redo solo track")
    Track **tracks_to_solo = (Track **)obj1;
    Timeline *tl = (Timeline *)obj2;
    uint8_t num_tracks_to_solo = val1.uint8_v;
    uint8_t solo_count = val2.uint8_v;
    bool end_state_solo = false;
    for (uint8_t i=0; i<num_tracks_to_solo; i++) {
	end_state_solo = track_solo(tracks_to_solo[i]);
    }
    if (end_state_solo) {
	rectify_solomute(tl, 1);
    } else if (solo_count > num_tracks_to_solo) {
	rectify_solomute(tl, 1);
    } else {
	rectify_solomute(tl, 0);
    }
}



void track_or_tracks_solo(Timeline *tl, Track *opt_track)
{

    Track *tracks_to_solo[MAX_TRACKS];
    uint8_t num_tracks_to_solo = 0;

    
    if (tl->num_tracks == 0) return;
    bool has_active_track = false;
    bool all_solo = true;
    int solo_count = 0;
    Track *track;
    for (uint8_t i=0; i<tl->num_tracks; i++) {
	track = tl->tracks[i];
	if (track->solo) {
	    solo_count++;
	}
	if (track->active) {
	    has_active_track = true;
	    if (!opt_track && !track->solo) {
		has_active_track = true;
		all_solo = false;
		track_solo(track);
		solo_count++;
		tracks_to_solo[num_tracks_to_solo] = track;
		num_tracks_to_solo++;
	    }
	}
    }
    if (!has_active_track || opt_track) {
	track = opt_track ? opt_track : timeline_selected_track(tl);
	if (!track) {
	    status_set_errstr("No track selected to solo");
	    return;
	}

	if (track_solo(track)) {
	    all_solo = false;
	    solo_count++;
	} else {
	    solo_count--;
	}
	tracks_to_solo[num_tracks_to_solo] = track;
	num_tracks_to_solo++;
    } else if (all_solo) {
	for (uint8_t i=0; i<tl->num_tracks; i++) {
	    track = tl->tracks[i];
	    if (track->active) {
		track_solo(track); /* unsolo */
		solo_count--;
		tracks_to_solo[num_tracks_to_solo] = track;
		num_tracks_to_solo++;
		
	    }
	}
    }
    rectify_solomute(tl, solo_count);

    if (num_tracks_to_solo > 0) {
	Track **undo_packet = calloc(num_tracks_to_solo, sizeof(Track *));
	memcpy(undo_packet, tracks_to_solo, num_tracks_to_solo * sizeof(Track *));
	Value num = {.uint8_v = num_tracks_to_solo};
	Value solocount = {.uint8_v = solo_count};
	user_event_push(
	    &proj->history,
	    undo_redo_tracks_solo,
	    undo_redo_tracks_solo,
	    NULL,
	    NULL,
	    (void *)undo_packet,
	    (void *)tl,
	    num,
	    solocount,
	    num,
	    solocount,
	    0,
	    0,
	    true,
	    false);
    }
	    

    tl->needs_redraw = true;
}



void track_or_tracks_mute(Timeline *tl)
{
    Track *muted_tracks[MAX_TRACKS];
    uint8_t num_muted = 0;
    
    /* if (tl->num_tracks == 0) return; */
    bool has_active_track = false;
    bool all_muted = true;
    Track *track;
    for (uint8_t i=0; i<tl->num_tracks; i++) {
	track = tl->tracks[i];
	if (track->active) {
	    has_active_track = true;
	    if (!track->muted) {
		has_active_track = true;
		all_muted = false;
		track_mute(track);
		muted_tracks[num_muted] = track;
		num_muted++;
	    }
	}
    }
    if (!has_active_track) {
	track = timeline_selected_track(tl);
	if (!track) {
	    ClickTrack *tt = timeline_selected_click_track(tl);
	    if (tt) {
		click_track_mute_unmute(tt);
	    }
	    return;
	}
	track_mute(track);
	muted_tracks[num_muted] = track;
	num_muted++;
    } else if (all_muted) {
	num_muted = 0;
	for (uint8_t i=0; i<tl->num_tracks; i++) {
	    track = tl->tracks[i];
	    if (track->active) {
		track_mute(track); /* unmute */
		muted_tracks[num_muted] = track;
		num_muted++;
	    }
	}
    }
    tl->needs_redraw = true;

    if (num_muted > 0) {
	Track **undo_packet = calloc(num_muted, sizeof(Track *));
	memcpy(undo_packet, muted_tracks, num_muted * sizeof(Track *));
	Value num = {.uint8_v = num_muted};
	user_event_push(
	    &proj->history,
	    undo_redo_tracks_mute,
	    undo_redo_tracks_mute,
	    NULL,
	    NULL,
	    (void *)undo_packet,
	    NULL,
	    num,
	    num,
	    num,
	    num,
	    0,
	    0,
	    true,
	    false);
    }
    
}

struct track_in_arg {
    Track *track;
    AudioConn *conn;
};
static void track_set_in_onclick(void *void_arg)
{
    /* struct select_dev_onclick_arg *carg = (struct select_dev_onclick_arg *)arg; */
    /* Track *track = carg->track; */
    /* AudioConn *dev = carg->new_in; */
    struct track_in_arg *arg = (struct track_in_arg *)void_arg;
    arg->track->input = arg->conn;
    textbox_set_value_handle(arg->track->tb_input_name, arg->track->input->name);

    window_pop_menu(main_win);
    Timeline *tl = proj->timelines[proj->active_tl_index];
    tl->needs_redraw = true;
    /* window_pop_mode(main_win); */
}

void track_set_input(Track *track)
{
    SDL_Rect *rect = &(track->tb_input_name->layout->rect);
    int y = rect->y;
    Menu *menu = menu_create_at_point(rect->x, rect->y);
    MenuColumn *c = menu_column_add(menu, "");
    MenuSection *sc = menu_section_add(c, "");
    
    for (int i=0; i<proj->num_record_conns; i++) {
	struct track_in_arg *arg = malloc(sizeof(struct track_in_arg));
	arg->track = track;
	arg->conn = proj->record_conns[i];
	if (arg->conn->available) {
	    MenuItem *item = menu_item_add(
		sc,
		arg->conn->name,
		" ",
		track_set_in_onclick,
		arg);
	    item->free_target_on_destroy = true;
	}
    }
    menu_add_header(menu,"", "Select audio input for track.\n\n'n' to select next item; 'p' to select previous item.");
    window_add_menu(main_win, menu);
    int move_by_y = 0;
    if ((move_by_y = y + menu->layout->rect.h - main_win->layout->rect.h) > 0) {
	menu_translate(menu, 0, -1 * move_by_y / main_win->dpi_scale_factor);
    }
}

void project_draw();
void track_rename(Track *track)
{
    textentry_edit(track->tb_name);
    /* txt_edit(track->tb_name->text, project_draw); */
    main_win->i_state = 0;
}

void clipref_rename(ClipRef *cr)
{
    txt_edit(cr->label->text, project_draw);
    main_win->i_state = 0;
}


void clipref_destroy(ClipRef *cr, bool displace_in_clip);
void clipref_destroy_no_displace(ClipRef *cr);
void clip_destroy(Clip *clip);


void clipref_ungrab(ClipRef *cr);
static void timeline_remove_track(Track *track)
{
    Timeline *tl = track->tl;
    for (uint16_t i=0; i<track->num_clips; i++) {
	ClipRef *clip = track->clips[i];
	if (clip->grabbed) {
	    clipref_ungrab(clip);
	}
    }
    if (proj->dragging) status_stat_drag();
    for (uint8_t i=track->tl_rank + 1; i<tl->num_tracks; i++) {
	Track *t = tl->tracks[i];
	tl->tracks[i-1] = t;
	t->tl_rank--;
    }
    layout_remove_child(track->layout);
    tl->num_tracks--;
    if (tl->num_tracks > 0 && tl->track_selector > tl->num_tracks - 1) tl->track_selector = tl->num_tracks - 1;
    timeline_rectify_track_area(tl);
    timeline_rectify_track_indices(tl);
    /* layout_size_to_fit_children_v(tl->track_area, true, 0); */
}

static void timeline_reinsert_track(Track *track)
{
    Timeline *tl = track->tl;
    for (int i=tl->num_tracks; i>track->tl_rank; i--) {
	tl->tracks[i] = tl->tracks[i-1];
    }
    tl->tracks[track->tl_rank] = track;
    tl->num_tracks++;
    layout_insert_child_at(track->layout, tl->track_area, track->layout->index);
    tl->layout_selector = track->layout->index;
    timeline_rectify_track_indices(tl);
    timeline_rectify_track_area(tl);
}

/* static void timeline_insert_track_at(Track *track, uint8_t index) */
/* { */
/*     Timeline *tl = track->tl; */
/*     /\* for (int i=0; i<tl->num_tracks; i++) { *\/ */
/*     /\* 	fprintf(stdout, "\t\t%d: Track index %d named \"%s\"\n", i, tl->tracks[i]->tl_rank, tl->tracks[i]->name); *\/ */
/*     /\* } *\/ */
/*     while (index > tl->num_tracks) index--; */
/*     /\* fprintf(stdout, "->adj ind: %d\n", index); *\/ */
/*     for (uint8_t i=tl->num_tracks; i>index; i--) { */
/* 	/\* fprintf(stdout, "\tmoving track %d->%d\n", i-1, i); *\/ */
/* 	tl->tracks[i] = tl->tracks[i - 1]; */
/* 	tl->tracks[i]->tl_rank = i; */
/*     } */
/*     layout_insert_child_at(track->layout, tl->track_area, index); */
/*     tl->tracks[index] = track; */
/*     track->tl_rank = index; */
/*     tl->num_tracks++; */
/*     timeline_rectify_track_indices(tl); */
/* } */


void track_delete(Track *track)
{
    track->deleted = true;
    Timeline *tl = track->tl;
    timeline_remove_track(track);
    timeline_reset(tl, false);
}
void track_undelete(Track *track)
{
    track->deleted = false;
    /* timeline_insert_track_at(track, track->tl_rank); */
    timeline_reinsert_track(track);
    timeline_reset(track->tl, false);
}
void track_destroy(Track *track, bool displace)
{
    /* Clip *clips_to_destroy[MAX_PROJ_CLIPS]; */
    /* uint8_t num_clips_to_destroy = 0; */
    for (uint16_t i=0; i<track->num_clips; i++) {
	ClipRef *cr = track->clips[i];
	if (cr) {
	    /* if (cr->home) { */
	    /* 	clips_to_destroy[num_clips_to_destroy] = cr->clip; */
	    /* 	num_clips_to_destroy++; */
	    /* } */
	    clipref_destroy_no_displace(track->clips[i]);
	}
    }
    /* fprintf(stdout, "OK deleted all crs, now clips\n"); */
    /* while (num_clips_to_destroy > 0) { */
    /* 	/\* fprintf(stdout, "Deleting clip\n"); *\/ */
    /* 	clip_destroy(clips_to_destroy[num_clips_to_destroy - 1]); */
    /* 	num_clips_to_destroy--; */
    /* } */
    /* fprintf(stdout, "Ok deleted all clips\n"); */
    slider_destroy(track->vol_ctrl);
    slider_destroy(track->pan_ctrl);
    textentry_destroy(track->tb_name);
    /* textbox_destroy(track->tb_name); */
    textbox_destroy(track->tb_input_label);
    textbox_destroy(track->tb_input_name);
    textbox_destroy(track->tb_vol_label);
    textbox_destroy(track->tb_pan_label);
    textbox_destroy(track->tb_mute_button);
    textbox_destroy(track->tb_solo_button);
    if (displace) {
	Timeline *tl = track->tl;
	for (uint8_t i=track->tl_rank + 1; i<tl->num_tracks; i++) {
	    Track *t = tl->tracks[i];
	    tl->tracks[i-1] = t;
	    t->tl_rank--;
	    /* t->layout = t->layout->parent->iterator->iterations[t->tl_rank]; */
	}
	layout_destroy(track->layout);
	/* layout_remove_iter_at(track->layout->parent->iterator, track->tl_rank); */
	tl->tracks[tl->num_tracks - 1] = NULL;
	/* track->layout->parent->iterator->num_iterations--; */
	tl->num_tracks--;
	timeline_reset(tl, false);
	/* timeline_reset(tl); */
	/* layout_reset(tl->layout); */
    }
    filter_deinit(&track->fir_filter);
    if (track->delay_line.buf_L) free(track->delay_line.buf_L);
    if (track->delay_line.buf_R) free(track->delay_line.buf_R);
    if (track->delay_line.cpy_buf) free(track->delay_line.cpy_buf);
    /* if (track->delay_line.lock) SDL_DestroyMutex(track->delay_line.lock); */
    pthread_mutex_destroy(&track->delay_line.lock);
    if (track->buf_L_freq_mag) free(track->buf_L_freq_mag);
    if (track->buf_R_freq_mag) free(track->buf_R_freq_mag);
    if (track->automation_dropdown) symbol_button_destroy(track->automation_dropdown);
    free(track);
}

void clipref_delete(ClipRef *cr)
{
    if (cr->grabbed) {
	clipref_ungrab(cr);
    }
    cr->track->tl->needs_redraw = true;
    cr->deleted = true;
    clipref_remove_from_track(cr);
}

void clipref_undelete(ClipRef *cr)
{
    cr->track->tl->needs_redraw = true;
    cr->deleted = false;
    clipref_insert_on_track(cr, cr->track);
}

static void clipref_check_and_remove_from_clipboard(ClipRef *cr)
{
    /* fprintf(stderr, "handling destruction CHECK AND REMOVE\n"); */
    Timeline *tl = cr->track->tl;
    bool displace = false;
    for (int i=0; i<tl->num_clips_in_clipboard; i++) {
	if (tl->clipboard[i] == cr) {
	    displace = true;
	    /* fprintf(stderr, "Found %p==%p at index %d\n", cr, tl->clipboard[i], i); */
	} else if (displace) {
	    /* fprintf(stderr, "\tMoving index %d->%d\n", i, i-1); */
	    tl->clipboard[i-1] = tl->clipboard[i];
	}
    }
    if (displace) {
	tl->num_clips_in_clipboard--;
	/* fprintf(stderr, "removed!\n"); */
    }
}

void clipref_destroy(ClipRef *cr, bool displace_in_clip)
{
    Track *track = cr->track;
    Clip *clip = cr->clip;

    clipref_check_and_remove_from_clipboard(cr);

    bool displace = false;
    for (uint16_t i=0; i<track->num_clips; i++) {
	if (track->clips[i] == cr) {
	    displace = true;
	} else if (displace && i>0) {
	    track->clips[i-1] = track->clips[i];
	}
    }
    if (displace) {
	track->num_clips--; /* else not found! */
    }
    /* fprintf(stdout, "\t->Track %d now has %d clips\n", track->tl_rank, track->num_clips); */

    if (displace_in_clip) {
	displace = false;
	for (uint16_t i=0; i<clip->num_refs; i++) {
	    if (cr == clip->refs[i]) displace = true;
	    else if (displace) {
		clip->refs[i-1] = clip->refs[i];
	    }
	}
	clip->num_refs--;
	/* fprintf(stdout, "\t->Clip at %p now has %d refs\n", clip, clip->num_refs); */
    }

    /* TODO: keep clips around */
    if (clip->num_refs == 0) {
	clip_destroy(clip);
    }
    textbox_destroy(cr->label);
    /* SDL_DestroyMutex(cr->lock); */
    pthread_mutex_destroy(&cr->lock);
    if (cr->waveform_texture)
	SDL_DestroyTexture(cr->waveform_texture);
    free(cr);
}
void clipref_destroy_no_displace(ClipRef *cr)
{
    clipref_check_and_remove_from_clipboard(cr);
    /* fprintf(stdout, "Clipref destroy no displace %s\n", cr->name); */
    bool displace = false;
    for (uint16_t i=0; i<cr->clip->num_refs; i++) {
	ClipRef *test = cr->clip->refs[i];
	if (test == cr) displace = true;
	else if (displace) {
	    cr->clip->refs[i-1] = cr->clip->refs[i];
	}
    }
    cr->clip->num_refs--;
    /* TODO: keep clips around */
    if (cr->clip->num_refs == 0) {
	clip_destroy(cr->clip);
    }
    pthread_mutex_destroy(&cr->lock);
    /* SDL_DestroyMutex(cr->lock); */
    textbox_destroy(cr->label);
    if (cr->waveform_texture)
	SDL_DestroyTexture(cr->waveform_texture);
    free(cr);
}

static void clip_destroy_no_displace(Clip *clip)
{
    for (uint16_t i=0; i<clip->num_refs; i++) {
	ClipRef *cr = clip->refs[i];
	clipref_destroy(cr, false);
    }
    if (clip == proj->src_clip) {
	proj->src_clip = NULL;
    }
    
    if (clip->L) free(clip->L);
    if (clip->R) free(clip->R);

    for (uint8_t i=0; i<proj->num_dropped; i++) {
	Clip **dropped_clip = &(proj->saved_drops[i].clip);
	if (*dropped_clip == clip) {
	    *dropped_clip = NULL;
	}
    }
    free(clip);
}

void clip_destroy(Clip *clip)
{
    /* fprintf(stdout, "CLIP DESTROY %s, num refs: %d\n", clip->name,  clip->num_refs); */
    /* fprintf(stdout, "DESTROYING CLIP %p, num: %d\n", clip, proj->num_clips); */
    for (uint16_t i=0; i<clip->num_refs; i++) {
	/* fprintf(stdout, "About to destroy clipref\n"); */
	ClipRef *cr = clip->refs[i];
	clipref_destroy(cr, false);
    }
    if (clip == proj->src_clip) {
	proj->src_clip = NULL;
    }
    bool displace = false;
    /* int num_displaced = 0; */
    for (uint16_t i=0; i<proj->num_clips; i++) {
	if (proj->clips[i] == clip) {
	    /* fprintf(stdout, "\tFOUND clip at pos %d\n", i); */
	    displace=true;
	} else if (displace && i > 0) {
	    /* fprintf(stdout, "\tmoving clip %p from pos %d to %d\n", proj->clips[i], i, i-1); */
	    proj->clips[i-1] = proj->clips[i];
	    /* num_displaced++; */
	}
    }
    /* fprintf(stdout, "\t->num displaced: %d\n", num_displaced); */
    proj->num_clips--;
    proj->active_clip_index = proj->num_clips;
    if (clip->L) free(clip->L);
    if (clip->R) free(clip->R);

    for (uint8_t i=0; i<proj->num_dropped; i++) {
	Clip **dropped_clip = &(proj->saved_drops[i].clip);
	if (*dropped_clip == clip) {
	    *dropped_clip = NULL;
	}
    }

    free(clip);
}


static void timeline_reinsert(Timeline *tl)
{
    /* Timeline *currently_active = proj->timelines[proj->active_tl_index]; */
    for (uint8_t i=proj->num_timelines; i>tl->index; i--) {
	proj->timelines[i] = proj->timelines[i-1];
	proj->timelines[i]->index++;
    }
    proj->timelines[tl->index] = tl;
    proj->num_timelines++;
    timeline_switch(tl->index);
    /* currently_active->layout->hidden = true; */
    /* tl->layout->hidden = false; */
    /* proj->active_tl_index = tl->index; */
    /* tl->needs_redraw = true; */
    /* project_reset_tl_label(tl->proj); */
}

NEW_EVENT_FN(undo_delete_timeline, "Undo delete timeline")
    Timeline *tl = (Timeline *)obj1;
    timeline_reinsert(tl);
}

NEW_EVENT_FN(redo_delete_timeline, "Redo delete timeline")
    Timeline *tl = (Timeline *)obj1;
    timeline_delete(tl, true);
}

NEW_EVENT_FN(dispose_delete_timeline, "")
    Timeline *tl = (Timeline *)obj1;
    timeline_destroy(tl, false);
}

void timeline_switch(uint8_t new_tl_index)
{
    Timeline *current = proj->timelines[proj->active_tl_index];
    current->layout->hidden = true;

    proj->active_tl_index = new_tl_index;
    Timeline *new = proj->timelines[new_tl_index];
    new->layout->hidden = false;
    
    /* Concession to bad design */
    proj->audio_rect = &(layout_get_child_by_name_recursive(new->layout, "audio_rect")->rect);
    proj->ruler_rect = &(layout_get_child_by_name_recursive(new->layout, "ruler")->rect);
    
    new->needs_redraw = true;
    project_reset_tl_label(new->proj);
}

void timeline_delete(Timeline *tl, bool from_undo)
{
    if (tl->proj->num_timelines == 1) {
	status_set_errstr("Cannot delete sole timeline on project.");
	return;
    }
    for (uint8_t i=tl->index; i<proj->num_timelines - 1; i++) {
	proj->timelines[i] = proj->timelines[i+1];
	proj->timelines[i]->index--;
    }
    proj->num_timelines--;
    if (proj->active_tl_index > proj->num_timelines - 1) {
	proj->active_tl_index--;
    }
    timeline_switch(proj->active_tl_index);
    /* Timeline *active = proj->timelines[proj->active_tl_index]; */
    /* active->layout->hidden = false; */
    /* project_reset_tl_label(proj); */
    /* active->needs_redraw = true; */
    if (!from_undo) {
	Value nullval = {.int_v = 0.0};
        user_event_push(
	    &proj->history,
	    undo_delete_timeline,
	    redo_delete_timeline,
	    dispose_delete_timeline,
	    NULL,
	    (void *)tl,
	    NULL,
	    nullval, nullval, nullval, nullval,
	    0, 0, false, false);
    }

}


void timeline_push_grabbed_clip_move_event(Timeline *tl);
void timeline_cache_grabbed_clip_offsets(Timeline *tl)
{
    for (uint8_t i=0; i<tl->num_grabbed_clips; i++) {
	tl->grabbed_clip_pos_cache[i].track_offset = tl->grabbed_clips[i]->track->tl_rank - tl->track_selector;
    }
}

void timeline_cache_grabbed_clip_positions(Timeline *tl)
{
    if (!tl->grabbed_clip_cache_initialized) {
	tl->grabbed_clip_cache_initialized = true;
    } else if (!tl->grabbed_clip_cache_pushed) {
	timeline_push_grabbed_clip_move_event(tl);
    }
    for (uint8_t i=0; i<tl->num_grabbed_clips; i++) {
	tl->grabbed_clip_pos_cache[i].track = tl->grabbed_clips[i]->track;
	tl->grabbed_clip_pos_cache[i].pos = tl->grabbed_clips[i]->pos_sframes;
	/* tl->grabbed_clip_pos_cache[i].track_offset = tl->grabbed_clips[i]->track->tl_rank - tl->track_selector; */
    }
    tl->grabbed_clip_cache_pushed = false;
}

static NEW_EVENT_FN(undo_move_clips, "undo move clips")
    ClipRef **cliprefs = (ClipRef **)obj1;
    struct track_and_pos *positions = (struct track_and_pos *)obj2;
    uint8_t num = val1.uint8_v;
    if (num == 0) return;
    for (uint8_t i = 0; i<num; i++) {
	if (!positions[i].track) break;
	clipref_move_to_track(cliprefs[i], positions[i].track);
	cliprefs[i]->pos_sframes = positions[i].pos;
	clipref_reset(cliprefs[i], false);
    }
    Timeline *tl = cliprefs[0]->track->tl;
    tl->needs_redraw = true;
}

static NEW_EVENT_FN(redo_move_clips, "redo move clips")
    ClipRef **cliprefs = (ClipRef **)obj1;
    struct track_and_pos *positions = (struct track_and_pos *)obj2;
    uint8_t num = val1.uint8_v;
    if (num == 0) return;
    for (uint8_t i = 0; i<num; i++) {
	if (!positions[i].track) break;
	clipref_move_to_track(cliprefs[i], positions[i + num].track);
	cliprefs[i]->pos_sframes = positions[i + num].pos;
	clipref_reset(cliprefs[i], false);
    }
    Timeline *tl = cliprefs[0]->track->tl;
    tl->needs_redraw = true;
}
void timeline_push_grabbed_clip_move_event(Timeline *tl)
{
    /* fprintf(stdout, "PUSH\n"); */
    ClipRef **cliprefs = calloc(tl->num_grabbed_clips, sizeof(ClipRef *));
    /* int32_t *positions = calloc(tl->num_grabbed_clips * 2, sizeof(ClipRef *)); */

    struct track_and_pos {
	Track *track;
	int32_t pos;
    };
    struct track_and_pos *positions = calloc(tl->num_grabbed_clips * 2, sizeof(struct track_and_pos));

    /* Undo positions in first half of array; redo in second half */
    for (uint8_t i=0; i<tl->num_grabbed_clips; i++) {
	clipref_reset(tl->grabbed_clips[i], false);
	cliprefs[i] = tl->grabbed_clips[i];
	/* undo pos */
	positions[i].track = tl->grabbed_clip_pos_cache[i].track;
	positions[i].pos = tl->grabbed_clip_pos_cache[i].pos;
	/* redo pos */
	positions[i + tl->num_grabbed_clips].track = cliprefs[i]->track;
	positions[i + tl->num_grabbed_clips].pos = cliprefs[i]->pos_sframes;
    }
    Value num = {.uint8_v = tl->num_grabbed_clips};

    user_event_push(
	&proj->history,
	undo_move_clips,
	redo_move_clips,
	NULL,
	NULL,
	(void *)cliprefs,
	(void *)positions,
	num,num,num,num,
	0,0,true,true);
    tl->grabbed_clip_cache_pushed = true;
}

/* Deprecated; replaced by timeline_delete_grabbed_cliprefs */
void timeline_destroy_grabbed_cliprefs(Timeline *tl)
{
    /* Clip *clips_to_destroy[255]; */
    /* uint8_t num_clips_to_destroy = 0; */
    /* fprintf(stdout, "Num grabbed clips to destroy: %d\n", tl->num_grabbed_clips); */
    while (tl->num_grabbed_clips > 0) {
	ClipRef *cr = tl->grabbed_clips[--tl->num_grabbed_clips];
	/* if (cr->home) { */
	/*     clips_to_destroy[num_clips_to_destroy] = cr->clip; */
	/*     num_clips_to_destroy++; */
	/* } */
	clipref_destroy(cr, true);
    }
    /* while (num_clips_to_destroy > 0) { */
    /* 	clip_destroy(clips_to_destroy[--num_clips_to_destroy]);	      */
    /* } */
    /* fprintf(stdout, "Deleted all grabbed cliprefs and clips\n"); */
}

static NEW_EVENT_FN(undo_delete_clips, "undo delete clips")
    ClipRef **clips = (ClipRef **)obj1;
    uint8_t num = val1.uint8_v;
    for (uint8_t i=0; i<num; i++) {
	clipref_undelete(clips[i]);
    }
}
static NEW_EVENT_FN(redo_delete_clips, "redo delete clips")
    ClipRef **clips = (ClipRef **)obj1;
    uint8_t num = val1.uint8_v;
    for (uint8_t i=0; i<num; i++) {
	clipref_delete(clips[i]);
    }
}
static NEW_EVENT_FN(dispose_delete_clips, "")
    ClipRef **clips = (ClipRef **)obj1;
    uint8_t num = val1.uint8_v;
    for (uint8_t i=0; i<num; i++) {
	clipref_destroy_no_displace(clips[i]);
    }
}

void timeline_delete_grabbed_cliprefs(Timeline *tl)
{
    ClipRef **deleted_cliprefs = calloc(tl->num_grabbed_clips, sizeof(ClipRef *));
    for (uint8_t i=0; i<tl->num_grabbed_clips; i++) {
	ClipRef *cr = tl->grabbed_clips[i];
	cr->grabbed = false;
	clipref_delete(cr);
	deleted_cliprefs[i] = cr;
    }

    Value num = {.uint8_v = tl->num_grabbed_clips};
    user_event_push(
	&proj->history,
	undo_delete_clips,
	redo_delete_clips,
	dispose_delete_clips,
	NULL,
	(void *)deleted_cliprefs,
	NULL,
	num,
	num,
	num,
	num,
	0, 0, true, false);
    tl->num_grabbed_clips = 0;
}


int32_t clipref_len(ClipRef *cr)
{
    if (cr->out_mark_sframes < cr->in_mark_sframes) {
	fprintf(stderr, "ERROR: clipref has negative length\n");
	breakfn();
	return 0;
    /* } else if (cr->out_mark_sframes == cr->in_mark_sframes) { */
    /* 	return cr->clip->len_sframes; */
    } else {
	return cr->out_mark_sframes - cr->in_mark_sframes;
    }
}

ClipRef *clipref_at_cursor_in_track(Track *track)
{
    for (int i=track->num_clips-1; i>=0; i--) {
	ClipRef *cr = track->clips[i];
	if (cr->pos_sframes <= track->tl->play_pos_sframes && cr->pos_sframes + clipref_len(cr) >= track->tl->play_pos_sframes) {
	    return cr;
	}
    }
    return NULL;
}

void clipref_bring_to_front()
{
    Timeline *tl = proj->timelines[proj->active_tl_index];
    Track *track = timeline_selected_track(tl);
    if (!track) return;
    /* bool displace = false; */
    ClipRef *to_move = NULL;
    for (int i=0; i<track->num_clips; i++) {
	ClipRef *cr = track->clips[i];
	if (!to_move && cr->pos_sframes <= tl->play_pos_sframes && cr->pos_sframes + clipref_len(cr) >= tl->play_pos_sframes) {
	    to_move = cr;
	} else if (to_move) {
	    track->clips[i-1] = track->clips[i];
	    if (i == track->num_clips - 1) {
		track->clips[i] = to_move;
	    }
	}
    }
    tl->needs_redraw = true;
}

bool clipref_marked(Timeline *tl, ClipRef *cr)
{
    if (tl->in_mark_sframes >= tl->out_mark_sframes) return false;
    int32_t cr_end = cr->pos_sframes + clipref_len(cr);
    if (cr_end >= tl->in_mark_sframes && cr->pos_sframes <= tl->out_mark_sframes) return true;
    return false;
}

ClipRef *clipref_at_cursor()
{
    Timeline *tl = proj->timelines[proj->active_tl_index];
    Track *track = timeline_selected_track(tl);
    if (!track) return NULL;
    
    /* Reverse iter to ensure top-most clip is returned in case of overlap */
    for (int i=track->num_clips -1; i>=0; i--) {
	ClipRef *cr = track->clips[i];
	if (cr->pos_sframes <= tl->play_pos_sframes && cr->pos_sframes + clipref_len(cr) >= tl->play_pos_sframes) {
	    return cr;
	}
    }
    return NULL;
}

ClipRef *clipref_before_cursor(int32_t *pos_dst)
{
    Timeline *tl = proj->timelines[proj->active_tl_index];
    Track *track = timeline_selected_track(tl);
    if (!track) return NULL;
    if (track->num_clips == 0) return NULL;
    ClipRef *ret = NULL;
    int32_t end = INT32_MIN;
    for (int i=0; i<track->num_clips; i++) {
	ClipRef *cr = track->clips[i];
	int32_t cr_end = cr->pos_sframes + clipref_len(cr);
	if (cr_end < tl->play_pos_sframes && cr_end >= end) {
	    ret = cr;
	    end = cr_end;
	}
    }
    *pos_dst = end;
    return ret;
}

ClipRef *clipref_after_cursor(int32_t *pos_dst)
{
    Timeline *tl = proj->timelines[proj->active_tl_index];
    Track *track = timeline_selected_track(tl);
    if (!track) return NULL;
    if (track->num_clips == 0) return NULL;
    ClipRef *ret = NULL;
    int32_t start = INT32_MAX;
    for (int i=0; i<track->num_clips; i++) {
	ClipRef *cr = track->clips[i];
	int32_t cr_start = cr->pos_sframes;
	if (cr_start > tl->play_pos_sframes && cr_start <= start) {
	    start = cr_start;
	    *pos_dst = cr_start;
	    ret = cr;
	}
    }
    return ret;
}

void clipref_grab(ClipRef *cr)
{
    Timeline *tl = cr->track->tl;
    if (tl->num_grabbed_clips == MAX_GRABBED_CLIPS) {
	char errstr[MAX_STATUS_STRLEN];
	snprintf(errstr, MAX_STATUS_STRLEN, "Cannot grab >%d clips", MAX_GRABBED_CLIPS);
	status_set_errstr(errstr);
	return;
    }

    tl->grabbed_clips[tl->num_grabbed_clips] = cr;
    tl->num_grabbed_clips++;
    cr->grabbed = true;
}

void clipref_ungrab(ClipRef *cr)
{
    Timeline *tl = cr->track->tl;
    bool displace = false;
    for (uint8_t i=0; i<tl->num_grabbed_clips; i++) {
	if (displace) {
	    tl->grabbed_clips[i - 1] = tl->grabbed_clips[i];
	} else if (cr == tl->grabbed_clips[i]) {
	    displace = true;
	}
    }
    cr->grabbed = false;
    tl->num_grabbed_clips--;
    status_stat_drag();
}

void timeline_ungrab_all_cliprefs(Timeline *tl)
{
    for (uint8_t i=0; i<tl->num_grabbed_clips; i++) {
	tl->grabbed_clips[i]->grabbed = false;
    }
    tl->num_grabbed_clips = 0;
}


static NEW_EVENT_FN(undo_cut_clipref, "undo cut clip")
    ClipRef *cr1 = (ClipRef *)obj1;
    ClipRef *cr2 = (ClipRef *)obj2;
    cr1->out_mark_sframes = val2.int32_v;
    clipref_reset(cr1, true);
    clipref_delete(cr2);
}

static NEW_EVENT_FN(redo_cut_clipref, "redo cut clip")
    ClipRef *cr1 = (ClipRef *)obj1;
    ClipRef *cr2 = (ClipRef *)obj2;
    clipref_undelete(cr2);
    cr1->out_mark_sframes = val1.int32_v;
    clipref_reset(cr1, true);
    /* clipref_reset(cr2); */
}

static NEW_EVENT_FN(cut_clip_dispose_forward, "")
    clipref_destroy_no_displace((ClipRef *)obj2);
}

    
static ClipRef *clipref_cut(ClipRef *cr, int32_t cut_pos_rel)
{
    ClipRef *new = track_create_clip_ref(cr->track, cr->clip, cr->pos_sframes + cut_pos_rel, false);
    if (!new) {
	return NULL;
    }
    if (cut_pos_rel < 0) return NULL;
    new->in_mark_sframes = cr->in_mark_sframes + cut_pos_rel;
    new->out_mark_sframes = cr->out_mark_sframes == 0 ? clipref_len(cr) : cr->out_mark_sframes;
    Value orig_end_pos = {.int32_v = cr->out_mark_sframes};
    cr->out_mark_sframes = cr->out_mark_sframes == 0 ? cut_pos_rel : cr->out_mark_sframes - (clipref_len(cr) - cut_pos_rel);
    track_reset(cr->track, true);

    Value cut_pos = {.int32_v = cr->out_mark_sframes};
    user_event_push(
	&proj->history,
	undo_cut_clipref,
	redo_cut_clipref,
	NULL,
	cut_clip_dispose_forward,
	cr, new,
	cut_pos, orig_end_pos, cut_pos, orig_end_pos,
	0, 0, false, false);

    return new;
}



void timeline_cut_at_cursor(Timeline *tl)
{
    Track *track = timeline_selected_track(tl);
    if (track) {
	status_cat_callstr(" clipref at cursor");
	ClipRef *cr = clipref_at_cursor();
	if (!cr) {
	    status_set_errstr("Error: no clip at cursor");
	    return;
	}
	if (tl->play_pos_sframes > cr->pos_sframes && tl->play_pos_sframes < cr->pos_sframes + clipref_len(cr)) {
	    clipref_cut(cr, tl->play_pos_sframes - cr->pos_sframes);
	}
    } else {
	timeline_cut_click_track_at_cursor(tl);
	status_cat_callstr(" tempo track at cursor");

    }
}


/* static NEW_EVENT_FN(undo_redo_move_track, "undo/redo move track") */
/*     Track *track = (Track *)obj1; */
/*     int direction = val1.int_v; */
/*     timeline_move_track(track->tl, track, direction, true); */
/* } */

/* void timeline_move_track(Timeline *tl, Track *track, int direction, bool from_undo) */
/* { */
/*     int old_pos = track->tl_rank; */
/*     int new_pos = old_pos + direction; */
/*     if (new_pos < 0 || new_pos >= tl->num_tracks) return; */

/*     Track *displaced = tl->tracks[new_pos]; */
/*     tl->tracks[new_pos] = track; */
/*     tl->tracks[old_pos] = displaced; */
/*     displaced->tl_rank = track->tl_rank; */

/*     /\* Layout *displaced_layout = displaced->layout; *\/ */
/*     /\* Layout *track_layout = track->layout; *\/ */

/*     /\* displaced_layout->parent->children[displaced_layout->index] = track_layout; *\/ */
/*     /\* displaced_layout->parent->children[track_layout->index] = displaced_layout; *\/ */
/*     /\* int saved_i = displaced_layout->index; *\/ */
/*     /\* displaced_layout->index = track_layout->index; *\/ */
/*     /\* track_layout->index = saved_i; *\/ */
/*     layout_swap_children(displaced->layout, track->layout); */
    
/*     track->tl_rank = new_pos; */
/*     tl->track_selector = track->tl_rank; */
/*     timeline_reset(tl, false); */

/*     if (!from_undo) { */
/* 	Value direction_forward = {.int_v = direction}; */
/* 	Value direction_reverse = {.int_v = direction * -1}; */
/* 	user_event_push( */
/* 	    &proj->history, */
/* 	    undo_redo_move_track, */
/* 	    undo_redo_move_track, */
/* 	    NULL, */
/* 	    NULL, */
/* 	    (void *)track, */
/* 	    NULL, */
/* 	    direction_reverse, */
/* 	    direction_reverse, */
/* 	    direction_forward, */
/* 	    direction_forward, */
/* 	    0, 0, false, false); */
/*     } */
/* } */

static void track_move_automation(Automation *a, int direction, bool from_undo);
NEW_EVENT_FN(undo_redo_move_automation, "undo/redo move automation")
    Automation *a = (Automation *)obj1;
    int direction = val1.int_v;
    track_move_automation(a, direction, true);
}

static void track_move_automation(Automation *a, int direction, bool from_undo)
{
    TEST_FN_CALL(automation_index, a);
    Track *track = a->track;
    track->tl->needs_redraw = true;
    TEST_FN_CALL(track_automation_order, track);
    int new_pos = a->index + direction;
    if (new_pos >= 0 && new_pos < track->num_automations) {
	Automation *to_swap = track->automations[new_pos];
	uint8_t swap_index = to_swap->index;
	uint8_t orig_index = a->index;
	to_swap->index = a->index;
	a->index = swap_index;
	layout_swap_children(a->layout, to_swap->layout);
	track->automations[orig_index] = to_swap;
	track->automations[new_pos] = a;
	track->selected_automation = new_pos;
	TEST_FN_CALL(track_automation_order, track);
	layout_reset(track->layout);
	if (!from_undo) {
	    Value undo_dir = {.int_v = -1 * direction};
	    Value redo_dir = {.int_v = direction};
	    user_event_push(
		&proj->history,
		undo_redo_move_automation,
		undo_redo_move_automation,
		NULL, NULL,
		(void *)a,
		NULL,
		undo_dir, undo_dir,
		redo_dir, redo_dir,
		0, 0, false, false);
	}
    }
    TEST_FN_CALL(track_automation_order, track);
}

static void timeline_move_track_at_index(Timeline *tl, int index, int direction, bool from_undo);
NEW_EVENT_FN(undo_redo_move_track_at_index, "undo/redo move track")
    int direction = val1.int_v;
    int index = val2.int_v;
    Timeline *tl = (Timeline *)obj1;
    timeline_move_track_at_index(tl, index, direction, true);

}

static void timeline_move_track_at_index(Timeline *tl, int index, int direction, bool from_undo)
{
    Layout *sel = tl->track_area->children[index];
    Layout *swap = tl->track_area->children[index + direction];
    layout_swap_children(sel, swap);
    layout_force_reset(tl->track_area);
    timeline_rectify_track_indices(tl);

    if (!from_undo) {
	Value undo_dir = {.int_v = -1 * direction};
	Value redo_dir = {.int_v = direction};
	Value undo_index = {.int_v = index + direction};
	Value redo_index = {.int_v = index};
	user_event_push(
	    &proj->history,
	    undo_redo_move_track_at_index,
	    undo_redo_move_track_at_index,
	    NULL, NULL,
	    (void *)tl,
	    NULL,
	    undo_dir, undo_index,
	    redo_dir, redo_index,
	    0, 0, false, false);
    }


    /* Track *sel_track = timeline_selected_track(tl); */
    /* if (sel_track) { */
    /* 	timeline_refocus_track(tl, sel_track, direction > 0); */
    /* } else { */
    /* 	ClickTrack *sel_tt = timeline_selected_click_track(tl); */
    /* 	timeline_refocus_click_track(tl, sel_tt, direction > 0); */
    /* } */
}

void timeline_move_track_or_automation(Timeline *tl, int direction)
{
    
    Track *t = timeline_selected_track(tl);
    if (t && t->num_automations > 0 && t->selected_automation >= 0) {
	track_move_automation(t->automations[t->selected_automation], direction, false);
	return;
    }
    int new_pos;
    if ((new_pos = tl->layout_selector + direction) < 0 || new_pos > tl->track_area->num_children - 1)
	return;
    timeline_move_track_at_index(tl, tl->layout_selector, direction, false);
    tl->layout_selector += direction;
    timeline_rectify_track_indices(tl);
    t = timeline_selected_track(tl);
    if (t) {
	timeline_refocus_track(tl, t, direction > 0);
    } else {
	ClickTrack *tt = timeline_selected_click_track(tl);
	timeline_refocus_click_track(tl, tt, direction > 0);
    }
    timeline_reset(tl, false);
    tl->needs_redraw = true;
}

static void select_out_onclick(void *arg)
{
    int index = *((int *)arg);
    audioconn_close(proj->playback_conn);
    proj->playback_conn = proj->playback_conns[index];
    if (audioconn_open(proj, proj->playback_conn) != 0) {
	fprintf(stderr, "Error: failed to open default audio conn \"%s\". More info: %s\n", proj->playback_conn->name, SDL_GetError());
	status_set_errstr("Error: failed to open default audio conn \"%s\"");
    }
    PageEl *el = panel_area_get_el_by_id(proj->panels, "panel_out_value");
    textbox_set_value_handle(((Button *)el->component)->tb, proj->playback_conn->name);
    window_pop_menu(main_win);
    Timeline *tl = proj->timelines[proj->active_tl_index];
    tl->needs_redraw = true;
    /* window_pop_mode(main_win); */
}
void project_set_default_out(void *nullarg)
{
    PageEl *el = panel_area_get_el_by_id(proj->panels, "panel_out_value");
    SDL_Rect *rect = &((Button *)el->component)->tb->layout->rect;
    Menu *menu = menu_create_at_point(rect->x, rect->y);
    MenuColumn *c = menu_column_add(menu, "");
    MenuSection *sc = menu_section_add(c, "");
    for (int i=0; i<proj->num_playback_conns; i++) {
	AudioConn *conn = proj->playback_conns[i];
	/* fprintf(stdout, "Conn index: %d\n", conn->index); */
	if (conn->available) {
	    menu_item_add(
		sc,
		conn->name,
		" ",
		select_out_onclick,
		&(conn->index));
	}
    }
    menu_add_header(menu,"", "Select the default audio output.\n\n'n' to select next item; 'p' to select previous item.");
    /* menu_reset_layout(menu); */
    window_add_menu(main_win, menu);
}

void project_set_chunk_size(uint16_t new_chunk_size)
{
    if (proj->recording) {
	transport_stop_recording();
    }
    transport_stop_playback();
    proj->chunk_size_sframes = new_chunk_size;
    for (uint8_t i=0; i<proj->num_record_conns; i++) {
	audioconn_reset_chunk_size(proj->record_conns[i], new_chunk_size);
    }
    for (uint8_t i=0; i<proj->num_playback_conns; i++) {
	audioconn_reset_chunk_size(proj->playback_conns[i], new_chunk_size);
    }
    audioconn_open(proj, proj->playback_conn);
    if (proj->freq_domain_plot) {
	waveform_destroy_freq_plot(proj->freq_domain_plot);
	proj->freq_domain_plot = NULL;
    }

}

static bool refocus_track_lt(Timeline *tl, Layout *lt, Layout *inner, bool at_bottom)
{
    int y_diff = inner->rect.y - lt->parent->rect.y;
    int audio_rect_h_by_2 = tl->proj->audio_rect->h / 2;
    if (lt->rect.y + lt->rect.h > proj->audio_rect->y + proj->audio_rect->h || lt->rect.y < proj->audio_rect->y) {
	if (at_bottom) {
	    lt->parent->scroll_offset_v = -1 * (y_diff - audio_rect_h_by_2) / main_win->dpi_scale_factor;
	} else {
	    lt->parent->scroll_offset_v = -1 * y_diff / main_win->dpi_scale_factor;
	}
	if (lt->parent->scroll_offset_v > 0) lt->parent->scroll_offset_v = 0;
	layout_force_reset(lt->parent);
	return true;
    }
    return false;
}

bool timeline_refocus_click_track(Timeline *tl, ClickTrack *tt, bool at_bottom)
{
    Layout *lt = tt->layout;
    return refocus_track_lt(tl, lt, lt, at_bottom);
}	

bool timeline_refocus_track(Timeline *tl, Track *track, bool at_bottom)
{
    Layout *lt, *inner;
    lt = track->layout;
    if (track->selected_automation > 0) {
	inner = track->automations[track->selected_automation]->layout;
    } else {
	inner = track->layout;
    }
    return refocus_track_lt(tl, lt, inner, at_bottom);

}


void timeline_play_speed_set(double new_speed)
{
    Timeline *tl = proj->timelines[proj->active_tl_index];
    double old_speed = proj->play_speed;
    proj->play_speed = new_speed;
    
    /* If speed crosses the zero line, need to invalidate direction-dependent caches */
    if (proj->play_speed * old_speed < 0.0) {
	timeline_set_play_position(tl, tl->play_pos_sframes);
    }
    
    status_stat_playspeed();

}
void timeline_play_speed_mult(double scale_factor)
{
    double new_speed = proj->play_speed * scale_factor;
    if (fabs(new_speed) > MAX_PLAY_SPEED) {
	timeline_play_speed_set(proj->play_speed);
    } else {
	timeline_play_speed_set(new_speed);
    }
}

void timeline_play_speed_adj(double dim)
{
    double new_speed = proj->play_speed;
    if (main_win->i_state & I_STATE_CMDCTRL) {
	new_speed += dim * PLAYSPEED_ADJUST_SCALAR_LARGE;
    } else {
	new_speed += dim * PLAYSPEED_ADJUST_SCALAR_SMALL;
    }
    timeline_play_speed_set(new_speed);
}

void timeline_scroll_playhead(double dim)
{
    Timeline *tl = proj->timelines[proj->active_tl_index];
    if (main_win->i_state & I_STATE_CMDCTRL) {
	dim *= proj->sample_rate * tl->sample_frames_per_pixel * PLAYHEAD_ADJUST_SCALAR_LARGE;
    } else {
	dim *= proj->sample_rate * tl->sample_frames_per_pixel * PLAYHEAD_ADJUST_SCALAR_SMALL;
    }
    int32_t new_pos = tl->play_pos_sframes + dim;
    timeline_set_play_position(proj->timelines[proj->active_tl_index], new_pos);
}


void project_active_tl_redraw(Project *proj)
{
    proj->timelines[proj->active_tl_index]->needs_redraw = true;
}

static bool track_minimize(Track *t)
{
    t->minimized = !t->minimized;
    if (t->minimized) {
	if (t->num_automations > 0) {
	    track_automations_hide_all(t);
	}
	t->layout->h.value = 31.0;
	t->inner_layout->h.value = 31.0;
    } else {
	t->layout->h.value = 96.0;
	t->inner_layout->h.value = 96.0;
    }

    return t->minimized;
}

void timeline_minimize_track_or_tracks(Timeline *tl)
{
    bool some_active = false;
    bool to_min = false;
    bool from_min = false;
    for (uint8_t i=0; i<tl->num_tracks; i++) {
	Track *t = tl->tracks[i];
	if (t->active) {
	    some_active = true;
	    if (track_minimize(t)) {
		to_min = true;
	    } else {
		from_min = true;
	    }
	}
    }
    if (!some_active) {
	Track *t = timeline_selected_track(tl);
	if (t) track_minimize(t);
    } else if (to_min && from_min) {
	for (uint8_t i=0; i<tl->num_tracks; i++) {
	    Track *t = tl->tracks[i];
	    if (t->active && !t->minimized) {
		track_minimize(t);
	    }
	}
    }
    timeline_rectify_track_area(tl);
    timeline_reset(tl, false);

}




