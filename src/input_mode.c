/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "input_mode.h"
#include "function_lookup.h"
#include "userfn.h"

Mode *MODES[NUM_INPUT_MODES];

static const char *input_mode_strs[] = {
    "global",
    "menu_nav",
    "timeline",
    "source",
    "modal",
    "text_edit",
    "tabview",
    "autocomplete_list",
    "midi_qwerty",
    "piano_roll"
};

const char *input_mode_str(InputMode im)
{
    if (im < NUM_INPUT_MODES) {
	return input_mode_strs[im];
    } else {
	return "[no mode]";
    }
}

InputMode input_mode_from_str(char *str)
{
    if (strcmp(str, "global") == 0) {
	return MODE_GLOBAL;
    } else if (strcmp(str, "menu_nav") == 0) {
	return MODE_MENU_NAV;
    } else if (strcmp(str, "timeline") == 0) {
	return MODE_TIMELINE;
    } else if (strcmp(str, "source") == 0) {
	return MODE_SOURCE;
    } else if (strcmp(str, "modal") == 0) {
	return MODE_MODAL;
    } else if (strcmp(str, "text_edit") == 0) {
	return MODE_TEXT_EDIT;
    } else if (strcmp(str, "tabview") == 0) {
	return MODE_TABVIEW;
    } else if (strcmp(str, "autocomplete_list") == 0) {
	return MODE_AUTOCOMPLETE_LIST;
    } else if (strcmp(str, "midi_qwerty") == 0) {
	return MODE_MIDI_QWERTY;
    } else if (strcmp(str, "piano_roll") == 0) {
	return MODE_PIANO_ROLL;
    } else {
	return -1;
    }
}


static Mode *mode_create(InputMode im)
{
    Mode *mode = calloc(1, sizeof(Mode));
    mode->name = input_mode_str(im);
    mode->num_subcats = 0;
    mode->im = im;
    MODES[im] = mode;	
    return mode;
}

static ModeSubcat *mode_add_subcat(Mode *mode, const char *name)
{
    ModeSubcat *sc = calloc(1, sizeof(ModeSubcat));
    if (!sc) {
	fprintf(stderr, "Error: failed to allocate memory\n");
	exit(1);
    }
    if (mode->num_subcats == MAX_MODE_SUBCATS) {
	fprintf(stderr, "Mode already has maximum number of subcats (%d)\n", mode->num_subcats);
	return NULL;
    }
    mode->subcats[mode->num_subcats] = sc;
    mode->num_subcats++;
    sc->name = name;
    sc->num_fns = 0;
    sc->mode = mode;
    return sc;
}

static void mode_subcat_add_fn(ModeSubcat *ms, UserFn *fn)
{
    if (ms->num_fns == MAX_MODE_SUBCAT_FNS) {
	fprintf(stderr, "Reached maximum number of functions per subcat \"%s\"\n", ms->name);
	exit(1);
    }
    ms->fns[ms->num_fns] = fn;
    ms->num_fns++;
    fn->mode = ms->mode;
    fn_lookup_index_fn(fn);
}

static UserFn *create_user_fn(
    const char *fn_id,
    const char *fn_display_name,
    void (*do_fn)(void *arg))
{
    UserFn *fn = calloc(1, sizeof(UserFn));
    fn->fn_id = fn_id;
    /* fn->annotation = NULL; */
    fn->fn_display_name = fn_display_name;
    fn->is_toggle = false;
    fn->do_fn = do_fn;
    return fn;
}



/* static void set_user_fn_toggle(UserFn *fn) */
/* { */
/*     fn->is_toggle = true; */
/* } */

