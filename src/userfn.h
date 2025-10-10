#ifndef JDAW_GUI_USERFN_H
#define JDAW_GUI_USERFN_H

void user_global_menu(void *nullarg);
void user_global_escape(void *nullarg);
void user_global_quit(void *nullarg);
void user_global_undo(void *nullarg);
void user_global_redo(void *nullarg);
void user_global_show_output_freq_domain(void *nullarg);
void user_global_save_project(void *nullarg);
void user_global_open_file(void *nullarg);
void user_global_start_server(void *nullarg);
void user_global_function_lookup(void *nullarg);
void user_global_start_or_stop_screenrecording(void *nullarg);
void user_global_chaotic_user_test(void *nullarg);
void user_global_api_print_all_routes(void *nullarg);


void user_menu_nav_next_item(void *nullarg);
void user_menu_nav_prev_item(void *nullarg);
void user_menu_nav_next_sctn(void *nullarg);
void user_menu_nav_prev_sctn(void *nullarg);
void user_menu_nav_column_right(void *nullarg);
void user_menu_nav_column_left(void *nullarg);
void user_menu_nav_choose_item(void *nullarg);


void user_menu_translate_up(void *nullarg);
void user_menu_translate_down(void *nullarg);
void user_menu_translate_left(void *nullarg);
void user_menu_translate_right(void *nullarg);
void user_menu_dismiss(void *nullarg);


void user_tl_play(void *nullarg);
void user_tl_play_pause(void *nullarg);
void user_tl_pause(void *nullarg);
void user_tl_rewind(void *nullarg);
void user_tl_play_slow(void *nullarg);
void user_tl_rewind_slow(void *nullarg);
void user_tl_move_playhead_left(void *nullarg);
void user_tl_move_playhead_right(void *nullarg);
void user_tl_move_playhead_left_slow(void *nullarg);
void user_tl_move_playhead_right_slow(void *nullarg);
void user_tl_nudge_left(void *nullarg);
void user_tl_nudge_right(void *nullarg);
void user_tl_small_nudge_left(void *nullarg);
void user_tl_small_nudge_right(void *nullarg);
void user_tl_one_sample_left(void *nullarg);
void user_tl_one_sample_right(void *nullarg);
void user_tl_move_right(void *nullarg);
void user_tl_move_left(void *nullarg);
void user_tl_lock_view_to_playhead(void *nullarg);
void user_tl_zoom_in(void *nullarg);
void user_tl_zoom_out(void *nullarg);
void user_tl_set_mark_out(void *nullarg);
void user_tl_set_mark_in(void *nullarg);
void user_tl_goto_mark_out(void *nullarg);
void user_tl_goto_mark_in(void *nullarg);
void user_tl_goto_zero(void *nullarg);
void user_tl_goto_previous_clip_boundary(void *nullarg);
void user_tl_goto_next_clip_boundary(void *nullarg);

void user_tl_goto_next_beat(void *nullarg);
void user_tl_goto_prev_beat(void *nullarg);
void user_tl_goto_next_subdiv(void *nullarg);
void user_tl_goto_prev_subdiv(void *nullarg);
void user_tl_goto_next_measure(void *nullarg);
void user_tl_goto_prev_measure(void *nullarg);

void user_tl_bring_rear_clip_to_front(void *nullarg);

void user_tl_toggle_loop_playback(void *nullarg);
void user_tl_bring_rear_clip_to_front(void *nullarg);
/* void user_tl_play_drag(void *nullarg); */
/* void user_tl_rewind_drag(void *nullarg); */
/* void user_tl_pause_drag(void *nullarg); */
void user_tl_toggle_drag(void *nullarg);
void user_tl_cut_clipref(void *nullarg);
void user_tl_cut_clipref_and_grab_edges(void *nullarg);
void user_tl_set_default_out(void *nullarg);
void user_tl_add_track(void *nullarg);

