/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

#include "color.h"
#include "layout.h"
/* #include "project.h" */
#include "session.h"

#define ERR_TIMER_MAX 100
#define CALL_TIMER_MAX 100
#define STAT_TIMER_MAX 50

extern struct colors colors;

void status_frame()
{
    Session *session = session_get();
    if (session->status_bar.stat_timer > 0) {
	session->status_bar.stat_timer--;
	return;
    }
    
    uint8_t *alpha = &(session->status_bar.error->text->color.a);
    if (session->status_bar.err_timer > 0) {
	session->status_bar.err_timer--;
    } else if (*alpha > 0) {
	txt_reset_drawable(session->status_bar.error->text);
	(*alpha)--;
	/* The alpha decrement appears to NEED to come after reset drawable.
	   This makes no sense. */
    }

    alpha = &(session->status_bar.call->text->color.a);
    if (session->status_bar.call_timer > 0) {
	session->status_bar.call_timer--;
    } else if (*alpha > 0) {
	txt_reset_drawable(session->status_bar.call->text);
	(*alpha)--;
    }
}

void status_set_errstr(const char *errstr)
{
    Session *session = session_get();
    strcpy(session->status_bar.errstr, errstr);
    /* textbox_set_text_color(session->status_bar.error, &colors.red); */
    textbox_size_to_fit(session->status_bar.error, 0, 0);
    textbox_set_text_color(session->status_bar.error, &colors.red);
    layout_center_agnostic(session->status_bar.error->layout, false, true);

    textbox_reset_full(session->status_bar.error);
    session->status_bar.err_timer = ERR_TIMER_MAX;
    session->status_bar.error->text->color.a = 255;
}

void status_set_undostr(const char *undostr)
{
    Session *session = session_get();
    strcpy(session->status_bar.errstr, undostr);
    /* textbox_set_text_color(session->status_bar.error, &colors.red); */
    textbox_size_to_fit(session->status_bar.error, 0, 0);
    textbox_set_text_color(session->status_bar.error, &colors.undo_purple);
    layout_center_agnostic(session->status_bar.error->layout, false, true);
    textbox_reset_full(session->status_bar.error);
    session->status_bar.err_timer = ERR_TIMER_MAX;
    session->status_bar.error->text->color.a = 255;
}

void status_set_statstr(const char *statstr)
{
    Session *session = session_get();
    strcpy(session->status_bar.callstr, statstr);
    textbox_set_text_color(session->status_bar.call, &colors.white);
    /* textbox_size_to_status);fit(session->status_bar.callor, 0, 0); */
    textbox_reset_full(session->status_bar.call);
    session->status_bar.stat_timer = STAT_TIMER_MAX;
    session->status_bar.call->text->color.a = 255;

}

void status_set_callstr(const char *callstr)
{
    Session *session = session_get();
    /* if (session->status_bar.stat_timer > 0) { */
    /* 	return; */
    /* } */
    layout_force_reset(session->status_bar.layout);
    strcpy(session->status_bar.callstr, callstr);
    textbox_size_to_fit(session->status_bar.call, 0, 0);
    textbox_set_text_color(session->status_bar.call, &colors.light_grey);
    layout_center_agnostic(session->status_bar.call->layout, false, true);
    /* textbox_set_pad(session->status_bar.call, 2, 2); */
    textbox_reset_full(session->status_bar.call);
    session->status_bar.call_timer = CALL_TIMER_MAX;
    session->status_bar.call->text->color.a = 255;
    /* layout_size_to_fit_children(session->status_bar.call->layout, false, 0); */
    /* layout_reset(session->status_bar.layout); */
}

void status_cat_callstr(const char *catstr)
{
    /* if (session->status_bar.stat_timer > 0) { */
    /* 	return; */
    /* } */
    Session *session = session_get();
    strcat(session->status_bar.callstr, catstr);
    textbox_size_to_fit(session->status_bar.call, 0, 0);
    layout_center_agnostic(session->status_bar.call->layout, false, true);
    textbox_reset_full(session->status_bar.call);

    session->status_bar.call_timer = CALL_TIMER_MAX;
    session->status_bar.call->text->color.a = 255;
    /* layout_size_to_fit_children(session->status_bar.call->layout, false, 0); */
    /* layout_force_reset(session->status_bar.layout); */
}

static void status_set_dragstr(char *dragstr)
{
    Session *session = session_get();
    strcpy(session->status_bar.dragstr, dragstr);
    textbox_set_text_color(session->status_bar.dragstat, &colors.green);

    textbox_size_to_fit(session->status_bar.dragstat, 0, 0);
    layout_center_agnostic(session->status_bar.dragstat->layout, false, true);
    /* layout_center_agnostic(session->status_bar.dragstat->layout, false, true); */
    textbox_reset_full(session->status_bar.dragstat);
    layout_reset(session->status_bar.layout);
    /* fprintf(stdout, "w: %d\n", session->status_bar.dragstat->layout->rect.w); */
}

void status_set_alert_str(char *alert_str)
{
    Session *session = session_get();
    if (!alert_str) {
        session->status_bar.dragstat->layout->w.value = 0.0f;
	layout_reset(session->status_bar.layout);
	status_stat_drag();
	return;
    }
    strcpy(session->status_bar.dragstr, alert_str);
    textbox_set_text_color(session->status_bar.dragstat, &colors.alert_orange);
    textbox_reset_full(session->status_bar.dragstat);
    textbox_size_to_fit(session->status_bar.dragstat, 0, 0);
    layout_center_agnostic(session->status_bar.dragstat->layout, false, true);
    /* textbox_reset_full(session->status_bar.dragstat, 0, ); */
    textbox_reset_full(session->status_bar.dragstat);
    layout_reset(session->status_bar.layout);
}


void status_stat_playspeed()
{
    Session *session = session_get();
    /* fprintf(stdout, "stat playspeed\n"); */
    char buf[64];
    snprintf(buf, sizeof(buf), "Play speed: %0.3f",session->playback.play_speed);
    status_set_statstr(buf);
}

void status_stat_drag()
{
    Session *session = session_get();
    if (!session->dragging) {
        session->status_bar.dragstat->layout->w.value = 0.0f;
	layout_reset(session->status_bar.layout);
	return;
    }
    Timeline *tl = ACTIVE_TL;
    char buf[64];
    snprintf(buf, sizeof(buf), "Dragging %d clips", tl->num_grabbed_clips);
    status_set_dragstr(buf);
}