static void mode_load_global()
{
    Mode *mode = mode_create(MODE_GLOBAL);
    ModeSubcat *mc = mode_add_subcat(mode, "");
    if (!mc) {
	return;
    }
    
    /* exit(0); */
    
    UserFn *fn;
    fn = create_user_fn(
	"menu",
	"Summon menu",
	user_global_menu);
    mode_subcat_add_fn(mc, fn);

    fn = create_user_fn(
	"escape",
	"Escape",
	user_global_escape);
    mode_subcat_add_fn(mc, fn);
    
    fn = create_user_fn(
	"quit",
	"Quit",
	user_global_quit);
    mode_subcat_add_fn(mc, fn);

    fn = create_user_fn(
	"undo",
	"Undo",
	user_global_undo);

    mode_subcat_add_fn(mc, fn);

    fn = create_user_fn(
	"redo",
	"Redo",
	user_global_redo);
    mode_subcat_add_fn(mc, fn);

    fn = create_user_fn(
	"show_output_freq_domain",
	"Show output spectrum",
	user_global_show_output_freq_domain);
    mode_subcat_add_fn(mc, fn);

    fn = create_user_fn(
	"save_project",
	"Save Project",
	user_global_save_project);
    mode_subcat_add_fn(mc, fn);

    fn = create_user_fn(
	"open_file",
	"Open file (.wav or .jdaw)",
	user_global_open_file);
    mode_subcat_add_fn(mc, fn);

    fn = create_user_fn(
	"start_api_server",
	"Start API server",
	user_global_start_server);
    mode_subcat_add_fn(mc, fn);

    fn = create_user_fn(
	"function_lookup",
	"Function lookup",
	user_global_function_lookup);
    mode_subcat_add_fn(mc, fn);

    fn = create_user_fn(
	"chaotic_user_test",
	"Chaotic user test (debug only)",
	user_global_chaotic_user_test);
    mode_subcat_add_fn(mc, fn);

    fn = create_user_fn(
	"api_print_all_routes",
	"Print all API routes",
	user_global_api_print_all_routes);
    mode_subcat_add_fn(mc, fn);
    /* fn = create_user_fn( */
    /* 	"start_or_stop_screenrecording", */
    /* 	"Start or stop screenrecording", */
    /* 	user_global_start_or_stop_screenrecording); */
    /* mode_subcat_add_fn(mc, fn); */
}


static void mode_load_menu_nav()
{
    
    Mode *mode = mode_create(MODE_MENU_NAV);
    ModeSubcat *mc = mode_add_subcat(mode, "");
    UserFn *fn;
    fn = create_user_fn(
	"menu_next_item",
	"Next item",
	user_menu_nav_next_item);
    mode_subcat_add_fn(mc, fn);
    
    fn = create_user_fn(
	"menu_previous_item",
	"Previous item",
	user_menu_nav_prev_item);
    mode_subcat_add_fn(mc, fn);

    fn = create_user_fn(
	"menu_next_section",
	"Next section",
	user_menu_nav_next_sctn);
    mode_subcat_add_fn(mc, fn);

    fn = create_user_fn(
	"menu_previous_section",
	"Previous section",
	user_menu_nav_prev_sctn);
    mode_subcat_add_fn(mc, fn);

    fn = create_user_fn(
	"menu_choose_item",
	"Choose item",
	user_menu_nav_choose_item);
    mode_subcat_add_fn(mc, fn);

    fn = create_user_fn(
	"menu_column_right",
	"Column right",
	user_menu_nav_column_right);
    mode_subcat_add_fn(mc, fn);

    fn = create_user_fn(
	"menu_column_left",
	"Column left",
	user_menu_nav_column_left);
    mode_subcat_add_fn(mc, fn);


    fn = create_user_fn(
	"menu_translate_up",
	"Move menu up",
	user_menu_translate_up);
    mode_subcat_add_fn(mc, fn);

    fn = create_user_fn(
	"menu_translate_down",
	"Move menu down",
	user_menu_translate_down);
    mode_subcat_add_fn(mc, fn);
    
    fn = create_user_fn(
	"menu_translate_right",
	"Move menu right",
	user_menu_translate_right);
    mode_subcat_add_fn(mc, fn);

    fn = create_user_fn(
	"menu_translate_left",
	"Move menu left",
	user_menu_translate_left);
    mode_subcat_add_fn(mc, fn);

    fn = create_user_fn(
	"menu_dismiss",
	"go back (dismiss)",
	user_menu_dismiss);
    mode_subcat_add_fn(mc, fn);

}