void user_tl_track_select_1(void *nullarg);
void user_tl_track_select_2(void *nullarg);
void user_tl_track_select_3(void *nullarg);
void user_tl_track_select_4(void *nullarg);
void user_tl_track_select_5(void *nullarg);
void user_tl_track_select_6(void *nullarg);
void user_tl_track_select_7(void *nullarg);
void user_tl_track_select_8(void *nullarg);
void user_tl_track_select_9(void *nullarg);
void user_tl_track_activate_all(void *nullarg);
void user_tl_track_selector_up(void *nullarg);
void user_tl_track_selector_down(void *nullarg);
void user_tl_move_track_up(void *nullarg);
void user_tl_move_track_down(void *nullarg);
void user_tl_tracks_minimize(void *nullarg);
void user_tl_track_activate_selected(void *nullarg);
void user_tl_track_rename(void *nullarg);
void user_tl_track_toggle_in(void *nullarg);
void user_tl_track_set_in(void *nullarg);
void user_tl_track_set_midi_out(void *nullarg);
void user_tl_track_add_filter(void *nullarg);
/* void user_tl_track_destroy(void *nullarg); */
void user_tl_track_delete(void *nullarg);

void user_tl_click_track_add(void *nullarg);
void user_tl_click_track_cut(void *nullarg);
void user_tl_split_stereo_clipref(void *nullarg);
void user_tl_click_track_set_tempo(void *nullarg);


void user_tl_track_show_hide_automations(void *nullarg);
void user_tl_track_add_automation(void *nullarg);
void user_tl_track_automation_toggle_read(void *nullarg);


void user_tl_track_open_settings(void *nullarg);
void user_tl_track_add_effect(void *nullarg);
void user_tl_track_open_synth(void *nullarg);
void user_tl_mute(void *nullarg);
void user_tl_solo(void *nullarg);
void user_tl_track_vol_up(void *nullarg);
/* void user_tl_track_vol_up_toggle(void *nullarg); */
void user_tl_track_vol_down(void *nullarg);

void user_tl_track_pan_left(void *nullarg);
void user_tl_track_pan_right(void *nullarg);


void user_tl_record(void *nullarg);
void user_tl_clipref_grab_ungrab(void *nullarg);
void user_tl_clipref_grab_left_edge(void *nullarg);
void user_tl_clipref_grab_right_edge(void *nullarg);
void user_tl_clipref_ungrab_edge(void *nullarg);
void user_tl_grab_marked_range(void *nullarg);
void user_tl_grab_marked_range_left_edge(void *nullarg);
void user_tl_grab_marked_range_right_edge(void *nullarg);
void user_tl_copy_grabbed_clips(void *nullarg);
void user_tl_paste_grabbed_clips(void *nullarg);
/* Deprecated; use user_tl_cliprefs_delete */
/* void user_tl_cliprefs_destroy(void *nullarg); */
void user_tl_rename_clip_at_cursor(void *nullarg);
void user_tl_delete_generic(void *nullarg);

void user_tl_load_clip_at_cursor_to_src(void *nullarg);
void user_tl_edit_clip_at_cursor(void *nullarg);
void user_tl_activate_source_mode(void *nullarg);
void user_tl_drop_from_source(void *nullarg);

void user_tl_drop_saved1_from_source(void *nullarg);
void user_tl_drop_saved2_from_source(void *nullarg);
void user_tl_drop_saved3_from_source(void *nullarg);
void user_tl_drop_saved4_from_source(void *nullarg);

void user_tl_add_new_timeline(void *nullarg);
void user_tl_previous_timeline(void *nullarg);
void user_tl_next_timeline(void *nullarg);
void user_tl_delete_timeline(void *nullarg);

void user_tl_write_mixdown_to_wav(void *nullarg);

void user_tl_activate_mqwert(void *nullarg);
void user_tl_insert_jlily(void *nullarg);

void user_source_play(void *nullarg);
void user_source_pause(void *nullarg);
void user_source_rewind(void *nullarg);
void user_source_play_slow(void *nullarg);
void user_source_rewind_slow(void *nullarg);
void user_source_set_in_mark(void *nullarg);
void user_source_set_out_mark(void *nullarg);
void user_source_zoom_in(void *nullarg);
void user_source_zoom_out(void *nullarg);
void user_source_move_left(void *nullarg);
void user_source_move_right(void *nullarg);

