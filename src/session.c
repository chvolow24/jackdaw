/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

#include "assets.h"
#include "audio_connection.h"
#include "color.h"
#include "consts.h"
#include "endpoint.h"
#include "endpoint_callbacks.h"
#include "init_panels.h"
#include "label.h"
#include "layout.h"
#include "layout_xml.h"
#include "session.h"
#include "transport.h"
/* #include "window.h" */

#define STATUS_BAR_H (14 * main_win->dpi_scale_factor)
#define STATUS_BAR_H_PAD (10 * main_win->dpi_scale_factor);

static Session *session = NULL;
extern Window *main_win;

extern struct colors colors;

Session *session_get()
{
    return session;
}

static void session_init_hamburger(Session *session);
static void session_init_status_bar(Session *session);
static void session_init_source_mode(Session *session);

Session *session_create()
{
    session = calloc(1, sizeof(Session));
    window_set_layout(main_win, layout_create_from_window(main_win));
    layout_read_xml_to_lt(main_win->layout, MAIN_LT_PATH);

    session->gui.layout = main_win->layout;
    session->gui.timeline_lt = layout_get_child_by_name_recursive(session->gui.layout, "timeline");
    session->gui.audio_rect = &(layout_get_child_by_name_recursive(session->gui.layout, "audio_rect")->rect);
    session->gui.console_column_rect = &(layout_get_child_by_name_recursive(session->gui.layout, "console_column")->rect);

    Layout *control_bar_layout = layout_get_child_by_name_recursive(session->gui.layout, "control_bar");

    static const SDL_Color timeline_label_txt_color = {0, 200, 100, 255};
    /* sessionect_init_quickref(session, control_bar_layout); */
    session->gui.control_bar_rect = &control_bar_layout->rect;
    session->gui.ruler_rect = &(layout_get_child_by_name_recursive(session->gui.timeline_lt, "ruler")->rect);
    Layout *timeline_label_lt = layout_get_child_by_name_recursive(session->gui.layout, "timeline_label");
    /* strcpy(session->gui.timeline_label_str, "Timeline 1: \"Main\""); */
    session->gui.timeline_label = textbox_create_from_str(
	session->gui.timeline_label_str,
	timeline_label_lt,
	main_win->bold_font,
	12,
	main_win
	);
    /* session->gui.timecode_tb = textbox_create_from_str( */
    /* 	NULL, */
    /* 	tc_lt, */
    /* 	main_win->mono_bold_font, */
    /* 	16, */
    /* 	main_win); */

    textbox_set_text_color(session->gui.timeline_label, &timeline_label_txt_color);
    textbox_set_background_color(session->gui.timeline_label, &colors.clear);
    textbox_set_align(session->gui.timeline_label, CENTER_LEFT);

    session->gui.right_arrow_texture = txt_create_texture("→", colors.white, main_win->mono_bold_font, 12, NULL, NULL);
    session->gui.left_arrow_texture = txt_create_texture("←", colors.white, main_win->mono_bold_font, 12, NULL, NULL);

    session_init_source_mode(session);
    session_init_audio_conns(session);
    session_init_midi(session);
    session_init_metronomes(session);
    
    /* Init panels after proj initialized ! */
    /* session_init_panels(session); */

    session_init_hamburger(session);

    int err;

    if ((err = pthread_mutex_init(&session->queued_ops.queued_val_changes_lock, NULL)) != 0) {
	fprintf(stderr, "Error initializing queued val changes mutex: %s\n", strerror(err));
	exit(1);
    }
    if ((err = pthread_mutex_init(&session->queued_ops.queued_callback_lock, NULL)) != 0) {
	fprintf(stderr, "Error initializing queued callback mutex: %s\n", strerror(err));
	exit(1);
    }
    if ((err = pthread_mutex_init(&session->queued_ops.ongoing_changes_lock, NULL)) != 0) {
	fprintf(stderr, "Error initializing ongoing changes mutex: %s\n", strerror(err));
	exit(1);
    }
    if ((err = pthread_mutex_init(&session->queued_ops.queued_audio_buf_lock, NULL)) != 0) {
	fprintf(stderr, "Error initializing queued audio buf mutex: %s\n", strerror(err));
	exit(1);
    }
    
    endpoint_init(
	&session->playback.play_speed_ep,
	&session->playback.play_speed,
	JDAW_FLOAT,
	"play_speed",
	"Play speed",
	JDAW_THREAD_MAIN,
	play_speed_gui_cb, NULL, NULL,
	NULL, NULL, NULL, NULL);
    
    api_endpoint_register(&session->playback.play_speed_ep, &session->server.api_root);
    session_init_status_bar(session);

    session->playback.output_vol = 1.0f;
    endpoint_init(
	&session->playback.output_vol_ep,
	&session->playback.output_vol,
	JDAW_FLOAT,
	"output_vol",
	"Output vol",
	JDAW_THREAD_PLAYBACK,
	page_el_gui_cb, NULL, NULL, /* page el args initialized in panel */
	NULL, NULL, NULL, NULL);
    endpoint_set_allowed_range(&session->playback.output_vol_ep, (Value){.float_v = 0.0f}, (Value){.float_v = 1.0f});
    endpoint_set_default_value(&session->playback.output_vol_ep, (Value){.float_v = 1.0f});
    endpoint_set_label_fn(&session->playback.output_vol_ep, label_amp_to_dbstr);
    api_endpoint_register(&session->playback.output_vol_ep, &session->server.api_root);
    session_init_status_bar(session);

	
    
    return session;
}