static void mode_load_timeline()
{
    Mode *mode = mode_create(MODE_TIMELINE);
    /* ModeSubcat *sc= mode_add_subcat(mode, "Timeline Navigation"); */
    UserFn *fn;


    /* Playback / record */

    ModeSubcat *sc = mode_add_subcat(mode, "Playback / Record");

    fn = create_user_fn(
	"tl_play",
	"Play",
	user_tl_play);
    mode_subcat_add_fn(sc, fn);
    
    fn = create_user_fn(
	"tl_pause",
	"Pause",
	user_tl_pause);
    mode_subcat_add_fn(sc, fn);
    
    fn = create_user_fn(
	"tl_rewind",
	"Rewind",
	user_tl_rewind);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"tl_play_pause",
	"Play/pause",
	user_tl_play_pause);
    mode_subcat_add_fn(sc, fn);

    fn=create_user_fn(
	"tl_play_slow",
	"Play slow",
	user_tl_play_slow);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"tl_rewind_slow",
	"Rewind slow",
	user_tl_rewind_slow);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"tl_move_playhead_left",
	"Move playhead left",
	user_tl_move_playhead_left);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"tl_move_playhead_right",
	"Move playhead right",
	user_tl_move_playhead_right);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"tl_move_playhead_left_slow",
	"Move playhead left (slow)",
	user_tl_move_playhead_left_slow);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"tl_move_playhead_right_slow",
	"Move playhead right (slow)",
	user_tl_move_playhead_right_slow);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"tl_nudge_left",
	"Nudge play position left (500 samples)",
	user_tl_nudge_left);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"tl_nudge_right",
	"Nudge play position right (500 samples)",
	user_tl_nudge_right);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"tl_small_nudge_left",
	"Nudge play position left (100 samples)",
	user_tl_small_nudge_left);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"tl_small_nudge_right",
	"Nudge play position right (100 samples)",
	user_tl_small_nudge_right);
    mode_subcat_add_fn(sc, fn);
    
    fn = create_user_fn(
	"tl_one_sample_left",
	"Move one sample left",
	user_tl_one_sample_left);
    mode_subcat_add_fn(sc, fn);

        fn = create_user_fn(
	"tl_one_sample_right",
	"Move one sample right",
	user_tl_one_sample_right);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"tl_record",
	"Record (start or stop)",
	user_tl_record);
    mode_subcat_add_fn(sc, fn);


    /* Other timeline navigation */
    
    sc = mode_add_subcat(mode, "Timeline navigation");

    fn = create_user_fn(
	"tl_track_selector_up",
	"Previous track (move selector up)",
        user_tl_track_selector_up);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"tl_track_selector_down",
	"Next track (move selector down)",
        user_tl_track_selector_down);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"tl_track_automation_toggle_read",
	"Toggle automation read",
        user_tl_track_automation_toggle_read);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"tl_move_track_down",
	"Move selected track down",
	user_tl_move_track_down);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"tl_move_track_up",
	"Move selected track up",
	user_tl_move_track_up);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"tl_track_or_tracks_minimize",
	"Minimize selected track(s)",
	user_tl_tracks_minimize);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"tl_move_right",
	"Move view right",
	user_tl_move_right);
    mode_subcat_add_fn(sc,fn);

    fn = create_user_fn(
	"tl_move_left",
	"Move view left",
	user_tl_move_left);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"tl_zoom_out",
	"Zoom out",
	user_tl_zoom_out);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"tl_zoom_in",
	"Zoom in",
	user_tl_zoom_in);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"tl_lock_view_to_playhead",
	"Lock view to playhead",
	user_tl_lock_view_to_playhead);
    mode_subcat_add_fn(sc, fn);

    /* fn = create_user_fn( */
    /* 	"tl_play_drag", */
    /* 	"Play and drag grabbed clips", */
    /* 	user_tl_play_drag */
    /* 	); */
    /* mode_subcat_add_fn(sc, fn); */

    /* fn = create_user_fn( */
    /* 	"tl_rewind_drag", */
    /* 	"Rewind and drag grabbed clips", */
    /* 	user_tl_rewind_drag */
    /* 	); */
    /* mode_subcat_add_fn(sc, fn); */

    /* fn = create_user_fn( */
    /* 	"tl_pause_drag", */
    /* 	"Pause and stop dragging clips", */
    /* 	user_tl_pause_drag */
    /* 	); */
    /* mode_subcat_add_fn(sc, fn); */

    
    /* Marks */
    
    sc = mode_add_subcat(mode, "Marks");
    fn = create_user_fn(
	"tl_set_in_mark",
	"Set In",
	user_tl_set_mark_in);
    mode_subcat_add_fn(sc, fn);
    
    fn = create_user_fn(
	"tl_set_out_mark",
	"Set Out",
	user_tl_set_mark_out);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"tl_goto_in_mark",
	"Go to In",
	user_tl_goto_mark_in);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"tl_goto_out_mark",
	"Go to Out",
	user_tl_goto_mark_out);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"tl_goto_zero",
	"Go to t=0",
	user_tl_goto_zero);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"tl_goto_previous_clip_boundary",
	"Go to previous clip boundary",
	user_tl_goto_previous_clip_boundary);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"tl_goto_next_clip_boundary",
	"Go to next clip boundary",
	user_tl_goto_next_clip_boundary);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"tl_goto_next_beat",
	"Go to next beat",
	user_tl_goto_next_beat);
    mode_subcat_add_fn(sc, fn);
    
    fn = create_user_fn(
	"tl_goto_prev_beat",
	"Go to previous beat",
	user_tl_goto_prev_beat);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"tl_goto_next_subdiv",
	"Go to next subdiv",
	user_tl_goto_next_subdiv);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"tl_goto_prev_subdiv",
	"Go to previous subdiv",
	user_tl_goto_prev_subdiv);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"tl_goto_next_measure",
	"Go to next measure",
	user_tl_goto_next_measure);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"tl_goto_prev_measure",
	"Go to previous measure",
	user_tl_goto_prev_measure);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"tl_bring_rear_clip_to_front",
	"Bring rear clip at cursor to front",
	user_tl_bring_rear_clip_to_front);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"tl_toggle_loop_playback",
	"Toggle loop playback",
	user_tl_toggle_loop_playback);
    mode_subcat_add_fn(sc, fn);

    /* Audio output */

    sc = mode_add_subcat(mode, "Output");
    fn = create_user_fn(
	"tl_set_default_out",
	"Set default audio output",
	user_tl_set_default_out);
    mode_subcat_add_fn(sc, fn);

    
    /* Tracks */
    sc= mode_add_subcat(mode, "Tracks");
    
    fn = create_user_fn(
	"tl_track_add",
	"Add Track",
        user_tl_add_track);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"tl_track_activate_selected",
	"Activate/deactivate selected track",
        user_tl_track_activate_selected);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"tl_track_activate_all",
	"Activate/deactivate all tracks",
        user_tl_track_activate_all);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"tl_track_delete",
	"Delete selected track or automation",
	user_tl_track_delete);
    mode_subcat_add_fn(sc, fn);
    
    fn = create_user_fn(
	"tl_track_select_1",
	"Activate track 1",
        user_tl_track_select_1);
    mode_subcat_add_fn(sc, fn);

        fn = create_user_fn(
	"tl_track_select_2",
	"Activate track 2",
        user_tl_track_select_2);
    mode_subcat_add_fn(sc, fn);

        fn = create_user_fn(
	"tl_track_select_3",
	"Activate track 3",
        user_tl_track_select_3);
    mode_subcat_add_fn(sc, fn);
    fn = create_user_fn(
	"tl_track_select_4",
	"Activate track 4",
        user_tl_track_select_4);
    mode_subcat_add_fn(sc, fn);

        fn = create_user_fn(
	"tl_track_select_5",
	"Activate track 5",
        user_tl_track_select_5);
    mode_subcat_add_fn(sc, fn);

        fn = create_user_fn(
	"tl_track_select_6",
	"Activate track 6",
        user_tl_track_select_6);
    mode_subcat_add_fn(sc, fn);

        fn = create_user_fn(
	"tl_track_select_7",
	"Activate track 7",
        user_tl_track_select_7);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"tl_track_select_8",
	"Activate track 8",
        user_tl_track_select_8);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"tl_track_select_9",
	"Activate track 9",
        user_tl_track_select_9);
    mode_subcat_add_fn(sc, fn);

    /* Click tracks */
    sc = mode_add_subcat(mode, "Click tracks");

    fn = create_user_fn(
	"tl_click_track_add",
	"Add click track",
        user_tl_click_track_add);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"tl_click_track_set_tempo",
	"Set tempo at cursor",
	user_tl_click_track_set_tempo);
    mode_subcat_add_fn(sc, fn);

    /* fn = create_user_fn( */
    /* 	"tl_click_track_cut", */
    /* 	"Cut tempo track at cursor", */
    /* 	user_tl_click_track_cut); */
    /* mode_subcat_add_fn(sc, fn); */

    /* fn = create_user_fn( */
    /* 	"tl_click_track_set_tempo", */
    /* 	"Set tempo at cursor", */
    /* 	user_tl_click_track_set_tempo); */
    /* mode_subcat_add_fn(sc, fn); */

    /* Track settings */
    sc = mode_add_subcat(mode, "Track settings");

    fn = create_user_fn(
	"tl_track_add_effect",
	"Add effect to track",
	user_tl_track_add_effect);
    mode_subcat_add_fn(sc, fn);
    
    fn = create_user_fn(
	"tl_track_open_settings",
	"Open track settings",
	user_tl_track_open_settings);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"tl_track_open_synth",
	"Open synth",
	user_tl_track_open_synth);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"tl_mute",
	"Mute or unmute selected track(s)",
	user_tl_mute);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"tl_solo",
	"Solo or unsolo selected track(s)",
	user_tl_solo);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"tl_track_vol_up",
	"Track volume up",
	user_tl_track_vol_up);
    /* set_user_fn_toggle(fn); */
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"tl_track_vol_down",
	"Track volume down",
	user_tl_track_vol_down);
    mode_subcat_add_fn(sc, fn);


    /* fn = create_user_fn( */
    /* 	"tl_track_vol_down_toggle", */
    /* 	"Track volume down", */
    /* 	user_tl_track_vol_down_toggle */
    /* 	); */
    /* mode_subcat_add_fn(sc, fn); */

    fn = create_user_fn(
	"tl_track_pan_left",
	"Track pan left",
	user_tl_track_pan_left);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"tl_track_pan_right",
	"Track pan right",
	user_tl_track_pan_right);
    mode_subcat_add_fn(sc, fn);
    
    fn = create_user_fn(
	"tl_track_rename",
	"Rename selected track",
	user_tl_track_rename);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"tl_track_set_in",
	"Set track input",
	user_tl_track_set_in);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"tl_track_show_hide_automations",
	"Show or hide all track automations",
	user_tl_track_show_hide_automations);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"tl_track_add_automation",
	"Add automation to track",
	user_tl_track_add_automation);
    mode_subcat_add_fn(sc, fn);

    /* fn = create_user_fn( */
    /* 	"tl_track_add_filter", */
    /* 	"Add filter to track", */
    /* 	user_tl_track_add_filter); */
    /* mode_subcat_add_fn(sc, fn); */

    /* Clips */
    sc = mode_add_subcat(mode, "Clips");

    fn = create_user_fn(
	"tl_grab_clips_at_cursor",
	"Grab clip at cursor",
	user_tl_clipref_grab_ungrab);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"tl_grab_marked_range",
	"Grab clips in marked range",
	user_tl_grab_marked_range);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"tl_copy_grabbed_clips",
	"Copy grabbed clips",
	user_tl_copy_grabbed_clips);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"tl_paste_grabbed_clips",
	"Paste grabbed clips",
	user_tl_paste_grabbed_clips);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"tl_toggle_drag",
	"Start or stop dragging clips",
	user_tl_toggle_drag);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"tl_cut_clipref",
	"Cut",
	user_tl_cut_clipref);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"tl_split_stereo_clipref",
	"Split stereo clip at cursor to mono",
	user_tl_split_stereo_clipref);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"tl_rename_clip_at_cursor",
	"Rename clip at cursor",
	user_tl_rename_clip_at_cursor);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"tl_delete_generic",
	"Delete",
	user_tl_delete_generic);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"tl_edit_clip_at_cursor",
	"Edit clip at cursor",
	user_tl_edit_clip_at_cursor);
    mode_subcat_add_fn(sc, fn);
    

    /* Sample mode */

    sc = mode_add_subcat(mode, "Sample mode");
    fn = create_user_fn(
	"tl_load_clip_at_cursor_to_source",
	"Load clip at cursor to source",
        user_tl_load_clip_at_cursor_to_src);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"tl_activate_source_mode",
	"Activate Source Mode",
	user_tl_activate_source_mode);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"tl_drop_from_source",
	"Drop clip from source",
	user_tl_drop_from_source);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"tl_drop_saved1_from_source",
	"Drop previously dropped clip (1)",
	user_tl_drop_saved1_from_source);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"tl_drop_saved2_from_source",
	"Drop previously dropped clip (2)",
	user_tl_drop_saved2_from_source);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"tl_drop_saved3_from_source",
	"Drop previously dropped clip (3)",
	user_tl_drop_saved3_from_source);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"tl_drop_saved4_from_source",
	"Drop previously dropped clip (4)",
	user_tl_drop_saved4_from_source);
    mode_subcat_add_fn(sc, fn);


    /* Project navigation */
    sc = mode_add_subcat(mode, "Multiple timelines");

    fn = create_user_fn(
	"tl_add_new_timeline",
	"Add new timeline",
	user_tl_add_new_timeline);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"tl_previous_timeline",
	"Previous timeline",
	user_tl_previous_timeline);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"tl_next_timeline",
	"Next timeline",
	user_tl_next_timeline);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"tl_delete_timeline",
	"Delete timeline",
	user_tl_delete_timeline);
    mode_subcat_add_fn(sc, fn);


    /* Export */

    sc = mode_add_subcat(mode, "Export");

    fn = create_user_fn(
	"tl_write_mixdown_to_wav",
	"Write mixdown to .wav file",
	user_tl_write_mixdown_to_wav);
    mode_subcat_add_fn(sc, fn);

    
    sc = mode_add_subcat(mode, "MIDI I/O");
    fn = create_user_fn(
	"tl_track_set_midi_out",
	"Set track MIDI out",
	user_tl_track_set_midi_out);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"tl_activate_qwerty_piano",
	"Activate QWERTY piano",
	user_tl_activate_mqwert);
    mode_subcat_add_fn(sc, fn);
    fn = create_user_fn(
	"tl_insert_jlily",
	"Insert Jlily (LilyPond) notes",
	user_tl_insert_jlily);
    mode_subcat_add_fn(sc, fn);


    /* fn = create_user_fn( */
    /* 	"tl_cliprefs_destroy", */
    /* 	"Delete selected clip(s)", */
    /* 	user_tl_cliprefs_destroy); */
    /* mode_subcat_add_fn(sc, fn); */
    
}

