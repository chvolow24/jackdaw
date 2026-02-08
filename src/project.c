/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

#include <errno.h>
#include <pthread.h>
#include <stdatomic.h>
#include <string.h>
#include <semaphore.h>
#include <fcntl.h>
#include "assets.h"
#include "audio_clip.h"
#include "audio_connection.h"
#include "clipref.h"
#include "color.h"
#include "consts.h"
#include "components.h"
/* #include "dsp.h" */
#include "endpoint_callbacks.h"
#include "endpoint.h"
#include "endpoint_callbacks.h"
#include "input.h"
#include "layout.h"
#include "layout_xml.h"
#include "log.h"
#include "menu.h"
#include "midi_io.h"
#include "midi_objs.h"
#include "midi_qwerty.h"
#include "page.h"
#include "panel.h"
#include "piano_roll.h"
#include "project.h"
#include "session.h"
#include "status.h"
#include "text.h"
#include "textbox.h"
#include "timeline.h"
#include "transport.h"
#include "waveform.h"
#include "window.h"

#define DEFAULT_SFPP 600
#define MAX_SFPP 96000
#define CR_RECT_V_PAD (4 * main_win->dpi_scale_factor)
#define NUM_TRACK_COLORS 7

/* #define CLIPREF_NAMELABEL_H 20 */
/* #define CLIPREF_NAMELABEL_H_PAD 8 */
/* #define CLIPREF_NAMELABEL_V_PAD 2 */

#define TRACK_NAME_H_PAD 3
#define TRACK_NAME_V_PAD 1
#define TRACK_CTRL_SLIDER_H_PAD 7
#define TRACK_CTRL_SLIDER_V_PAD 5

#define SEM_NAME_UNPAUSE "/tl_%d_unpause_sem"
#define SEM_NAME_WRITABLE_CHUNKS "/tl_%d_writable_chunks"
#define SEM_NAME_READABLE_CHUNKS "/tl_%d_readable_chunks"

#define PLAYSPEED_ADJUST_SCALAR_LARGE 0.1f
#define PLAYSPEED_ADJUST_SCALAR_SMALL 0.015f
#define PLAYSPEED_ADJUST_SCALAR_TINY 0.001f

#define PLAYHEAD_ADJUST_SCALAR_LARGE 0.001f;
#define PLAYHEAD_ADJUST_SCALAR_SMALL 0.00015f

extern Window *main_win;

extern struct colors colors;

extern Symbol *SYMBOL_TABLE[];

/* SDL_Color color_mute_solo_grey = {210, 210, 210, 255}; */


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
    Session *session = session_get();
    /* Layout *tl_lt = layout_get_child_by_name_recursive(session->gui.layout, "timeline"); */
    Layout *tl_lt = session->gui.timeline_lt;

    /* If loading a file, session proj will have > 0 timelines and we can't use the existing one */
    if (session->proj.num_timelines == 0) {
	new_tl->track_area = layout_get_child_by_name_recursive(tl_lt, "tracks_area");
	for (int i=0; i<new_tl->track_area->num_children; i++) {
	    layout_destroy_no_offset(new_tl->track_area->children[i]);
	}
	new_tl->track_area->num_children = 0;

    } else {
	Layout *track_area_copy = layout_copy(session->proj.timelines[0]->track_area, session->gui.timeline_lt);
	track_area_copy->scroll_offset_h = 0;
	track_area_copy->scroll_offset_v = 0;
	track_area_copy->scroll_momentum_h = 0;
	track_area_copy->scroll_momentum_v = 0;
	/* layout_write(stderr, track_area_copy, 0); */
	for (int i=0; i<track_area_copy->num_children; i++) {
	    layout_destroy_no_offset(track_area_copy->children[i]);
	}
	track_area_copy->num_children = 0;
	new_tl->track_area = track_area_copy;
	timeline_rectify_track_area(new_tl);
	/* layout_write(stderr, track_area_copy, 0); */

    }


    /* TODO:

       FIX NEW TIMELINE LAYOUT LOGIC

    */


       
    /* /\* if (new_tl->index != 0) { *\/ */
    /* 	Layout *main_lt_fresh = layout_read_from_xml(MAIN_LT_PATH); */
    /* 	Layout *new_tl_lt = layout_get_child_by_name_recursive(main_lt_fresh, "timeline"); */
    /* 	Layout *cpy = layout_copy(new_tl_lt, tl_lt->parent); */
    /* 	layout_destroy(main_lt_fresh); */
    /* 	/\* Layout *cpy = layout_copy(tl_lt, tl_lt->parent); *\/ */
    /* 	/\* new_tl->layout = cpy; *\/ */
    /* /\* } else { *\/ */
    /* 	/\* new_tl->layout = tl_lt; *\/ */
    /* /\* } *\/ */
    /* 	breakfn(); */
    /* 	/\* layout_destroy(tl_lt); *\/ */
	
    /* /\* layout_reset(new_tl->layout); *\/ */
    /* new_tl->track_area = layout_get_child_by_name_recursive(new_tl->layout, "tracks_area"); */

    /* /\* Clear tracks in old timeline layout before copying *\/ */
    /* for (int i=0; i<new_tl->track_area->num_children; i++) { */
    /* 	layout_destroy_no_offset(new_tl->track_area->children[i]); */
    /* } */
    /* new_tl->track_area->num_children = 0; */

    
    /* new_tl->layout = layout_get_child_by_name_recursive(session->gui.layout, "timeline"); */
    
    new_tl->timeview.sample_frames_per_pixel = DEFAULT_SFPP;
    new_tl->timeview.rect = session->gui.audio_rect;
    new_tl->timeview.play_pos = &new_tl->play_pos_sframes;
    new_tl->timeview.in_mark = &new_tl->in_mark_sframes;
    new_tl->timeview.out_mark = &new_tl->out_mark_sframes;
    new_tl->timeview.offset_left_sframes = 0;
    new_tl->timeview.restrict_view = true;
    new_tl->timeview.view_min = TL_MIN_SFRAMES;
    new_tl->timeview.view_max = TL_MAX_SFRAMES;
    new_tl->timeview.max_sfpp = MAX_SFPP;
    
    strcpy(new_tl->timecode.str, "+00:00:00:00000");
    Layout *tc_lt = layout_get_child_by_name_recursive(tl_lt, "timecode");
    if (!session->gui.timecode_tb) {
	session->gui.timecode_tb = textbox_create_from_str(
	    new_tl->timecode.str,
	    tc_lt,
	    main_win->mono_bold_font,
	    16,
	    main_win);
	textbox_set_background_color(session->gui.timecode_tb, &colors.black);
	textbox_set_text_color(session->gui.timecode_tb, &colors.white);
	textbox_set_trunc(session->gui.timecode_tb, false);
    }

    Layout *ruler_lt = layout_get_child_by_name_recursive(tl_lt, "ruler");
    Layout *lemniscate_lt = layout_add_child(ruler_lt);
    lemniscate_lt->h.type = SCALE;
    lemniscate_lt->h.value = 1.0;
    lemniscate_lt->w.type = ABS;
    lemniscate_lt->w.value = 500.0;
    lemniscate_lt->x.type = ABS;
    lemniscate_lt->x.value = 0.0;
    layout_force_reset(lemniscate_lt);
    if (!session->gui.loop_play_lemniscate) {
	session->gui.loop_play_lemniscate = textbox_create_from_str(
	    "∞",
	    lemniscate_lt,
	    main_win->std_font,
	    24,
	    main_win);
	textbox_set_background_color(session->gui.loop_play_lemniscate, &colors.clear);
	textbox_set_text_color(session->gui.loop_play_lemniscate, &colors.white);
	textbox_reset_full(session->gui.loop_play_lemniscate);
    }
    
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
    new_tl->writable_small_chunks = 0;
    atomic_init(&new_tl->writable_chunks, 0);
    atomic_init(&new_tl->readable_chunks, 0);
