#ifndef JDAW_GUI_USERFN_H
#define JDAW_GUI_USERFN_H


void user_global_menu(void *nullarg);
void user_global_quit(void *nullarg);
void user_global_undo(void *nullarg);
void user_global_redo(void *nullarg);
void user_global_show_output_freq_domain(void *nullarg);
void user_global_save_project(void *nullarg);
void user_global_open_file(void *nullarg);
void user_global_start_or_stop_screenrecording(void *nullarg);



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
void user_tl_pause(void *nullarg);
void user_tl_rewind(void *nullarg);
void user_tl_play_slow(void *nullarg);
void user_tl_rewind_slow(void *nullarg);
void user_tl_move_right(void *nullarg);
void user_tl_move_left(void *nullarg);
void user_tl_zoom_in(void *nullarg);
void user_tl_zoom_out(void *nullarg);
void user_tl_set_mark_out(void *nullarg);
void user_tl_set_mark_in(void *nullarg);
void user_tl_goto_mark_out(void *nullarg);
void user_tl_goto_mark_in(void *nullarg);
void user_tl_goto_zero(void *nullarg);
/* void user_tl_play_drag(void *nullarg); */
/* void user_tl_rewind_drag(void *nullarg); */
/* void user_tl_pause_drag(void *nullarg); */
void user_tl_toggle_drag(void *nullarg);
void user_tl_cut_clipref(void *nullarg);
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
void user_tl_track_activate_selected(void *nullarg);
void user_tl_track_rename(void *nullarg);
void user_tl_track_toggle_in(void *nullarg);
void user_tl_track_set_in(void *nullarg);
void user_tl_track_add_filter(void *nullarg);
void user_tl_track_destroy(void *nullarg);


void user_tl_mute(void *nullarg);
void user_tl_solo(void *nullarg);
void user_tl_track_vol_up(void *nullarg);
/* void user_tl_track_vol_up_toggle(void *nullarg); */
void user_tl_track_vol_down(void *nullarg);

void user_tl_track_pan_left(void *nullarg);
void user_tl_track_pan_right(void *nullarg);


void user_tl_record(void *nullarg);
void user_tl_clipref_grab_ungrab(void *nullarg);
void user_tl_cliprefs_destroy(void *nullarg);


void user_tl_load_clip_at_point_to_src(void *nullarg);
void user_tl_activate_source_mode(void *nullarg);
void user_tl_drop_from_source(void *nullarg);

void user_tl_drop_saved1_from_source(void *nullarg);
void user_tl_drop_saved2_from_source(void *nullarg);
void user_tl_drop_saved3_from_source(void *nullarg);
void user_tl_drop_saved4_from_source(void *nullarg);

void user_tl_add_new_timeline(void *nullarg);
void user_tl_previous_timeline(void *nullarg);
void user_tl_next_timeline(void *nullarg);

void user_tl_write_mixdown_to_wav(void *nullarg);

void user_source_play(void *nullarg);
void user_source_pause(void *nullarg);
void user_source_rewind(void *nullarg);
void user_source_play_slow(void *nullarg);
void user_source_rewind_slow(void *nullarg);
void user_source_set_in_mark(void *nullarg);
void user_source_set_out_mark(void *nullarg);

void user_modal_next(void *nullarg);
void user_modal_previous(void *nullarg);
void user_modal_next_escape(void *nullarg);
void user_modal_previous_escape(void *nullarg);
void user_modal_select(void *nullarg);
void user_modal_dismiss(void *nullarg);
void user_modal_submit_form(void *nullarg);


void user_text_edit_escape(void *nullarg);
void user_text_edit_backspace(void *nullarg);
void user_text_edit_cursor_right(void *nullarg);
void user_text_edit_cursor_left(void *nullarg);
void user_text_edit_select_all(void *nullarg);
#endif