static void mode_load_source()
{
    Mode *mode = mode_create(MODE_SOURCE);
    ModeSubcat *sc= mode_add_subcat(mode, "");
    UserFn *fn;

    fn = create_user_fn(
	"source_play",
	"Play (source)",
        user_source_play);
    mode_subcat_add_fn(sc, fn);


    fn = create_user_fn(
	"source_pause",
	"Pause (source)",
        user_source_pause);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"source_rewind",
	"Rewind (source)",
        user_source_rewind);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"source_play_slow",
	"Play slow (source)",
	user_source_play_slow);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"source_rewind_slow",
	"Rewind slow (source",
	user_source_rewind_slow);
    mode_subcat_add_fn(sc, fn);
    
    fn = create_user_fn(
	"source_set_in_mark",
	"Set In Mark (source)",
        user_source_set_in_mark);
    mode_subcat_add_fn(sc, fn);
    
    fn = create_user_fn(
	"source_set_out_mark",
	"Set Out Mark (source)",
        user_source_set_out_mark);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"source_zoom_in",
	"Zoom in (source)",
	user_source_zoom_in);
    mode_subcat_add_fn(sc, fn);
    
    fn = create_user_fn(
	"source_zoom_out",
	"Zoom out (source)",
	user_source_zoom_out);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"source_move_left",
	"Move left (source)",
	user_source_move_left);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"source_move_right",
	"Move right (source)",
	user_source_move_right);
    mode_subcat_add_fn(sc, fn);

}