static void session_init_source_mode(Session *session)
{
    session->source_mode.timeview.play_pos = &session->source_mode.src_play_pos_sframes;
    session->source_mode.timeview.in_mark = &session->source_mode.src_in_sframes;
    session->source_mode.timeview.out_mark = &session->source_mode.src_out_sframes;
}

static void session_init_hamburger(Session *session)
{
    Layout *hamburger_lt = layout_get_child_by_name_recursive(session->gui.layout, "hamburger");

    session->gui.hamburger = &hamburger_lt->rect;
    session->gui.bun_patty_bun[0] = &hamburger_lt->children[0]->children[0]->rect;
    session->gui.bun_patty_bun[1] = &hamburger_lt->children[1]->children[0]->rect;
    session->gui.bun_patty_bun[2] = &hamburger_lt->children[2]->children[0]->rect;
}


static void session_init_status_bar(Session *session)
{

    pthread_mutex_init(&session->status_bar.errstr_lock, NULL);
    Layout *status_bar_lt = layout_get_child_by_name_recursive(session->gui.layout, "status_bar");
    Layout *draglt = layout_add_child(status_bar_lt);
    Layout *calllt = layout_add_child(status_bar_lt);
    Layout *errlt = layout_add_child(status_bar_lt);
    session->status_bar.layout = status_bar_lt;

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

    session->status_bar.call = textbox_create_from_str(session->status_bar.callstr, calllt, main_win->mono_bold_font, 14, main_win);
    textbox_set_trunc(session->status_bar.call, false);
    textbox_set_text_color(session->status_bar.call, &colors.light_grey);
    textbox_set_background_color(session->status_bar.call, &colors.clear);
    textbox_set_align(session->status_bar.call, CENTER_LEFT);

    session->status_bar.dragstat = textbox_create_from_str(session->status_bar.dragstr, draglt, main_win->mono_bold_font, 14, main_win);
    textbox_set_trunc(session->status_bar.dragstat, false);
    textbox_set_text_color(session->status_bar.dragstat, &colors.green);
    textbox_set_background_color(session->status_bar.dragstat, &colors.clear);
    textbox_set_align(session->status_bar.dragstat, CENTER_LEFT);
    
    session->status_bar.error = textbox_create_from_str(session->status_bar.errstr, errlt, main_win->mono_bold_font, 14, main_win);
    textbox_set_trunc(session->status_bar.error, false);
    textbox_set_text_color(session->status_bar.error, &colors.red);
    textbox_set_background_color(session->status_bar.error, &colors.clear);
    textbox_set_align(session->status_bar.error, CENTER_LEFT);
    /* textbox_size_to_fit(session->status_bar.call, 0, 0); */
    /* textbox_size_to_fit(session->status_bar.error, 0, 0); */
    textbox_reset_full(session->status_bar.error);
    textbox_reset_full(session->status_bar.dragstat);
    textbox_reset_full(session->status_bar.call);

}

static void session_deinit_status_bar(Session *session)
{
    pthread_mutex_destroy(&session->status_bar.errstr_lock);
    if (session->status_bar.call) textbox_destroy(session->status_bar.call);
    if (session->status_bar.dragstat) textbox_destroy(session->status_bar.dragstat);
    if (session->status_bar.error) textbox_destroy(session->status_bar.error);

}

void session_destroy()
{
    if (!session) {
	fprintf(stderr, "Error: no session to destroy\n");
	return;
    }
    window_clear_higher_modes(main_win, MODE_GLOBAL);
    if (session->playback.recording) {
	transport_stop_recording();
    } else {
	transport_stop_playback();
    }

    /* api_quit(proj); */

    for (int i=0; i<session->audio_io.num_record_conns; i++) {
	audioconn_destroy(session->audio_io.record_conns[i]);
    }
    for (int i=0; i<session->audio_io.num_playback_conns; i++) {
	audioconn_destroy(session->audio_io.playback_conns[i]);
    }
    for (int i=0; i<session->audio_io.num_record_devices; i++) {
	audio_device_destroy(session->audio_io.record_devices[i]);
    }
    for (int i=0; i<session->audio_io.num_playback_devices; i++) {
	audio_device_destroy(session->audio_io.playback_devices[i]);
    }    


    textbox_destroy(session->gui.timeline_label);
    textbox_destroy(session->gui.timecode_tb);
    textbox_destroy(session->gui.loop_play_lemniscate);

    SDL_DestroyTexture(session->gui.right_arrow_texture);
    SDL_DestroyTexture(session->gui.left_arrow_texture);
    
    session_deinit_panels(session);
    session_deinit_status_bar(session);

    session_destroy_animations(session);
    session_destroy_metronomes(session);
    session_loading_screen_deinit();
    session_deinit_midi(session);


    user_event_history_clear(&session->history);
    if (session->proj_initialized) {
	project_deinit(&session->proj);
    }
    
    session = NULL;

}

