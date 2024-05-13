#ifndef JDAW_GUI_USERFN_H
#define JDAW_GUI_USERFN_H


void user_global_expose_help();
void user_global_quit();
void user_global_undo();
void user_global_redo();
void user_global_save_project();
void user_menu_nav_next_item();
void user_menu_nav_prev_item();
void user_menu_nav_next_sctn();
void user_menu_nav_prev_sctn();
void user_menu_nav_column_right();
void user_menu_nav_column_left();
void user_menu_nav_choose_item();


void user_menu_translate_up();
void user_menu_translate_down();
void user_menu_translate_left();
void user_menu_translate_right();
void user_menu_dismiss();


void user_tl_play();
void user_tl_pause();
void user_tl_rewind();
void user_tl_play_slow();
void user_tl_rewind_slow();
void user_tl_move_right();
void user_tl_move_left();
void user_tl_zoom_in();
void user_tl_zoom_out();
void user_tl_set_mark_out();
void user_tl_set_mark_in();
void user_tl_goto_mark_out();
void user_tl_goto_mark_in();
void user_tl_goto_zero();
/* void user_tl_play_drag(); */
/* void user_tl_rewind_drag(); */
/* void user_tl_pause_drag(); */
void user_tl_toggle_drag();
void user_tl_add_track();

void user_tl_track_select_1();
void user_tl_track_select_2();
void user_tl_track_select_3();
void user_tl_track_select_4();
void user_tl_track_select_5();
void user_tl_track_select_6();
void user_tl_track_select_7();
void user_tl_track_select_8();
void user_tl_track_select_9();
void user_tl_track_activate_all();
void user_tl_track_selector_up();
void user_tl_track_selector_down();
void user_tl_track_activate_selected();
void user_tl_track_rename();
void user_tl_track_toggle_in();
void user_tl_track_set_in();

void user_tl_mute();
void user_tl_solo();
void user_tl_track_vol_up();
/* void user_tl_track_vol_up_toggle(); */
void user_tl_track_vol_down();

void user_tl_track_pan_left();
void user_tl_track_pan_right();


void user_tl_record();
void user_tl_clipref_grab_ungrab();


void user_tl_load_clip_at_point_to_src();
void user_tl_activate_source_mode();
void user_tl_drop_from_source();

void user_tl_add_new_timeline();
void user_tl_previous_timeline();
void user_tl_next_timeline();

void user_source_play();
void user_source_pause();
void user_source_rewind();
void user_source_play_slow();
void user_source_rewind_slow();
void user_source_set_in_mark();
void user_source_set_out_mark();
#endif