static void mode_load_modal()
{
    Mode *mode = mode_create(MODE_MODAL);
    ModeSubcat *sc= mode_add_subcat(mode, "");
    UserFn *fn;   


    fn = create_user_fn(
	"modal_next",
	"Go to next item",
	user_modal_next);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"modal_previous",
	"Go to previous item",
	user_modal_previous);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"modal_next_escape",
	"Go to next item (escape DirNav)",
	user_modal_next_escape);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"modal_previous_escape",
	"Go to previous item (escape DirNav)",
	user_modal_previous_escape);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"modal_select",
	"Select item",
	user_modal_select);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"modal_dismiss",
	"Dismiss modal window",
	user_modal_dismiss);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"modal_submit_form",
	"Submit form",
	user_modal_submit_form);
    mode_subcat_add_fn(sc, fn);
}

static void mode_load_text_edit()
{
    Mode *mode = mode_create(MODE_TEXT_EDIT);
    /* MODES[TEXT_EDIT] = mode; */

    ModeSubcat *sc = mode_add_subcat(mode, "");
    UserFn *fn = create_user_fn(
	"text_edit_escape",
	"Escape text edit",
	user_text_edit_escape);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"text_edit_full_escape",
	"Escape text edit",
	user_text_edit_full_escape);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"text_edit_backspace",
	"Backspace",
	user_text_edit_backspace);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"text_edit_cursor_right",
	"Move cursor right",
	user_text_edit_cursor_right);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"text_edit_cursor_left",
	"Move cursor left",
	user_text_edit_cursor_left);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"text_edit_select_all",
	"Select all",
	user_text_edit_select_all);
    mode_subcat_add_fn(sc, fn);

}