void session_set_proj(Session *session, Project *new_proj)
{
    if (session->playback.recording) {
	transport_stop_recording();
    } else {
	transport_stop_playback();
    }
    bool reopen_playback_conn;
    if (session->audio_io.playback_conn->open) {
	audioconn_close(session->audio_io.playback_conn);
	reopen_playback_conn = true;
    }

    session_clear_all_queues();
    
    /* Call must occur before de-init project */
    user_event_history_clear(&session->history);
    
    project_deinit(&session->proj);
    session_deinit_panels(session);
    memcpy(&session->proj, new_proj, sizeof(Project));
    for (int i=0; i<session->proj.num_timelines; i++) {
	session->proj.timelines[i]->proj = &session->proj;
    }
    session_init_panels(session);
    layout_force_reset(session->gui.layout);
    timeline_switch(0);
    timeline_reset_full(session->proj.timelines[0]);
    if (reopen_playback_conn) {
	audioconn_open(session, session->audio_io.playback_conn);
    }
}

uint32_t session_get_sample_rate()
{
    uint32_t ret;
    if (session->proj_reading) {
	ret =  session->proj_reading->sample_rate;
    } else if (session->proj_initialized) {
	ret = session->proj.sample_rate;
    } else {
	#ifdef TESTBUILD
	fprintf(stderr, "ERROR: falling back to default sample rate\n");
	breakfn();
	#endif
	ret = DEFAULT_SAMPLE_RATE;
    }
    if (ret <= 0) {
	fprintf(stderr, "ERROR: sample rate is %d\n", ret);
	breakfn();
    }
    return ret;
}

/* Call from any thread to queue audio data for immediate or delayed playback */
void session_queue_audio(int channels, float *c1, float *c2, int32_t len, int32_t delay, bool free_when_done)
{
    int err;
    if ((err = pthread_mutex_lock(&session->queued_ops.queued_audio_buf_lock)) != 0) {
	fprintf(stderr, "Error locking queued audio buf lock (in session_queue_audio): %s\n", strerror(err));
    }

    Session *session = session_get();
    if (session->queued_ops.num_queued_audio_bufs == MAX_QUEUED_BUFS) goto unlock_and_exit;
    
    QueuedBuf *qb = session->queued_ops.queued_audio_bufs + session->queued_ops.num_queued_audio_bufs;
    qb->channels = channels;
    qb->buf[0] = c1;
    qb->buf[1] = c2;
    qb->len_sframes = len;
    qb->play_index = 0;
    qb->play_after_sframes = delay;
    qb->free_when_done = free_when_done;
    session->queued_ops.num_queued_audio_bufs++;
    if (!session->audio_io.playback_conn->playing) {
	audioconn_start_playback(session->audio_io.playback_conn);
    }
unlock_and_exit:
    if ((err = pthread_mutex_unlock(&session->queued_ops.queued_audio_buf_lock)) != 0) {
	fprintf(stderr, "Error unlocking queued audio buf lock (in session_queue_audio): %s\n", strerror(err));
    }

}
void session_flush_callbacks(Session *session, enum jdaw_thread thread);
void session_flush_ongoing_changes(Session *session, enum jdaw_thread thread);

#define check_queued_ops_lock(name) \
    if ((err = pthread_mutex_lock(&session->queued_ops.name))) {	\
	fprintf(stderr, "Error locking " #name " (in session_clear_all_queues)\n");\
    } \

#define check_queued_ops_unlock(name) \
    if ((err = pthread_mutex_unlock(&session->queued_ops.name))) {	\
	fprintf(stderr, "Error locking " #name " (in session_clear_all_queues)\n");\
    } \


void session_clear_all_queues()
{
    /* Clear audio bufs */
    int err;
    check_queued_ops_lock(queued_audio_buf_lock);
    for (int i=0; i<session->queued_ops.num_queued_audio_bufs; i++) {
        QueuedBuf *qb = session->queued_ops.queued_audio_bufs + i;
	if (qb->free_when_done) {
	    free(qb->buf[0]);
	    if (qb->channels > 1) free(qb->buf[1]);
	}
    }
    session->queued_ops.num_queued_audio_bufs = 0;
    check_queued_ops_unlock(queued_audio_buf_lock);


    /* Clear ongoing changes */
    /* check_queued_ops_lock(ongoing_changes_lock); */
    for (int i=0; i<NUM_JDAW_THREADS; i++) {
	session_flush_ongoing_changes(session, i);
    }
    /* check_queued_ops_unlock(ongoing_changes_lock); */


    /* Clear callbacks */
    /* check_queued_ops_lock(queued_callback_lock); */
    for (int i=0; i<NUM_JDAW_THREADS; i++) {
	session_flush_callbacks(session, i);
    }
    /* check_queued_ops_unlock(queued_callback_lock); */
}