void user_modal_next(void *nullarg);
void user_modal_previous(void *nullarg);
void user_modal_next_escape(void *nullarg);
void user_modal_previous_escape(void *nullarg);
void user_modal_select(void *nullarg);
void user_modal_dismiss(void *nullarg);
void user_modal_submit_form(void *nullarg);


void user_text_edit_escape(void *nullarg);
void user_text_edit_full_escape(void *nullarg);
void user_text_edit_backspace(void *nullarg);
void user_text_edit_cursor_right(void *nullarg);
void user_text_edit_cursor_left(void *nullarg);
void user_text_edit_select_all(void *nullarg);


void user_tabview_next_escape(void *nullarg);
void user_tabview_previous_escape(void *nullarg);
void user_tabview_enter(void *nullarg);
void user_tabview_left(void *nullarg);
void user_tabview_right(void *nullarg);
void user_tabview_next_tab(void *nullarg);
void user_tabview_previous_tab(void *nullarg);
void user_tabview_move_current_tab_left(void *nullarg);
void user_tabview_move_current_tab_right(void *nullarg);
    
void user_tabview_escape(void *nullarg);

void user_autocomplete_next(void *nullarg);
void user_autocomplete_previous(void *nullarg);
void user_autocomplete_select(void *nullarg);
void user_autocomplete_escape(void *nullarg);

void user_midi_qwerty_escape(void *nullarg);
void user_midi_qwerty_octave_up(void *nullarg);
void user_midi_qwerty_octave_down(void *nullarg);
void user_midi_qwerty_transpose_up(void *nullarg);
void user_midi_qwerty_transpose_down(void *nullarg);
void user_midi_qwerty_velocity_up(void *nullarg);
void user_midi_qwerty_velocity_down(void *nullarg);
void user_midi_qwerty_c1(void *nullarg);
void user_midi_qwerty_cis1(void *nullarg);
void user_midi_qwerty_d1(void *nullarg);
void user_midi_qwerty_dis(void *nullarg);
void user_midi_qwerty_e(void *nullarg);
void user_midi_qwerty_f(void *nullarg);
void user_midi_qwerty_fis(void *nullarg);
void user_midi_qwerty_g(void *nullarg);
void user_midi_qwerty_gis(void *nullarg);
void user_midi_qwerty_a(void *nullarg);
void user_midi_qwerty_ais(void *nullarg);
void user_midi_qwerty_b(void *nullarg);
void user_midi_qwerty_c2(void *nullarg);
void user_midi_qwerty_cis2(void *nullarg);
void user_midi_qwerty_d2(void *nullarg);
void user_midi_qwerty_dis2(void *nullarg);
void user_midi_qwerty_e2(void *nullarg);
void user_midi_qwerty_f2(void *nullarg);

void user_piano_roll_escape(void *nullarg);
void user_piano_roll_zoom_in(void *nullarg);
void user_piano_roll_zoom_out(void *nullarg);
void user_piano_roll_move_left(void *nullarg);
void user_piano_roll_move_right(void *nullarg);
void user_piano_roll_note_up(void *nullarg);
void user_piano_roll_note_down(void *nullarg);
void user_piano_roll_next_note(void *nullarg);
void user_piano_roll_prev_note(void *nullarg);
void user_piano_roll_up_note(void *nullarg);
void user_piano_roll_down_note(void *nullarg);
void user_piano_roll_dur_shorter(void *nullarg);
void user_piano_roll_dur_longer(void *nullarg);
void user_piano_roll_insert_note(void *nullarg);
void user_piano_roll_insert_rest(void *nullarg);
void user_piano_roll_grab_ungrab(void *nullarg);
void user_piano_roll_grab_note_left_edge(void *nullarg);
void user_piano_roll_grab_note_right_edge(void *nullarg);
void user_piano_roll_grab_marked_range(void *nullarg);

void user_piano_roll_toggle_tie(void *nullarg);
void user_piano_roll_toggle_chord_mode(void *nullarg);

#endif