static void mode_load_tabview()
{
    Mode *mode = mode_create(MODE_TABVIEW);
    /* modes[TABVIEW] = mode; */

    ModeSubcat *sc = mode_add_subcat(mode, "");

    UserFn *fn = create_user_fn(
	"tabview_next_escape",
	"Next element",
	user_tabview_next_escape);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"tabview_previous_escape",
	"Previous element",
	user_tabview_previous_escape);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"tabview_enter",
	"Select",
	user_tabview_enter);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"tabview_left",
	"Move left",
	user_tabview_left);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"tabview_right",
	"Move right",
	user_tabview_right);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"tabview_next_tab",
	"Next tab",
	user_tabview_next_tab);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"tabview_previous_tab",
	"Previous tab",
	user_tabview_previous_tab);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"tabview_move_current_tab_left",
	"Move current tab left",
	user_tabview_move_current_tab_left);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"tabview_move_current_tab_right",
	"Move current tab right",
	user_tabview_move_current_tab_right);
    mode_subcat_add_fn(sc, fn);


    fn = create_user_fn(
	"tabview_escape",
	"Close tab view",
	user_tabview_escape);
    mode_subcat_add_fn(sc, fn);
}




static void mode_load_autocomplete_list()
{
    Mode *mode = mode_create(MODE_AUTOCOMPLETE_LIST);
    ModeSubcat *sc = mode_add_subcat(mode, "");

    UserFn *fn = create_user_fn(
	"autocomplete_next",
	"Next item",
	user_autocomplete_next);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"autocomplete_previous",
	"Previous item",
	user_autocomplete_previous);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"autocomplete_select",
	"Select item",
	user_autocomplete_select);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"autocomplete_escape",
	"Escape autocomplete list",
	user_autocomplete_escape);
    mode_subcat_add_fn(sc, fn);
}

