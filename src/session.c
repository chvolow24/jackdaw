#include "layout_xml.h"
#include "session.h"
#include "transport.h"
/* #include "window.h" */

static Session *session = NULL;
extern Window *main_win;

extern SDL_Color color_global_clear;

Session *session_get()
{
    return session;
}


Session *session_create()
{
    session = calloc(1, sizeof(Session));
    window_set_layout(main_win, layout_create_from_window(main_win));
    layout_read_xml_to_lt(main_win->layout, MAIN_LT_PATH);

    session->gui.layout = main_win->layout;
    session->gui.audio_rect = &(layout_get_child_by_name_recursive(session->gui.layout, "audio_rect")->rect);
    session->gui.console_column_rect = &(layout_get_child_by_name_recursive(session->gui.layout, "console_column")->rect);


    Layout *control_bar_layout = layout_get_child_by_name_recursive(session->gui.layout, "control_bar");

    static const SDL_Color timeline_label_txt_color = {0, 200, 100, 255};
    /* sessionect_init_quickref(session, control_bar_layout); */
    session->gui.control_bar_rect = &control_bar_layout->rect;
    session->gui.ruler_rect = &(layout_get_child_by_name_recursive(session->gui.layout, "ruler")->rect);
    Layout *timeline_label_lt = layout_get_child_by_name_recursive(session->gui.layout, "timeline_label");
    strcpy(session->gui.timeline_label_str, "Timeline 1: \"Main\"");
    session->gui.timeline_label = textbox_create_from_str(
	session->gui.timeline_label_str,
	timeline_label_lt,
	main_win->bold_font,
	12,
	main_win
	);
    textbox_set_text_color(session->gui.timeline_label, &timeline_label_txt_color);
    textbox_set_background_color(session->gui.timeline_label, &color_global_clear);
    textbox_set_align(session->timeline_label, CENTER_LEFT);

    return session;
}

void session_destroy()
{
    if (!session) {
	fprintf(stderr, "Error: no session to destroy\n");
	return;
    }
    if (session->playback.recording) {
	transport_stop_recording();
    } else {
	transport_stop_playback();
    }

    /* api_quit(proj); */
    user_event_history_destroy(&session->history);

    for (uint8_t i=0; i<session->audio_io.num_record_conns; i++) {
	audioconn_destroy(session->audio_io.record_conns[i]);
    }
    for (uint8_t i=0; i<session->audio_io.num_playback_conns; i++) {
	audioconn_destroy(session->audio_io.playback_conns[i]);
    }

    free(session->output_L);
    free(session->output_R);
    free(session->output_L_freq);
    free(session->output_R_freq);

    textbox_destroy(session->gui.timeline_label);
    panel_area_destroy(session->gui.panels);

        
    if (session->status_bar.call) textbox_destroy(session->status_bar.call);
    if (session->status_bar.dragstat) textbox_destroy(session->status_bar.dragstat);
    if (session->status_bar.error) textbox_destroy(session->status_bar.error);

    session_destroy_animations(session);
    session_destroy_metronomes(session);
    session_loading_screen_deinit(session);
    session_deinit_midi(session);

}