/* retry2: */
/*     snprintf(buf, 128, SEM_NAME_READABLE_CHUNKS, new_tl->index); */
/*     if ((new_tl->readable_chunks = sem_open(buf, O_CREAT | O_EXCL, 0666, 0)) == SEM_FAILED) { */
/* 	if (errno != EEXIST) { */
/* 	    perror("Error opening readable chunks sem"); */
/* 	} */
/* 	sem_unlink(buf); */
/* 	if (!retry) { */
/* 	    goto retry2; */
/* 	    retry = true; */
/* 	} else { */
/* 	    fprintf(stderr, "Fatal error: retry failed\n"); */
/* 	    exit(1); */
/* 	} */

/* 	/\* exit(1); *\/ */
/*     } */
/* retry3: */
/*     snprintf(buf, 128, SEM_NAME_WRITABLE_CHUNKS, new_tl->index); */
/*     int init_writable_chunks = proj->fourier_len_sframes * RING_BUF_LEN_FFT_CHUNKS / proj->chunk_size_sframes; */
/*     if ((new_tl->writable_chunks = sem_open(buf, O_CREAT | O_EXCL, 0666, init_writable_chunks)) == SEM_FAILED) { */
/* 	if (errno != EEXIST) { */
/* 	    perror("Error opening writable chunks sem"); */
/* 	} */
/* 	sem_unlink(buf); */
/* 	if (!retry) { */
/* 	    goto retry3; */
/* 	    retry = true; */
/* 	} else { */
/* 	    fprintf(stderr, "Fatal error: retry failed\n"); */
/* 	    exit(1); */
/* 	} */
/* 	/\* exit(1); *\/ */
/*     } */
    new_tl->needs_redraw = true;
    proj->timelines[proj->num_timelines] = new_tl;
    proj->num_timelines++;

    api_node_register(&new_tl->api_node, &session->server.api_root, new_tl->name, NULL);
    
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
    Project *proj = tl->proj;
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

    /* if (tl->timecode_tb) textbox_destroy(tl->timecode_tb); */
    /* if (tl->loop_play_lemniscate) textbox_destroy(tl->loop_play_lemniscate); */

    if (sem_close(tl->unpause_sem) != 0) perror("Sem close");
    /* if (sem_close(tl->writable_chunks) != 0) perror("Sem close"); */
    /* if (sem_close(tl->readable_chunks) != 0) perror("Sem close"); */

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

    free(tl->dsp_chunks_info);
    /* layout_destroy(tl->layout); */
    layout_destroy(tl->track_area);
    free(tl);
}

/* static void clip_destroy_no_displace(Clip *clip); */

void project_deinit(Project *proj)
{
    /* fprintf(stdout, "PROJECT_DESTROY num tracks: %d\n", proj->timelines[0]->num_tracks); */
    for (uint16_t i=0; i<proj->num_clips; i++) {
	clip_destroy_no_displace(proj->clips[i]);
    }
    for (uint16_t i=0; i<proj->num_midi_clips; i++) {
	midi_clip_destroy(proj->midi_clips[i]);
    }

    for (uint8_t i=0; i<proj->num_timelines; i++) {
	timeline_destroy(proj->timelines[i], false);
    }

    free(proj->output_L);
    free(proj->output_R);
    free(proj->output_L_freq);
    free(proj->output_R_freq);
}

void project_reset_tl_label(Project *proj)
{
    Session *session = session_get();
    Timeline *tl = ACTIVE_TL;
    snprintf(session->gui.timeline_label_str, MAX_NAMELENGTH, "Timeline %d: \"%s\"\n", proj->active_tl_index + 1, tl->name);
    textbox_reset_full(session->gui.timeline_label);
}

/* void project_init_audio_conns(Project *proj); */
/* int project_init_midi(Project *proj); */
/* void project_init_quickref(Project *proj, Layout *control_bar_layout); */
int project_init(
    Project *proj,
    char *name,
    uint8_t channels,
    uint32_t sample_rate,
    SDL_AudioFormat fmt,
    uint16_t chunk_size_sframes,
    uint16_t fourier_len_sframes,
    bool create_empty_timeline
    )
{
    /* Project *proj = calloc(1, sizeof(Project)); */
    if (strlen(name) > MAX_NAMELENGTH - 1) {
	fprintf(stderr, "Error: project name exceeds max len (%d)\n", MAX_NAMELENGTH);
	return -1;
    }
    
    strcpy(proj->name, name);
    char win_title_buf[MAX_NAMELENGTH];
    snprintf(win_title_buf, MAX_NAMELENGTH, "Jackdaw - %s", proj->name);
    
    SDL_SetWindowTitle(main_win->win, win_title_buf);

    proj->channels = channels;
    proj->sample_rate = sample_rate;
    proj->fmt = fmt;
    proj->chunk_size_sframes = chunk_size_sframes;
    proj->fourier_len_sframes = fourier_len_sframes;
    /* Layout *source_lt = layout_get_child_by_name_recursive(session->gui.layout, "source_area"); */
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
    /* textbox_set_background_color(proj->source_name_tb, &colors.clear); */
    /* textbox_set_text_color(proj->source_name_tb, &colors.white); */

    if (create_empty_timeline) {
	project_add_timeline(proj, "Main");
	project_reset_tl_label(proj);
    }
    /* Initialize output */
    /* proj->output_len = chunk_size_sframes; */


    proj->output_L = malloc(sizeof(float) * fourier_len_sframes);
    proj->output_R = malloc(sizeof(float) * fourier_len_sframes);
    proj->output_L_freq = malloc(sizeof(double) * fourier_len_sframes);
    proj->output_R_freq  = malloc(sizeof(double) * fourier_len_sframes);
    memset(proj->output_L, '\0', sizeof(float) * fourier_len_sframes);
    memset(proj->output_R, '\0', sizeof(float) * fourier_len_sframes);

    envelope_follower_set_times_msec(&proj->output_L_ef, ENV_F_STD_ATTACK_MSEC, ENV_F_STD_RELEASE_MSEC, sample_rate);
    envelope_follower_set_times_msec(&proj->output_R_ef, ENV_F_STD_ATTACK_MSEC, ENV_F_STD_RELEASE_MSEC, sample_rate);

    

    /* strcpy(session->status_bar.errstr, "Uh oh! this is an call!\n"); */
    /* strcpy(session->status_bar.errorstr, "Ok error str\n"); */



    

    /* Initialize endpoints */
    /* api_start_server(proj, 9302); */

    return 0;
}



extern SDL_Color source_mode_bckgrnd;
extern SDL_Color clip_ref_home_bckgrnd;
extern SDL_Color timeline_marked_bckgrnd;



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

int project_obj_name_completion(Text *txt, void *obj)
{   
    Value nullval = {.int_v = 0};
    char *old_value = strdup(txt->cached_value);

    if (obj) {
	APINode *node = (APINode *)obj;
	api_node_renamed(node);
    }
    
    user_event_push(
	undo_rename_object,
	redo_rename_object,
	NULL, NULL,
	(void *)txt,
	old_value,
	nullval, nullval, nullval, nullval,
	0, 0, false, true);
    return 0;
}


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

    if (tl->click_track_frozen) {
	click_track_stack[click_track_index] = tl->click_tracks[0];
	click_track_index++;
    }

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
	for (int j=0; j<tl->num_click_tracks; j++) {
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
    if (tl->layout_selector < 0) return NULL; /* frozen CT */
    if (tl->num_tracks == 0 || tl->track_selector < 0) {
	return NULL;
    } else if (tl->track_selector > tl->num_tracks - 1) {
	fprintf(stderr, "ERROR: track selector points past end of tracks arr\n");
	exit(1);
    } else {
	return tl->tracks[tl->track_selector];
    }
}