static void mode_load_midi_qwerty()
{
    Mode *mode = mode_create(MODE_MIDI_QWERTY);
    ModeSubcat *sc = mode_add_subcat(mode, "");

    UserFn *fn = create_user_fn(
	"midi_qwerty_escape",
	"Escape MIDI QWERTY",
	user_midi_qwerty_escape);
    mode_subcat_add_fn(sc, fn);
    fn = create_user_fn(
	"midi_qwerty_octave_up",
	"Octave up",
	user_midi_qwerty_octave_up);
    mode_subcat_add_fn(sc, fn);
    fn = create_user_fn(
	"midi_qwerty_octave_down",
	"Octave down",
	user_midi_qwerty_octave_down);
    mode_subcat_add_fn(sc, fn);
    fn = create_user_fn(
	"midi_qwerty_transpose_up",
	"Transpose up",
	user_midi_qwerty_transpose_up);
    mode_subcat_add_fn(sc, fn);
    fn = create_user_fn(
	"midi_qwerty_transpose_down",
	"Transpose down",
	user_midi_qwerty_transpose_down);
    mode_subcat_add_fn(sc, fn);
    fn = create_user_fn(
	"midi_qwerty_velocity_up",
	"Velocity up",
	user_midi_qwerty_velocity_up);
    mode_subcat_add_fn(sc, fn);
    fn = create_user_fn(
	"midi_qwerty_velocity_down",
	"Velocity down",
	user_midi_qwerty_velocity_down);
    mode_subcat_add_fn(sc, fn);



    
    fn = create_user_fn(
	"midi_qwerty_c1",
	"midi qwerty c1",
	user_midi_qwerty_c1);
    mode_subcat_add_fn(sc, fn);
    fn = create_user_fn(
	"midi_qwerty_cis1",
	"midi qwerty cis1",
	user_midi_qwerty_cis1);
    mode_subcat_add_fn(sc, fn);
    fn = create_user_fn(
	"midi_qwerty_d1",
	"midi qwerty d1",
	user_midi_qwerty_d1);
    mode_subcat_add_fn(sc, fn);
    fn = create_user_fn(
	"midi_qwerty_dis",
	"midi qwerty dis",
	user_midi_qwerty_dis);
    mode_subcat_add_fn(sc, fn);
    fn = create_user_fn(
	"midi_qwerty_e",
	"midi qwerty e",
	user_midi_qwerty_e);
    mode_subcat_add_fn(sc, fn);
    fn = create_user_fn(
	"midi_qwerty_f",
	"midi qwerty f",
	user_midi_qwerty_f);
    mode_subcat_add_fn(sc, fn);
    fn = create_user_fn(
	"midi_qwerty_fis",
	"midi qwerty fis",
	user_midi_qwerty_fis);
    mode_subcat_add_fn(sc, fn);
    fn = create_user_fn(
	"midi_qwerty_g",
	"midi qwerty g",
	user_midi_qwerty_g);
    mode_subcat_add_fn(sc, fn);
    fn = create_user_fn(
	"midi_qwerty_gis",
	"midi qwerty gis",
	user_midi_qwerty_gis);
    mode_subcat_add_fn(sc, fn);
    fn = create_user_fn(
	"midi_qwerty_a",
	"midi qwerty a",
	user_midi_qwerty_a);
    mode_subcat_add_fn(sc, fn);
    fn = create_user_fn(
	"midi_qwerty_ais",
	"midi qwerty ais",
	user_midi_qwerty_ais);
    mode_subcat_add_fn(sc, fn);
    fn = create_user_fn(
	"midi_qwerty_b",
	"midi qwerty b",
	user_midi_qwerty_b);
    mode_subcat_add_fn(sc, fn);
    fn = create_user_fn(
	"midi_qwerty_c2",
	"midi qwerty c2",
	user_midi_qwerty_c2);
    mode_subcat_add_fn(sc, fn);
    fn = create_user_fn(
	"midi_qwerty_cis2",
	"midi qwerty cis2",
	user_midi_qwerty_cis2);
    mode_subcat_add_fn(sc, fn);
    fn = create_user_fn(
	"midi_qwerty_d2",
	"midi qwerty d2",
	user_midi_qwerty_d2);
    mode_subcat_add_fn(sc, fn);
    fn = create_user_fn(
	"midi_qwerty_dis2",
	"midi qwerty dis2",
	user_midi_qwerty_dis2);
    mode_subcat_add_fn(sc, fn);
    fn = create_user_fn(
	"midi_qwerty_e2",
	"midi qwerty e2",
	user_midi_qwerty_e2);
    mode_subcat_add_fn(sc, fn);
    fn = create_user_fn(
	"midi_qwerty_f2",
	"midi qwerty f2",
	user_midi_qwerty_f2);
    mode_subcat_add_fn(sc, fn);
}

void mode_load_piano_roll()
{
    Mode *mode = mode_create(MODE_PIANO_ROLL);
    ModeSubcat *sc = mode_add_subcat(mode, "");

    UserFn *fn = create_user_fn(
	"piano_roll_escape",
	"Escape / exit piano roll",

	user_piano_roll_escape);

    mode_subcat_add_fn(sc, fn);


    fn = create_user_fn(
	"piano_roll_zoom_in",
	"Zoom in (piano roll)",
	user_piano_roll_zoom_in);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"piano_roll_zoom_out",
	"Zoom out (piano roll)",
	user_piano_roll_zoom_out);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"piano_roll_move_left",
	"Move view left (piano roll)",
	user_piano_roll_move_left);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"piano_roll_move_right",
	"Move view right (piano roll)",
	user_piano_roll_move_right);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"piano_roll_note_down",
	"Note selector down",
	user_piano_roll_note_down);
    mode_subcat_add_fn(sc, fn);
    fn = create_user_fn(
	"piano_roll_note_up",
	"Note selector up",
	user_piano_roll_note_up);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"piano_roll_next_note",
	"Go to next note",
	user_piano_roll_next_note);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"piano_roll_prev_note",
	"Go to previous note",
	user_piano_roll_prev_note);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"piano_roll_dur_longer",
	"Longer note duration",
	user_piano_roll_dur_longer);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"piano_roll_dur_shorter",
	"Shorter note duration",
	user_piano_roll_dur_shorter);
    mode_subcat_add_fn(sc, fn);
}

void mode_load_all()
{
    mode_load_global();
    mode_load_menu_nav();
    mode_load_timeline();
    mode_load_source();
    mode_load_modal();
    mode_load_text_edit();
    mode_load_tabview();
    mode_load_autocomplete_list();
    mode_load_midi_qwerty();
    mode_load_piano_roll();
}