void timeline_select_track(Track *track)
{
    Timeline *tl = track->tl;
    tl->track_selector = track->tl_rank;
    tl->layout_selector = track->layout->index;
    tl->click_track_selector = -1;
    tl->needs_redraw = true;
}

ClickTrack *timeline_selected_click_track(Timeline *tl)
{
    if (tl->layout_selector < 0 && tl->click_track_frozen)
	return tl->click_tracks[0];
    if (tl->num_click_tracks == 0 || tl->click_track_selector < 0) {
	return NULL;
    } else if (tl->click_track_selector > tl->num_click_tracks - 1) {
	fprintf(stderr, "ERROR: tempo track selector points past end of tracks arr\n");
	exit(1);
    } else {
	return tl->click_tracks[tl->click_track_selector];
    }
}

ClickTrack *track_governing_click_track(Track *t)
{
    ClickTrack *ct = NULL;
    for (int i=t->tl->num_click_tracks - 1; i>=0; i--) {
	if (t->tl->click_tracks[i]->layout->index <= t->layout->index) {
	    ct = t->tl->click_tracks[i];
	    break;
	}
    }
    if (!ct && t->tl->click_track_frozen) return t->tl->click_tracks[0];
    return ct;

}

/* A click track "governs" all tracks situated below it until another click track appears */
ClickTrack *timeline_governing_click_track(Timeline *tl)
{
    ClickTrack *ct = NULL;
    for (int i=tl->num_click_tracks - 1; i>=0; i--) {
	if (tl->click_tracks[i]->layout->index <= tl->layout_selector) {
	    ct = tl->click_tracks[i];
	    break;
	}
    }
    if (!ct && tl->click_track_frozen) return tl->click_tracks[0];

    return ct;
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

static void vol_dsp_cb(Endpoint *ep)
{
    Track *track = ep->xarg3;
    float new_ctrl_val = ep->current_write_val.float_v;
    track->vol = pow(new_ctrl_val, VOL_EXP);
}

Track *timeline_add_track_with_name(Timeline *tl, const char *track_name, int at)
{
    if (tl->num_tracks == MAX_TRACKS) return NULL;
    Track *track = calloc(1, sizeof(Track));
    tl->tracks[tl->num_tracks] = track;
    track->tl_rank = tl->num_tracks++;
    track->tl = tl;
    strncpy(track->name, track_name, MAX_NAMELENGTH);

    track->channels = tl->proj->channels;

    track->clips_alloc_len = 16;
    track->clips = calloc(track->clips_alloc_len, sizeof(ClipRef *));

    Session *session = session_get();
    track->input = session->audio_io.record_conns[session->audio_io.default_record_conn_index];
    track->color = track_colors[track_color_index];
    if (track_color_index < NUM_TRACK_COLORS -1) {
	track_color_index++;
    } else {
	track_color_index = 0;
    }

    effect_chain_init(&track->effect_chain, tl->proj, &track->api_node, track->name, tl->proj->fourier_len_sframes);
    /* int err = pthread_mutex_init(&track->effect_chain_lock, NULL); */
    /* if (err != 0) { */
    /* 	fprintf(stderr, "Error initializing effect chain lock: %s\n", strerror(err)); */
    /* } */

    midi_event_ring_buf_init(&track->note_offs);
   
    api_node_register(&track->api_node, &track->tl->api_node, track->name, NULL);
        
    endpoint_init(
	&track->vol_ep,
	&track->vol_ctrl_val,
	JDAW_FLOAT,
	"vol",
	"Vol",
	JDAW_THREAD_DSP,
	track_slider_cb, NULL, vol_dsp_cb,
	&track->vol_ctrl, track->tl, track, NULL);
    endpoint_set_allowed_range(
	&track->vol_ep,
	(Value){.float_v=0.0},
	(Value){.float_v=TRACK_VOL_MAX_PRE_EXP});

    endpoint_set_default_value(&track->vol_ep, (Value){.float_v = 1.0});
    endpoint_set_label_fn(&track->vol_ep, label_amp_pre_exp_to_dbstr);
    api_endpoint_register(&track->vol_ep, &track->api_node);

    endpoint_init(
	&track->pan_ep,
	&track->pan,
	JDAW_FLOAT,
	"pan",
	"Pan",
	JDAW_THREAD_DSP,
	track_slider_cb,
	NULL, NULL,
	&track->pan_ctrl, track->tl,
	NULL, NULL);
    endpoint_set_allowed_range(
	&track->pan_ep,
	(Value){.float_v = 0.0},
	(Value){.float_v = 1.0});
    endpoint_set_default_value(&track->pan_ep, (Value){.float_v = 0.5});
    endpoint_set_label_fn(&track->pan_ep, label_pan);
    api_endpoint_register(&track->pan_ep, &track->api_node);



    /* track->eq.track = track; */
    /* eq_init(&track->eq); */

    /* track->saturation.track = track; */
    /* saturation_init(&track->saturation); */
    /* int ir_len = track->tl->proj->fourier_len_sframes/4; */
    /* int fr_len = track->tl->proj->fourier_len_sframes * 2; */
    /* track->buf_L_freq_mag = calloc(fr_len, sizeof(double)); */
    /* track->buf_R_freq_mag = calloc(fr_len, sizeof(double)); */
    /* filter_init(&track->fir_filter, track, LOWPASS, ir_len, fr_len); */

    /* delay_line_init(&track->delay_line, track, track->tl->session_get_sample_rate()); */





    
    /* Layout *track_area = layout_get_child_by_name_recursive(tl->layout, "tracks_area"); */
    Layout *track_template = layout_read_xml_to_lt(tl->track_area, TRACK_LT_PATH);
    track->layout = track_template;

    if (at > -1) {
	layout_displace_child(track->layout, at - tl->track_area->num_children + 1);
    }
        
    track->inner_layout = track_template->children[0];
    timeline_rectify_track_area(tl);
    timeline_rectify_track_indices(tl);
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
    textbox_set_background_color(track->tb_vol_label, &colors.clear);
    textbox_set_align(track->tb_vol_label, CENTER_LEFT);
    textbox_set_pad(track->tb_vol_label, TRACK_NAME_H_PAD, 0);
    textbox_set_trunc(track->tb_vol_label, false);

    track->tb_pan_label = textbox_create_from_str(
	"Pan:",
	pan_label,
	main_win->bold_font,
	12,
	main_win);
    textbox_set_background_color(track->tb_pan_label, &colors.clear);
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
	project_obj_name_completion,
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
    textbox_set_border(track_tb, &colors.black, 1, 0);
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
    textbox_set_background_color(track->tb_input_label, &colors.clear);
    textbox_set_align(track->tb_input_label, CENTER_LEFT);
    textbox_set_pad(track->tb_input_label, TRACK_NAME_H_PAD, 0);


    const char *track_in_name;
    switch (track->input_type) {
    case MIDI_DEVICE:
	track_in_name = ((MIDIDevice *)track->input)->name;
	break;
    case AUDIO_CONN:
	track_in_name = ((AudioConn *)track->input)->name;
    }
    track->tb_input_name = textbox_create_from_str(
	track_in_name,
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
    textbox_set_border(track->tb_input_name, &colors.black, 1, BUBBLE_CORNER_RADIUS);
    /* textbox_set_style(track->tb_input_name, BUTTON_CLASSIC); */

    
    track->tb_mute_button = textbox_create_from_str(
	"M",
	mute,
	main_win->bold_font,
	14,
	main_win);
    textbox_set_border(track->tb_mute_button, &colors.black, 1, MUTE_SOLO_BUTTON_CORNER_RADIUS);
    textbox_set_background_color(track->tb_mute_button, &colors.mute_solo_grey);
    /* textbox_set_style(track->tb_mute_button, BUTTON_CLASSIC); */
    /* textbox_reset_full(track->tb_mute_button); */

    track->tb_solo_button = textbox_create_from_str(
	"S",
	solo,
	main_win->bold_font,
	14,
	main_win);
    textbox_set_border(track->tb_solo_button, &colors.black, 1, MUTE_SOLO_BUTTON_CORNER_RADIUS);
    textbox_set_background_color(track->tb_solo_button, &colors.mute_solo_grey);
    /* textbox_set_style(track->tb_solo_button, BUTTON_CLASSIC); */
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
    track->vol_ctrl_val = 1.0f;

    track->vol_ctrl = slider_create(
	vol_ctrl_lt,
	&track->vol_ep,
	(Value){.float_v = 0.0f},
	(Value){.float_v = TRACK_VOL_MAX_PRE_EXP},
	SLIDER_HORIZONTAL,
	SLIDER_FILL,
	&label_amp_pre_exp_to_dbstr,
	&session->dragged_component);
    slider_add_point_of_interest(track->vol_ctrl, (Value){.float_v = 1.0});
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
    /* 	&tl->session->dragged_component); */
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
    /* 	&tl->session->dragged_component); */
    track->pan_ctrl = slider_create(
	pan_ctrl_lt,
	&track->pan_ep,
	(Value){.float_v = 0.0f},
	(Value){.float_v = 1.0f},
	SLIDER_HORIZONTAL,
	SLIDER_TICK,
	label_pan,
	&session->dragged_component);
    slider_add_point_of_interest(track->pan_ctrl, (Value){.float_v = 0.5});
    track->pan_ctrl->disallow_unsafe_mode = true;
    /* track->pan_ep.xarg1 = track->pan_ctrl; */

    /* slider_reset(track->vol_ctrl); */
    /* slider_reset(track->pan_ctrl); */
    Layout *auto_dropdown_lt = layout_get_child_by_name_recursive(track->inner_layout, "automation_dropdown");
    track->automation_dropdown = symbol_button_create(
	auto_dropdown_lt,
	SYMBOL_DROPDOWN,
	auto_dropdown_action,
	(void *)track,
	NULL);
    track->automation_dropdown->background_color = &colors.grey;
    auto_dropdown_lt->w.value = (float)SYMBOL_TABLE[track->automation_dropdown->symbol_index]->x_dim_pix / main_win->dpi_scale_factor;
    auto_dropdown_lt->h.value = (float)SYMBOL_TABLE[track->automation_dropdown->symbol_index]->y_dim_pix / main_win->dpi_scale_factor;
    auto_dropdown_lt->x.value += 4.0f;
    layout_reset(auto_dropdown_lt);
    
    track->selected_automation = -1;
    track->console_rect = &(layout_get_child_by_name_recursive(track->inner_layout, "track_console")->rect);
    track->colorbar = &(layout_get_child_by_name_recursive(track->inner_layout, "colorbar")->rect);


    track_reset_full(track);
    if (tl->layout_selector < 0) tl->layout_selector = 0;    
    timeline_rectify_track_indices(tl);

    /* api_endpoint_register(&track->saturation.gain_ep, &track->saturation.track->api_node); */
    return track;

}

Track *timeline_add_track(Timeline *tl, int at)
{
    if (tl->num_tracks == MAX_TRACKS) return NULL;
    char name[MAX_NAMELENGTH];
    snprintf(name, sizeof(name), "Track %d", tl->num_tracks + 1);

    return timeline_add_track_with_name(tl, name, at);
}


void project_clear_active_clips()
{
    Session *session = session_get();
    session->proj.active_clip_index = session->proj.num_clips;
}

/* int32_t clip_ref_len(ClipRef *cr) */
/* { */
/*     if (cr->out_mark_sframes <= cr->start_in_clip) { */
/* 	return cr->clip->len_sframes; */
/*     } else { */
/* 	return cr->out_mark_sframes - cr->start_in_clip; */
/*     } */
/* } */

/* void midi_clipref_reset(MIDIClipRef *mcr, bool rescaled) */
/* { */
/*     mcr->layout->rect.x = timeline_get_draw_x(mcr->track->tl, mcr->tl_pos); */

/*     /\* TODO: Len! *\/ */
/*     uint32_t cr_len = 96000; */
/*     /\* uint32_t cr_len = mcr->start_in_clip >= mcr->end_in_clip *\/ */
/*     /\* 	? mcr->clip->len_sframes *\/ */
/*     /\* 	: cr->out_mark_sframes - cr->start_in_clip; *\/ */
/*     mcr->layout->rect.w = timeline_get_draw_w(mcr->track->tl, cr_len); */
/*     mcr->layout->rect.y = mcr->track->inner_layout->rect.y + CR_RECT_V_PAD; */
/*     mcr->layout->rect.h = mcr->track->inner_layout->rect.h - 2 * CR_RECT_V_PAD; */
/*     layout_set_values_from_rect(mcr->layout); */
/*     layout_reset(mcr->layout); */
/*     textbox_reset_full(mcr->label); */
/*     /\* cr->waveform_redraw = true; *\/ */

/* } */

/* void clipref_reset(ClipRef *cr, bool rescaled) */
/* { */

/*     cr->layout->rect.x = timeline_get_draw_x(cr->track->tl, cr->tl_pos); */
/*     uint32_t cr_len = cr->start_in_clip >= cr->out_mark_sframes */
/* 	? cr->clip->len_sframes */
/* 	: cr->out_mark_sframes - cr->start_in_clip; */
/*     cr->layout->rect.w = timeline_get_draw_w(cr->track->tl, cr_len); */
/*     cr->layout->rect.y = cr->track->inner_layout->rect.y + CR_RECT_V_PAD; */
/*     cr->layout->rect.h = cr->track->inner_layout->rect.h - 2 * CR_RECT_V_PAD; */
/*     layout_set_values_from_rect(cr->layout); */
/*     layout_reset(cr->layout); */
/*     textbox_reset_full(cr->label); */
/*     /\* if (rescaled) { *\/ */
/*     cr->waveform_redraw = true; */
/*     /\* } *\/ */
/*     /\* cr->rect.x = timeline_get_draw_x(cr->tl_pos); *\/ */
/*     /\* uint32_t cr_len = cr->start_in_clip >= cr->out_mark_sframes *\/ */
/*     /\* 	? cr->clip->len_sframes *\/ */
/*     /\* 	: cr->out_mark_sframes - cr->start_in_clip; *\/ */
/*     /\* cr->rect.w = timeline_get_draw_w(cr_len); *\/ */
/*     /\* cr->rect.y = cr->track->inner_layout->rect.y + CR_RECT_V_PAD; *\/ */
/*     /\* cr->rect.h = cr->track->layout->rect.h - 2 * CR_RECT_V_PAD; *\/ */
/* } */

/* static void clipref_remove_from_track(ClipRef *cr) */
/* { */
/*     bool displace = false; */
/*     Track *track = cr->track; */
/*     for (uint16_t i=0; i<track->num_clips; i++) { */
/* 	ClipRef *test = track->clips[i]; */
/* 	if (test == cr) { */
/* 	    displace = true; */
/* 	} else if (displace && i > 0) { */
/* 	    track->clips[i - 1] = test; */
	    
/* 	} */
/*     } */
/*     if (displace) track->num_clips--; /\* else not found! *\/ */
/* } */

/* void clipref_displace(ClipRef *cr, int displace_by) */
/* { */
/*     Track *track = cr->track; */
/*     Timeline *tl = track->tl; */
/*     int new_rank = track->tl_rank + displace_by; */
/*     if (new_rank >= 0 && new_rank < tl->num_tracks) { */
/* 	Track *new_track = tl->tracks[new_rank]; */
/* 	clipref_remove_from_track(cr); */
/* 	new_track->clips[new_track->num_clips] = cr; */
/* 	new_track->num_clips++; */
/* 	cr->track = new_track; */
/* 	/\* fprintf(stdout, "ADD CLIP TO TRACK %s, which has %d clips now\n", new_track->name, new_track->num_clips); *\/ */
	
/*     } */
/*     timeline_reset(tl, false); */
/* } */

/* static void clipref_insert_on_track(ClipRef *cr, Track *target) */
/* { */
/*     target->clips[target->num_clips] = cr; */
/*     target->num_clips++; */
/*     cr->track = target; */
/* } */

/* void clipref_move_to_track(ClipRef *cr, Track *target) */
/* { */
/*     Track *prev_track = cr->track; */
/*     Timeline *tl = prev_track->tl; */
/*     clipref_remove_from_track(cr); */
/*     clipref_insert_on_track(cr, target); */
/*     timeline_reset(tl, false); */
/* } */

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
    /* for (uint16_t i=0; i<track->num_clips; i++) { */
    /* 	clipref_reset(track->clips[i], true); */
    /* } */
    /* for (uint16_t i=0; i<track->num_midi_cliprefs; i++) { */
    /* 	midi_clipref_reset(track->midi_cliprefs[i], true); */
    /* } */
    slider_reset(track->vol_ctrl);
    slider_reset(track->pan_ctrl);
}
    

void track_reset(Track *track, bool rescaled)
{
    for (uint16_t i=0; i<track->num_clips; i++) {
	clipref_reset(track->clips[i], rescaled);
    }

    /* for (uint16_t i=0; i<track->num_clips; i++) { */
    /* 	clipref_reset(track->clips[i], rescaled); */
    /* } */
    /* for (uint16_t i=0; i<track->num_midi_cliprefs; i++) { */
    /* 	midi_clipref_reset(track->midi_cliprefs[i], rescaled); */
    /* } */
	
    for (uint8_t i=0; i<track->num_automations; i++) {
	Automation *a = track->automations[i];
	/* automation_clear_cache(track->automations[i]); */
	/* a->current = NULL; */
	if (a->shown) {
	    automation_reset_keyframe_x(a);
	}
    }
}

void timeline_reset_loop_play_lemniscate(Timeline *tl)
{
    if (tl->in_mark_sframes >= tl->out_mark_sframes) return;

    Session *session = session_get();
    int in_x = timeline_get_draw_x(tl, tl->in_mark_sframes);
    int out_x = timeline_get_draw_x(tl, tl->out_mark_sframes);
    layout_reset(session->gui.loop_play_lemniscate->layout);

    session->gui.loop_play_lemniscate->layout->rect.x = in_x;
    session->gui.loop_play_lemniscate->layout->rect.w = out_x - in_x;
    layout_set_values_from_rect(session->gui.loop_play_lemniscate->layout);
    textbox_reset(session->gui.loop_play_lemniscate);
    tl->needs_redraw = true;
    
}

void timeline_reset_full(Timeline *tl)
{
    for (int i=0; i<tl->num_tracks; i++) {
	track_reset_full(tl->tracks[i]);
    }

    Session *session = session_get();
    layout_reset(session->gui.layout);
    if (session->playback.loop_play) {
	timeline_reset_loop_play_lemniscate(tl);
    }

    tl->needs_redraw = true;

}

void timeline_reset(Timeline *tl, bool rescaled)
{
    Session *session = session_get();
    layout_reset(session->gui.layout);
    for (int i=0; i<tl->num_tracks; i++) {
	track_reset(tl->tracks[i], rescaled);
    }
    if (session->playback.loop_play) {
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
/* SDL_Color colors.solo_yellow = {255, 200, 0, 130}; */

/* SDL_Color mute_red = {255, 0, 0, 130}; */
/* SDL_Color solo_yellow = {255, 200, 0, 130}; */

/* SDL_Color mute_red = {240, 80, 80, 255}; */
/* SDL_Color solo_yellow = {255, 200, 50, 255}; */

extern SDL_Color textbox_default_bckgrnd_clr;

bool track_mute(Track *track)
{
    track->muted = !track->muted;
    if (track->muted) {
	textbox_set_background_color(track->tb_mute_button, &colors.mute_red);
	if (track->synth) {
	    synth_clear_all(track->synth);
	}
    } else {
	textbox_set_background_color(track->tb_mute_button, &colors.mute_solo_grey);
    }
    track->tl->needs_redraw = true;
    return track->muted;
}

bool track_solo(Track *track)
{
    track->solo = !track->solo;
    Textbox *piano_roll_solo_button = piano_roll_get_solo_button();
    if (track->solo) {
	if (track->muted) {
	    track->muted = false;
	    textbox_set_background_color(track->tb_mute_button, &colors.mute_solo_grey);
	}
	track->solo_muted = false;
	textbox_set_background_color(track->tb_solo_button, &colors.solo_yellow);
	if (piano_roll_solo_button) textbox_set_background_color(piano_roll_solo_button, &colors.solo_yellow);
    } else {
	textbox_set_background_color(track->tb_solo_button, &colors.mute_solo_grey);
	if (piano_roll_solo_button) textbox_set_background_color(piano_roll_solo_button, &colors.mute_solo_grey);

    }
    track->tl->needs_redraw = true;
    return track->solo;
}

void track_solomute(Track *track)
{
    track->solo_muted = true;
    textbox_set_background_color(track->tb_solo_button, &colors.mute_red);
    if (track->synth)
	synth_clear_all(track->synth);
}
void track_unsolomute(Track *track)
{
    track->solo_muted = false;
    textbox_set_background_color(track->tb_solo_button, &colors.mute_solo_grey);
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
    void *obj;
    enum track_in_type type;
};

struct midi_out_arg {
    Track *track;
    void *obj;
    enum midi_out_type type;
};

void track_set_out_builtin_synth(Track *track)
{
    if (!track->synth) {
	track->synth = synth_create(track);
    }
    track->midi_out_type = MIDI_OUT_SYNTH;
    track->midi_out = track->synth;
    timeline_check_set_midi_monitoring();
}

static void track_set_midi_out_onclick(void *arg_v)
{
    Session *session = session_get();
    struct midi_out_arg *arg = arg_v;
    Track *track = arg->track;
    track->midi_out_type = arg->type;
    if (arg->type == MIDI_OUT_SYNTH) {
	if (!track->synth) {
	    track->synth = synth_create(track);
	}
	track->midi_out = track->synth;
    } else {
	track->midi_out = arg->obj;
    }
    timeline_check_set_midi_monitoring();
    window_pop_menu(main_win);
    Timeline *tl = ACTIVE_TL;
    tl->needs_redraw = true;
}

void track_set_input_to(Track *track, enum track_in_type type, void *obj)
{
    Session *session = session_get();
    switch (type) {
    case AUDIO_CONN: {
	track->input_type = AUDIO_CONN;
	AudioConn *conn = obj;
	track->input = conn;
	textbox_set_value_handle(track->tb_input_name, conn->name);
    }
	break;
    case MIDI_DEVICE: {
	track->input_type = MIDI_DEVICE;
	MIDIDevice *device = obj;
	midi_device_open(device);
	track->input = obj;
	textbox_set_value_handle(track->tb_input_name, device->name);
	if (device->type == MIDI_DEVICE_QWERTY) {
	    panel_page_refocus(session->gui.panels, "QWERTY piano", 1);
	}
    }
	break;
    }
    timeline_check_set_midi_monitoring();
    if (session->piano_roll) {
	piano_roll_set_in();
    }
}


static void track_set_in_onclick(void *void_arg)
{
    struct track_in_arg *arg = (struct track_in_arg *)void_arg;
    track_set_input_to(arg->track, arg->type, arg->obj);
    arg->track->tl->needs_redraw = true;
    window_pop_menu(main_win);
}

void track_set_midi_out(Track *track)
{
    Session *session = session_get();
    /* Refresh device list: skip for now :( */
    /* session_populate_midi_device_lists(session_get()); */

    Menu *menu = menu_create_at_point(track->layout->rect.x + track->layout->rect.w / 2, track->layout->rect.y);
    MenuColumn *c = menu_column_add(menu, "");
    MenuSection *sc = menu_section_add(c, "");
    /* Project *proj = &session_get()->proj; */
    /* Session *session = session_get(); */
    for (int i=0; i<session->midi_io.num_outputs; i++) {
	struct midi_out_arg *arg = malloc(sizeof(struct midi_out_arg));
	MIDIDevice *d = session->midi_io.outputs + i;
	arg->track = track;
	arg->obj = d;
	arg->type = MIDI_OUT_DEVICE;

	MenuItem *item = menu_item_add(
	    sc,
	    d->name,
	    "(device)",
	    track_set_midi_out_onclick,
	    /* track_set_in_onclick, */
	    arg);
	
	    item->free_target_on_destroy = true;
    }

    struct midi_out_arg *arg = malloc(sizeof(struct midi_out_arg));
    arg->track = track;
    arg->type = MIDI_OUT_SYNTH;
    MenuItem *item = menu_item_add(
	sc,
	"Jackdaw Synth",
	"(softsynth)",
	track_set_midi_out_onclick,
	arg);
    item->free_target_on_destroy = true;
    menu_add_header(menu, "", "Select track midi out");
    window_add_menu(main_win, menu);
    /* int move_by_y = 0; */
    /* if ((move_by_y = y + menu->layout->rect.h - main_win->layout->rect.h) > 0) { */
    /* 	menu_translate(menu, 0, -1 * move_by_y / main_win->dpi_scale_factor); */
    /* } */
}
    

void track_set_input(Track *track)
{
    Session *session = session_get();

    /* Refresh device list: skip for now :( */
    /* session_populate_midi_device_lists(session_get()); */

    SDL_Rect *rect;
    if (session->piano_roll) {
	rect = piano_roll_get_device_name_rect();
    } else {
	rect = &(track->tb_input_name->layout->rect);
    }
    /* int y = rect->y; */
    Menu *menu = menu_create_at_point(rect->x, rect->y);
    MenuColumn *c = menu_column_add(menu, "");
    MenuSection *sc = menu_section_add(c, "");
    /* Project *proj = &session_get()->proj; */
    /* Session *session = session_get(); */
    for (int i=0; i<session->audio_io.num_record_conns; i++) {
	struct track_in_arg *arg = malloc(sizeof(struct track_in_arg));
	AudioConn *conn = session->audio_io.record_conns[i];
	arg->track = track;
	arg->obj = conn;
	arg->type = AUDIO_CONN;
	if (conn->available) {
	    MenuItem *item = menu_item_add(
		sc,
		conn->name,
		"(Audio)",
		track_set_in_onclick,
		arg);
	    item->free_target_on_destroy = true;
	}
    }
    for (int i=0; i<session->midi_io.num_inputs; i++) {
	MIDIDevice *device = session->midi_io.inputs + i;
	struct track_in_arg *arg = malloc(sizeof(struct track_in_arg));
	arg->type = MIDI_DEVICE;
	arg->obj = device;
	arg->track = track;
	MenuItem *item = menu_item_add(
	    sc,
	    device->name,
	    "(MIDI)",
	    track_set_in_onclick,
	    arg);
	item->free_target_on_destroy = true;
	
    }
    menu_add_header(menu,"", "Select track input");
    window_add_menu(main_win, menu);
    /* int move_by_y = 0; */
    /* if ((move_by_y = y + menu->layout->rect.h - main_win->layout->rect.h) > 0) { */
    /* 	menu_translate(menu, 0, -1 * move_by_y / main_win->dpi_scale_factor); */
    /* } */
}

void timeline_force_stop_midi_monitoring()
{
    log_tmp(LOG_DEBUG, "Forcing stop midi monitoring...\n");
    Session *session = session_get();
    if (!session->midi_io.monitoring) return;
    Synth *synth = session->midi_io.monitor_synth;
    if (!synth) return;
    session->midi_io.monitor_synth = NULL;
    session->midi_io.monitor_device = NULL;
    /* Only close output audio device if project is not playing from timeline */
    if (!session->playback.playing) {
	audioconn_stop_playback(session->audio_io.playback_conn);
    }
    pthread_mutex_lock(&synth->audio_proc_lock);
    synth_close_all_notes(synth);
    api_node_set_owner(&synth->api_node, JDAW_THREAD_DSP);
    pthread_mutex_unlock(&synth->audio_proc_lock);

    session->midi_io.monitoring = false;
}

/* Use on the selected track to set session monitoring info */
bool timeline_check_set_midi_monitoring()
{
    log_tmp(LOG_DEBUG, "Checking/setting midi monitoring...\n");
    Session *session = session_get();
    Timeline *tl = ACTIVE_TL;
    Track *track = timeline_selected_track(tl);
    bool was_monitoring = session->midi_io.monitoring;
    if (track) {
	if (track->midi_out && track->midi_out_type == MIDI_OUT_SYNTH) {
	    char out_device_name[MAX_NAMELENGTH];
	    /* TODO: use actual device names, not literal 'synth' */
	    snprintf(out_device_name, MAX_NAMELENGTH, "%s:%s", track->name, "synth");
	    /* snprintf(session->midi_io.monitor_out_text, MIDI_MONITOR_STRLEN, "%s:%s", track->name, "synth"); */
	    snprintf(session->midi_io.monitor_out_text, MIDI_MONITOR_STRLEN, "%s", track->synth->preset_name);
	    PageEl *el = panel_area_get_el_by_id(session->gui.panels, "midi_monitor_out_name");
	    Textbox *tb = el->component;
	    textbox_reset_full(tb);
	    mqwert_set_monitor_device_name(out_device_name);
	} else {
	    snprintf(session->midi_io.monitor_out_text, MIDI_MONITOR_STRLEN, "%s", "(none)");
	    mqwert_set_monitor_device_name("(none)");
	}
    }

    if (track && /*track->input_type == MIDI_DEVICE && */ track->midi_out && track->midi_out_type == MIDI_OUT_SYNTH) {
	session->midi_io.monitor_synth = track->midi_out;
	if (session->midi_qwerty) {
	    session->midi_io.monitor_device = session->midi_io.midi_qwerty;
	    snprintf(session->midi_io.monitor_in_text, MIDI_MONITOR_STRLEN, "%s", "QWERTY");
	    PageEl *el = panel_area_get_el_by_id(session->gui.panels, "midi_monitor_in_name");
	    Textbox *tb = el->component;
	    textbox_reset_full(tb);

	} else if (track->input_type == MIDI_DEVICE) {
	    session->midi_io.monitor_device = track->input;
	    snprintf(session->midi_io.monitor_in_text, MIDI_MONITOR_STRLEN, "%s", ((MIDIDevice *)track->input)->name);
	    PageEl *el = panel_area_get_el_by_id(session->gui.panels, "midi_monitor_in_name");
	    Textbox *tb = el->component;
	    textbox_reset_full(tb);
	} else {
	    snprintf(session->midi_io.monitor_in_text, MIDI_MONITOR_STRLEN, "%s", "(none)");
	    PageEl *el = panel_area_get_el_by_id(session->gui.panels, "midi_monitor_in_name");
	    Textbox *tb = el->component;
	    textbox_reset_full(tb);

	    goto no_monitor;
	}
	log_tmp(LOG_DEBUG, "\tMonitoring!\n");
	MIDIDevice *d = session->midi_io.monitor_device;

	/* Clear notes in system device buffer */
	Pm_Read(d->stream, d->buffer, PM_EVENT_BUF_NUM_EVENTS);
	d->num_unconsumed_events = 0;

	/* Clear notes in synth if present */
	Synth *synth = session->midi_io.monitor_synth;
	
	pthread_mutex_lock(&synth->audio_proc_lock);
	synth_close_all_notes(synth);
	api_node_set_owner(&track->synth->api_node, JDAW_THREAD_PLAYBACK);
	pthread_mutex_unlock(&synth->audio_proc_lock);
	
	audioconn_start_playback(session->audio_io.playback_conn);
	session->midi_io.monitoring = true;
	if (!was_monitoring) {
	    panel_page_refocus(session->gui.panels, "MIDI monitoring", 1);
	}
	return true;
    } else {
    no_monitor:
	log_tmp(LOG_DEBUG, "\tNo monitor\n");
	session->midi_io.monitor_synth = NULL;
	session->midi_io.monitor_device = NULL;
	/* Only close output audio device if project is not playing from timeline */
	if (!session->playback.playing) {
	    audioconn_stop_playback(session->audio_io.playback_conn);
	}
	if (track && track->synth) {
	    pthread_mutex_lock(&track->synth->audio_proc_lock);
	    synth_close_all_notes(track->synth);
	    api_node_set_owner(&track->synth->api_node, JDAW_THREAD_DSP);
	    pthread_mutex_unlock(&track->synth->audio_proc_lock);
	    /* api_node_set_owner(&track->synth->api_node, JDAW_THREAD_DSP); */
	}
	session->midi_io.monitoring = false;
	/* fprintf(stderr, "NO Monitor\n"); */
	if (was_monitoring) {
	    panel_page_refocus(session->gui.panels, "MIDI monitoring", 1);
	}

	return false;
    }
}

void track_rename(Track *track)
{
    textentry_edit(track->tb_name);
    /* txt_edit(track->tb_name->text, project_draw); */
    main_win->i_state = 0;
}


static void timeline_remove_track(Track *track)
{
    Timeline *tl = track->tl;
    for (uint16_t i=0; i<track->num_clips; i++) {
	ClipRef *clip = track->clips[i];
	if (clip->grabbed) {
	    timeline_clipref_ungrab(clip);
	    /* clipref_ungrab(clip); */
	}
    }
    Session *session = session_get();
    if (session->dragging) status_stat_drag();
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

static void track_remove_bus_out(Track *track)
{
    if (!track->bus_out) return;
    for (int i=0; i<track->bus_out->num_bus_ins; i++) {
	if (track->bus_out->bus_ins[i] == track) {
	    memmove(track->bus_out->bus_ins + i, track->bus_out->bus_ins + i + 1, (track->bus_out->num_bus_ins - i - 1) * sizeof(Track *));
	    track->bus_out->num_bus_ins--;
	    return;
	}
    }
}

void track_set_bus_out(Track *track, Track *bus_out)
{
    /* Check for cycles */

    if (track->bus_out == bus_out) return;
    else if (track->bus_out) track_remove_bus_out(track);
    
    Track *test = bus_out;
    while (test) {
	if (test == track) {
	    fprintf(stderr, "Error: circular route\n");
	    return;
	}
	test = test->bus_out;
    }

    track->bus_out = bus_out;
    if (bus_out->num_bus_ins + 1 >= bus_out->bus_ins_arrlen) {
	if (bus_out->bus_ins_arrlen == 0) {
	    bus_out->bus_ins_arrlen = 4;
	    bus_out->bus_ins = calloc(bus_out->bus_ins_arrlen, sizeof(Track *));
 	} else {
	    bus_out->bus_ins_arrlen *= 2;
	    bus_out->bus_ins = realloc(bus_out->bus_ins, bus_out->bus_ins_arrlen * sizeof(Track *));
	}
    }
    bus_out->bus_ins[bus_out->num_bus_ins] = track;
    bus_out->num_bus_ins++;
	
    for (int i=0; i<bus_out->num_bus_ins; i++) {
	fprintf(stderr, "%d Track \"%s\" has bus in: \"%s\"\n", i, bus_out->name, bus_out->bus_ins[i]->name);
    }
}

void track_delete(Track *track)
{
    track->deleted = true;
    Timeline *tl = track->tl;
    /* fprintf(stderr, "\n\nBEFORE:\n"); */
    /* api_node_print_all_routes(&tl->api_node); */
    timeline_remove_track(track);
    api_node_deregister(&track->api_node);
    timeline_reset(tl, false);
    timeline_check_set_midi_monitoring();
    /* fprintf(stderr, "\nAFTER:\n"); */
    /* api_node_print_all_routes(&tl->api_node); */

    /* api_node_print_all_routes(&tl->api_node); */
}

void track_undelete(Track *track)
{
    track->deleted = false;
    /* timeline_insert_track_at(track, track->tl_rank); */
    timeline_reinsert_track(track);
    timeline_reset(track->tl, false);
    api_node_reregister(&track->api_node);
    timeline_check_set_midi_monitoring();
    /* fprintf(stderr, "\nAFTER UNDELETE:\n"); */
    /* api_node_print_all_routes(&track->tl->api_node); */

    
}

void track_destroy(Track *track, bool displace)
{
    if (main_win->active_tabview && main_win->active_tabview->connected_obj == track) {
	tabview_close(main_win->active_tabview);
    }
    for (uint16_t i=0; i<track->num_clips; i++) {
	ClipRef *cr = track->clips[i];
	if (cr) {
	    clipref_destroy_no_displace(track->clips[i]);
	}
    }
    for (int i=0; i<track->num_automations; i++) {
	automation_destroy(track->automations[i]);
    }

    effect_chain_deinit(&track->effect_chain);
    /* for (int i=0; i<track->num_effects; i++) { */
    /* 	effect_destroy(track->effects[i]); */
    /* } */

    midi_event_ring_buf_deinit(&track->note_offs);

    
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
    
    /* filter_deinit(&track->fir_filter); */
    /* if (track->delay_line.buf_L) free(track->delay_line.buf_L); */
    /* if (track->delay_line.buf_R) free(track->delay_line.buf_R); */
    /* if (track->delay_line.cpy_buf) free(track->delay_line.cpy_buf); */
    /* if (track->delay_line.lock) SDL_DestroyMutex(track->delay_line.lock); */
    /* pthread_mutex_destroy(&track->delay_line.lock); */

    free(track->clips);
    /* if (track->buf_L_freq_mag) free(track->buf_L_freq_mag); */
    /* if (track->buf_R_freq_mag) free(track->buf_R_freq_mag); */
    if (track->automation_dropdown) symbol_button_destroy(track->automation_dropdown);

    if (track->synth) synth_destroy(track->synth);
    free(track);
}



static void timeline_reinsert(Timeline *tl)
{
    /* Timeline *currently_active = ACTIVE_TL; */
    Project *proj = tl->proj;
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
    Session *session = session_get();
    Timeline *current = ACTIVE_TL;
    current->track_area->hidden = true;

    session->proj.active_tl_index = new_tl_index;
    Timeline *new = session->proj.timelines[new_tl_index];
    new->track_area->hidden = false;

    textbox_set_value_handle(session->gui.timecode_tb, new->timecode.str);
    
    /* /\* Concession to bad design *\/ */
    /* session->gui.audio_rect = &(layout_get_child_by_name_recursive(new->track_area->parent, "audio_rect")->rect); */
    /* session->gui.ruler_rect = &(layout_get_child_by_name_recursive(new->track_area->parent, "ruler")->rect); */
    
    new->needs_redraw = true;
    project_reset_tl_label(new->proj);
}

void timeline_delete(Timeline *tl, bool from_undo)
{
    if (tl->proj->num_timelines == 1) {
	status_set_errstr("Cannot delete sole timeline on project.");
	return;
    }
    Project *proj = &session_get()->proj;
    for (uint8_t i=tl->index; i<proj->num_timelines - 1; i++) {
	proj->timelines[i] = proj->timelines[i+1];
	proj->timelines[i]->index--;
    }
    proj->num_timelines--;
    if (proj->active_tl_index > proj->num_timelines - 1) {
	proj->active_tl_index--;
    }
    timeline_switch(proj->active_tl_index);
    if (!from_undo) {
	Value nullval = {.int_v = 0.0};
        user_event_push(
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

static Dimension cached_frozen_ct_y;
static void check_freeze_click_track(Timeline * tl)
{
    if (tl->click_track_frozen) return;
    ClickTrack *ct = timeline_selected_click_track(tl);
    if (ct) {
	tl->click_track_frozen = true;
	cached_frozen_ct_y = ct->layout->y;
	ct->layout->y.type = REL;
	ct->layout->y.value = 0;
	Layout *new_parent = ct->layout->parent->parent;
	layout_remove_child(ct->layout);
	layout_reparent(ct->layout, new_parent);
	/* ct->rect.y */
	/* tl->track_area->rect.y -= session_get()->gui.ruler_rect->h; */
	/* layout_set_values_from_rect(tl->track_area); */
	layout_reset(tl->track_area);
	tl->layout_selector = -1;
	tl->click_track_selector = 0;
	tl->track_selector = -1;
	tl->needs_redraw = true;
    }
}

bool check_unfreeze_click_track(Timeline *tl)
{
    if (!tl->click_track_frozen) return false;
    ClickTrack *ct = timeline_selected_click_track(tl);
    if (ct) {
	layout_remove_child(ct->layout);
	layout_insert_child_at(ct->layout, tl->track_area, 0);
	ct->layout->y = cached_frozen_ct_y;
	tl->click_track_frozen = false;
	layout_reset(tl->track_area);
	tl->needs_redraw = true;
	tl->layout_selector = 0;
	tl->click_track_selector = 0;
	tl->track_selector = -1;
	return true;	
    }
    return false;
}

void timeline_move_track_or_automation(Timeline *tl, int direction)
{
    
    Track *t = timeline_selected_track(tl);
    if (t && t->num_automations > 0 && t->selected_automation >= 0) {
	track_move_automation(t->automations[t->selected_automation], direction, false);
	return;
    }
    int new_pos = tl->layout_selector + direction;
    if (new_pos >= tl->track_area->num_children) return;
    if (new_pos < 0) {
	check_freeze_click_track(tl);
	return;
    }
    if (tl->layout_selector == -1) {
	if (check_unfreeze_click_track(tl)) return;
    }
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

void project_set_chunk_size(uint16_t new_chunk_size)
{
    Session *session = session_get();
    if (session->playback.recording) {
	transport_stop_recording();
    }
    transport_stop_playback();
    Project *proj = &session_get()->proj;
    proj->chunk_size_sframes = new_chunk_size;
    for (uint8_t i=0; i<session->audio_io.num_record_conns; i++) {
	audioconn_reset_chunk_size(session->audio_io.record_conns[i], new_chunk_size);
    }
    for (uint8_t i=0; i<session->audio_io.num_playback_conns; i++) {
	audioconn_reset_chunk_size(session->audio_io.playback_conns[i], new_chunk_size);
    }
    audioconn_open(session, session->audio_io.playback_conn);
    if (session->gui.freq_domain_plot) {
	waveform_destroy_freq_plot(session->gui.freq_domain_plot);
	session->gui.freq_domain_plot = NULL;
    }

}

static bool refocus_track_lt(Timeline *tl, Layout *lt, Layout *inner, bool at_bottom)
{
    Session *session = session_get();
    int y_diff = inner->rect.y - lt->parent->rect.y;
    int audio_rect_h_by_2 = session->gui.audio_rect->h / 2;
    if (lt->rect.y + lt->rect.h > session->gui.audio_rect->y + session->gui.audio_rect->h || lt->rect.y < session->gui.audio_rect->y) {
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
    Session *session = session_get();
    Timeline *tl = ACTIVE_TL;
    double old_speed = session->playback.play_speed;
    session->playback.play_speed = new_speed;
    
    /* If speed crosses the zero line, need to invalidate direction-dependent caches */
    if (session->playback.play_speed * old_speed < 0.0) {
	timeline_set_play_position(tl, tl->play_pos_sframes, false);
    }
    
    status_stat_playspeed();

}
void timeline_play_speed_mult(double scale_factor)
{
    Session *session = session_get();
    double new_speed = session->playback.play_speed * scale_factor;
    if (fabs(new_speed) > MAX_PLAY_SPEED) {
	timeline_play_speed_set(session->playback.play_speed);
    } else {
	timeline_play_speed_set(new_speed);
    }
}

void timeline_play_speed_adj(double dim)
{
    Session *session = session_get();
    double new_speed = session->playback.play_speed;
    if ((main_win->i_state & I_STATE_CMDCTRL) && (main_win->i_state & I_STATE_META)) {
	new_speed *= dim < -0.5 ? pow(1.0/2.0, 1.0/12.0) : dim > 0.5 ? pow(1.0/2.0, -1.0/12.0) : 1.0;
    } else if (main_win->i_state & I_STATE_CMDCTRL) {
	new_speed += dim * PLAYSPEED_ADJUST_SCALAR_LARGE;
    } else if (main_win->i_state & I_STATE_META) {
	new_speed += dim * PLAYSPEED_ADJUST_SCALAR_TINY;
    } else {
	new_speed += dim * PLAYSPEED_ADJUST_SCALAR_SMALL;
    }
    timeline_play_speed_set(new_speed);
}

void timeline_scroll_playhead(double dim)
{
    Session *session = session_get();
    Timeline *tl = ACTIVE_TL;
    if (main_win->i_state & I_STATE_CMDCTRL) {
	dim *= session_get_sample_rate() * tl->timeview.sample_frames_per_pixel * PLAYHEAD_ADJUST_SCALAR_LARGE;
    } else {
	dim *= session_get_sample_rate() * tl->timeview.sample_frames_per_pixel * PLAYHEAD_ADJUST_SCALAR_SMALL;
    }
    int32_t new_pos = tl->play_pos_sframes + dim;
    timeline_set_play_position(ACTIVE_TL, new_pos, true);
}


void project_active_tl_redraw(Project *proj)
{
    Session *session = session_get();
    ACTIVE_TL->needs_redraw = true;
}

bool track_minimize(Track *t)
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




