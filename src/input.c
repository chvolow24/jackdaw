/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/


/*****************************************************************************************************************
    input.c

    * User input handling framework
    * Initialize user input modes
    * User input functions in userfn.c
 *****************************************************************************************************************/


#include <string.h>
#include <stdbool.h>
#include "input.h"
#include "layout.h"
#include "menu.h"
#include "userfn.h"

#define not_whitespace_char(c) (c != ' ' && c != '\n' && c != '\t')
#define is_whitespace_char(c) ((c) == ' ' || (c) == '\n' || (c) == '\t')

#define FN_REFERENCE_FILENAME "fn_reference.md"

static Mode *modes[NUM_INPUT_MODES];
static KeybNode *input_hash_table[INPUT_HASH_SIZE];
/* KeybNode *input_keyup_hash_table[INPUT_KEYUP_HASH_SIZE]; */
extern Window *main_win;


static const char *input_mode_strs[] = {
    "global",
    "menu_nav",
    "timeline",
    "source",
    "modal",
    "text_edit",
    "tabview"
};

const char *input_mode_str(InputMode im)
{
    if (im < NUM_INPUT_MODES) {
	return input_mode_strs[im];
    } else {
	return "[no mode]";
    }
    /* switch (im) { */
    /* case GLOBAL: */
    /* 	return "global"; */
    /* case MENU_NAV: */
    /* 	return "menu_nav"; */
    /* case TIMELINE: */
    /* 	return "timeline"; */
    /* case SOURCE: */
    /* 	return "source"; */
    /* default: */
    /* 	fprintf(stderr, "ERROR: [no mode string for value %d]\n", im); */
    /* 	return "[no mode]"; */
    /* } */
}

InputMode input_mode_from_str(char *str)
{
    if (strcmp(str, "global") == 0) {
	return GLOBAL;
    } else if (strcmp(str, "menu_nav") == 0) {
	return MENU_NAV;
    } else if (strcmp(str, "timeline") == 0) {
	return TIMELINE;
    } else if (strcmp(str, "source") == 0) {
	return SOURCE;
    } else if (strcmp(str, "modal") == 0) {
	return MODAL;
    } else if (strcmp(str, "text_edit") == 0) {
	return TEXT_EDIT;
    } else if (strcmp(str, "tabview") == 0) {
	return TABVIEW;
    } else {
	return -1;
    }
}

static Mode *mode_create(InputMode im)
{
    Mode *mode = malloc(sizeof(Mode));
    mode->name = input_mode_str(im);
    mode->num_subcats = 0;
    modes[im] = mode;	
    return mode;
}

static ModeSubcat *mode_add_subcat(Mode *mode, const char *name)
{
    ModeSubcat *sc = malloc(sizeof(ModeSubcat));
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
    Mode *mode = mode_create(GLOBAL);
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
	"Open File (.wav or .jdaw)",
	user_global_open_file);
    mode_subcat_add_fn(mc, fn);

    fn = create_user_fn(
	"start_server",
	"Start API server",
	user_global_start_server);
    mode_subcat_add_fn(mc, fn);

    fn = create_user_fn(
	"chaotic_user_test",
	"Chaotic user test (debug only)",
	user_global_chaotic_user_test);
    mode_subcat_add_fn(mc, fn);
    /* fn = create_user_fn( */
    /* 	"start_or_stop_screenrecording", */
    /* 	"Start or stop screenrecording", */
    /* 	user_global_start_or_stop_screenrecording); */
    /* mode_subcat_add_fn(mc, fn); */
}


static void mode_load_menu_nav()
{
    
    Mode *mode = mode_create(MENU_NAV);
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
    Mode *mode = mode_create(TIMELINE);
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
	"Move playhead rigth",
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
	"Move track selector up",
        user_tl_track_selector_up);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"tl_track_selector_down",
	"Move track selector down",
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
	"Go to prev beat",
	user_tl_goto_prev_beat);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"tl_goto_next_subdiv",
	"Go to next subdiv",
	user_tl_goto_next_subdiv);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"tl_goto_prev_subdiv",
	"Go to prev subdiv",
	user_tl_goto_prev_subdiv);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"tl_goto_next_measure",
	"Go to next measure",
	user_tl_goto_next_measure);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"tl_goto_prev_measure",
	"Go to prev measure",
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
	"Select track 1",
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

    /* Tempo tracks */
    sc = mode_add_subcat(mode, "Tempo tracks");

    fn = create_user_fn(
	"tl_click_track_add",
	"Add tempo track",
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
	"tl_track_open_settings",
	"Open track settings",
	user_tl_track_open_settings);
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
	"tl_rename_clip_at_cursor",
	"Rename clip at cursor",
	user_tl_rename_clip_at_cursor);
    mode_subcat_add_fn(sc, fn);

    fn = create_user_fn(
	"tl_delete_generic",
	"Delete",
	user_tl_delete_generic);
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

    /* fn = create_user_fn( */
    /* 	"tl_cliprefs_destroy", */
    /* 	"Delete selected clip(s)", */
    /* 	user_tl_cliprefs_destroy); */
    /* mode_subcat_add_fn(sc, fn); */
    
}

static void mode_load_source()
{
    Mode *mode = mode_create(SOURCE);
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
        user_source_set_out_mark
	);
    mode_subcat_add_fn(sc, fn);

}

static void mode_load_modal()
{
    Mode *mode = mode_create(MODAL);
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

void mode_load_text_edit()
{
    Mode *mode = mode_create(TEXT_EDIT);
    modes[TEXT_EDIT] = mode;

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


void mode_load_tabview()
{
    Mode *mode = mode_create(TABVIEW);
    modes[TABVIEW] = mode;

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
	"tabview_escape",
	"Close tab view",
	user_tabview_escape);
    mode_subcat_add_fn(sc, fn);
}
void input_init_mode_load_all()
{
    mode_load_global();
    mode_load_menu_nav();
    mode_load_timeline();
    mode_load_source();
    mode_load_modal();
    mode_load_text_edit();
    mode_load_tabview();
}

void input_init_hash_table()
{
    memset(input_hash_table, '\0', INPUT_HASH_SIZE * sizeof(KeybNode*));
    /* memset(input_keyup_hash_table, '\0', INPUT_KEYUP_HASH_SIZE * sizeof(KeybNode*)); */
}

static int input_hash(uint16_t i_state, SDL_Keycode key)
{
    return (7 * i_state + 13 * key) % INPUT_HASH_SIZE;
}

char *input_get_keycmd_str(uint16_t i_state, SDL_Keycode keycode);

UserFn *input_get(uint16_t i_state, SDL_Keycode keycode)
{    
    int hash = input_hash(i_state, keycode);
    int win_mode_i = main_win->num_modes - 1;
    InputMode mode = main_win->modes[win_mode_i];
    while (win_mode_i >= -1) {
	KeybNode *init_node = input_hash_table[hash];
	KeybNode *node = init_node;
	if (!node) {
	    return NULL;
	}
	while (1) {
	    /* if ((node->kb->mode == mode || node->kb->mode == GLOBAL) && node->kb->i_state == i_state && node->kb->keycode == keycode) { */
	    if ((node->kb->mode == mode) && node->kb->i_state == i_state && node->kb->keycode == keycode) {
		if (node->kb->mode == GLOBAL && mode == TEXT_EDIT) {
		    txt_stop_editing(main_win->txt_editing);
		}
		return node->kb->fn;
	    } else if (node->next) {
		node = node->next;
	    } else {
		break;
	    }
	}
	if (mode == TEXT_EDIT) return NULL; /* No "sieve" for text edit mode */
	win_mode_i--;
	if (win_mode_i < 0) {
	    mode = GLOBAL;
	} else {
	    mode = main_win->modes[win_mode_i];
	}
    }
    return NULL;
}

/* char *input_state_str(uint16_t i_state) */
/* { */
    

/* } */


static void input_read_keycmd(char *keycmd, uint16_t *i_state, SDL_Keycode *key)
{
    
    /* Get prefix */
    if (strncmp("C-S-", keycmd, 4) == 0) {
	*i_state =  I_STATE_CMDCTRL | I_STATE_SHIFT;
	keycmd += 4;
    } else if (strncmp("A-S-", keycmd, 4) == 0) {
	*i_state =  I_STATE_META | I_STATE_SHIFT;
	keycmd += 4;
    } else if (strncmp("C-A-", keycmd, 4) == 0) {
	*i_state =  I_STATE_CMDCTRL | I_STATE_META;
	keycmd += 4;
    } else if (strncmp("C-", keycmd, 2) == 0) {
	*i_state =  I_STATE_CMDCTRL;
	keycmd += 2;
    } else if (strncmp("S-", keycmd, 2) == 0) {
	*i_state =  I_STATE_SHIFT;
	keycmd += 2;
    } else if (strncmp("A-", keycmd, 2) == 0) {
	*i_state =  I_STATE_META;
	keycmd += 2;
    } else if (strncmp("K-", keycmd, 2) == 0) {
	*i_state = I_STATE_K;
	keycmd += 2;
    } else {
	*i_state =  0;
    }

    if (strcmp(keycmd, "<ret>") == 0) {
	*key = SDLK_RETURN;
    } else if (strcmp(keycmd, "<esc>") == 0) {
	*key = SDLK_ESCAPE;
    } else if (strcmp(keycmd, "<spc>") == 0) {
	*key = SDLK_SPACE;
    } else if (strcmp(keycmd, "<del>") == 0) {
	*key = SDLK_BACKSPACE;
    } else if (strcmp(keycmd, "<tab>") == 0) {
	*key = SDLK_TAB;
    } else if (strcmp(keycmd, "<up>") == 0) {
	*key = SDLK_UP;
    } else if (strcmp(keycmd, "<down>") == 0) {
        *key = SDLK_DOWN;
    } else if (strcmp(keycmd, "<left>") == 0) {
	/* fprintf(stdout, "SETTING KEY TO %d\n", SDLK_LEFT); */
	/* exit(0); */
	*key = SDLK_LEFT;
    } else if (strcmp(keycmd, "<right>") == 0) {
	*key = SDLK_RIGHT;
    } else {
	*key = * (char *) keycmd;
    }
    
}

char *input_get_keycmd_str(uint16_t i_state, SDL_Keycode keycode)
{
    char buf[32];
    memset(buf, '\0', 32);
    const char *mod;
    switch (i_state) {
    case (0):
	mod = "";
	break;
    case (I_STATE_CMDCTRL):
	mod = "C-";
	break;
    case (I_STATE_SHIFT):
	mod = "S-";
	break;
    case (I_STATE_CMDCTRL | I_STATE_SHIFT):
	mod = "C-S-";
	break;
    case I_STATE_META:
	mod = "A-";
	break;
    case (I_STATE_META | I_STATE_SHIFT):
	mod = "A-S-";
	break;
    case (I_STATE_META | I_STATE_CMDCTRL):
	mod = "C-A-";
	break;
    case (I_STATE_K):
	mod = "K-";
	break;
    default:
	mod = "";
	break;
    }

    switch (keycode) {
    case SDLK_RETURN:
	snprintf(buf, sizeof(buf),"%s<ret>", mod);
	break;
    case SDLK_SPACE:
	snprintf(buf, sizeof(buf), "%s<spc>", mod);
	break;
    case SDLK_ESCAPE:
	snprintf(buf, sizeof(buf), "%s<esc>", mod);
	break;
    case SDLK_BACKSPACE:
	snprintf(buf, sizeof(buf), "%s<del>", mod);
	break;
    case SDLK_TAB:
	snprintf(buf, sizeof(buf), "%s<tab>", mod);
	break;
    case SDLK_UP:
	snprintf(buf, sizeof(buf), "%s<up>", mod);
	break;
    case SDLK_DOWN:
	snprintf(buf, sizeof(buf), "%s<down>", mod);
	break;
    case SDLK_LEFT:
	snprintf(buf, sizeof(buf), "%s<left>", mod);
	break;
    case SDLK_RIGHT:
	snprintf(buf, sizeof(buf), "%s<right>", mod);
	break;
    default:
	snprintf(buf, sizeof(buf), "%s%c", mod, keycode);
    }

    char *ret = malloc(strlen(buf) + 1);
    strcpy(ret, buf);
    /* fprintf(stdout, "\t->keycmd str %s (%s)\n", ret, buf); */
    return ret;
}

/* Returns null if no function found by that id */
UserFn *input_get_fn_by_id(char *id, InputMode im)
{
    Mode *mode = modes[im];
    for (uint8_t s=0; s<mode->num_subcats; s++) {
	ModeSubcat *sc = mode->subcats[s];
	for (uint8_t f=0; f<sc->num_fns; f++) {
	    UserFn *fn = sc->fns[f];
	    if (strcmp(id, fn->fn_id) == 0) {
		return fn;
	    }
	}
    }
    return NULL;
}

void input_bind_fn(UserFn *fn, uint16_t i_state, SDL_Keycode keycode, InputMode mode)
{
    int hash = input_hash(i_state, keycode);
    /* fprintf(stdout, "Binding input %s in mode %s. Root: %p\n", input_get_keycmd_str(i_state, keycode), input_mode_str(mode), &input_hash_table[hash]); */
    KeybNode *keyb_node = input_hash_table[hash];
    /* KeybNode *last = NULL; */
    Keybinding *kb = malloc(sizeof(Keybinding));
    /* UserFn *user_fn = NULL; */
    if (!keyb_node) {
	/* fprintf(stdout, "\t->first slot empty\n"); */
	keyb_node = malloc(sizeof(KeybNode));
	keyb_node->kb = kb;
	keyb_node->next = NULL;
	input_hash_table[hash] = keyb_node;
    } else {
	while (keyb_node->kb->mode != mode || keyb_node->kb->i_state != i_state || keyb_node->kb->keycode != keycode) {
	    if (keyb_node->next) {
		/* fprintf(stdout, "\t->slot %p taken, next...\n", &keyb_node); */
		/* last = keyb_node; */
		keyb_node = keyb_node->next;
	    } else {
		keyb_node->next = malloc(sizeof(KeybNode));
		keyb_node = keyb_node->next;
		/* fprintf(stdout, "\t->inserting at %p\n", &keyb_node); */
		/* kb = malloc(sizeof(Keybinding)); */
		keyb_node->kb = kb;
		keyb_node->next = NULL;
		break;
	    }   
	}
    }
    kb->mode = mode;
    kb->i_state = i_state;
    kb->keycode = keycode;
    kb->keycmd_str = input_get_keycmd_str(i_state, keycode);
    if (fn->annotation[0] == '\0') {
	strcat(fn->annotation, kb->keycmd_str);
    } else {
	strcat(fn->annotation, "\t/\t");
	strcat(fn->annotation, kb->keycmd_str);
    }
    /* if (!fn->annotation) { */
    /* 	/\* fprintf(stdout, "binding fn to %s\n", kb->keycmd_str); *\/ */
    /* 	fn->annotation = kb->keycmd_str; */
    /* } else { */
    /* 	strcat((char *)fn->annotation, kb->keycmd_str); */
    /* } */
    kb->fn = fn;
    /* if (last) { */
    /* 	last->next = keyb_node; */
    /* }  */
}


static int check_next_line_indent(FILE *f)
{
    char c;
    int ret = 0;
    while (fgetc(f) != '\n') {}
    /* while (is_whitespace_char((c = fgetc(f)))) { */
    while (1) {
	c = fgetc(f);
	if (c == EOF) {
	    return -1;
	}
	if (!is_whitespace_char(c)) {
	    ungetc(c, f);
	    return ret;
	}
	ret++;
    }
    ungetc(c, f);
    
}

static Layout *create_menu_layout()
{
    Layout *menu_lt = layout_add_child(main_win->layout);
    layout_set_default_dims(menu_lt);
    menu_lt->w.value = 1200.0f;
    layout_reset(menu_lt);
    return menu_lt;
}

static void create_menu_from_mode_subcat(void *sc_v)
{
    ModeSubcat *sc = (ModeSubcat *)sc_v;
    Layout *m_layout = create_menu_layout();
    Menu *m = menu_create(m_layout, main_win);
    MenuColumn *c = menu_column_add(m, sc->name);
    MenuSection *sctn = menu_section_add(c, "");
    for (uint8_t i=0; i<sc->num_fns; i++) {
	UserFn *fn = sc->fns[i];
	menu_item_add(sctn, fn->fn_display_name, fn->annotation, fn->do_fn, NULL);
    }
    menu_add_header(m, sc->name, "n  -  next item\np  -  previous item\nh  -  go back (dismiss)\n<ret>  -  select item");
    window_add_menu(main_win, m);
    /* if (main_win->modes[main_win->num_modes - 1] != MENU_NAV) { */
    /* 	window_push_mode(main_win, MENU_NAV); */
    /* } */
}

void input_create_menu_from_mode(InputMode im)
{
    Mode *mode = modes[im];
    if (!mode) {
	fprintf(stderr, "Error: mode %s not initialized\n", input_mode_str(im));
    }
    if (mode->num_subcats == 1) {
	create_menu_from_mode_subcat(mode->subcats[0]);
    } else {
	Layout *m_layout = create_menu_layout();
	Menu *m = menu_create(m_layout, main_win);
	MenuColumn *c = menu_column_add(m, "");
	MenuSection *sctn = menu_section_add(c, "");
	for (uint8_t i=0; i<mode->num_subcats; i++) {
	    ModeSubcat *sc = mode->subcats[i];
	    menu_item_add(sctn, sc->name, ">", create_menu_from_mode_subcat, sc);
	}
	menu_add_header(m, "", "n  -  next item\np  -  previous item\nh  -  go back (dismiss)\n<ret>  -  select item");
	window_add_menu(main_win, m);
	/* if (main_win->modes[main_win->num_modes - 1] != MENU_NAV) { */
	/*     window_push_mode(main_win, MENU_NAV); */
	/* } */
    }
    /* return NULL; */
}




/* Menu *input_create_menu_from_mode(InputMode im) */
/* { */
/*     Mode *mode = modes[im]; */
/*     if (!mode) { */
/* 	fprintf(stderr, "Error: mode %s not initialized\n", input_mode_str(im)); */
/* 	exit(1); */
/*     } */
/*     Layout *m_layout = create_menu_layout(); */
/*     if (!m_layout) { */
/* 	fprintf(stderr, "Error: Unable to create menu layout\n"); */
/* 	exit(1); */
/*     } */
/*     Menu *m = menu_create(m_layout, main_win); */
/*     for (int i=0; i<mode->num_subcats; i++) { */
/* 	ModeSubcat *sc = mode->subcats[i]; */
/* 	MenuColumn *c = menu_column_add(m, sc->name); */
/* 	MenuSection *sctn = menu_section_add(c, ""); */
/* 	for (int j=0; j<sc->num_fns; j++) { */
/* 	    UserFn *fn = sc->fns[j]; */
/* 	    menu_item_add(sctn, fn->fn_display_name, fn->annotation, fn->do_fn, NULL); */
/* 	} */
/*     } */
/*     menu_add_header(m, mode->name, "Here are functions available to you in aforementioned mode."); */
/*     return m; */
/* } */


Menu *input_create_master_menu()
{
    /* InputMode im = GLOBAL; */

    Layout *m_layout = create_menu_layout();
    if (!m_layout) {
	fprintf(stderr, "Error: Unable to create menu layout\n");
	exit(1);
    }
    Menu *m = menu_create(m_layout, main_win);
    /* while (im < NUM_INPUT_MODES) { */
    for (uint8_t i=0; i<main_win->num_modes + 1; i++) {
	InputMode im;
	if (i == 0) {
	    im = GLOBAL;
	} else {
	    im = main_win->modes[i - 1];

	}
	if (im == MENU_NAV) continue;
	Mode *mode = modes[im];
	if (!mode) {
	    fprintf(stderr, "Error: mode %s not initialized\n", input_mode_str(im));
	    exit(1);
	}
	/* Layout *m_layout = create_menu_layout(); */
	/* if (!m_layout) { */
	/*     fprintf(stderr, "Error: Unable to create menu layout\n"); */
	/*     exit(1); */
	/* } */
	if (mode->num_subcats == 1) {
	    MenuColumn *c = menu_column_add(m, "");
	    MenuSection *sctn = menu_section_add(c, mode->name);
	    for (uint8_t i=0; i<mode->subcats[0]->num_fns; i++) {
		UserFn *fn = mode->subcats[0]->fns[i];
		menu_item_add(sctn, fn->fn_display_name, fn->annotation, fn->do_fn, NULL);
	    }
	} else {
	    MenuColumn *c = menu_column_add(m, "");
	    for (int i=0; i<mode->num_subcats; i++) {
		ModeSubcat *sc = mode->subcats[i];
	    
		MenuSection *sctn = menu_section_add(c, sc->name);
		menu_item_add(sctn, sc->name, ">", create_menu_from_mode_subcat, sc);
	    }
	    /* for (int j=0; j<sc->num_fns; j++) { */
	    /* 	UserFn *fn = sc->fns[j]; */
	    /* 	menu_item_add(sctn, fn->fn_display_name, fn->annotation, fn->do_fn, NULL); */
	    /* } */
	/* } */
	}
    }

    menu_add_header(m, "", "n - next item\np - previous item\nl - column right\nj - column left\n<ret> - select\nm - dismiss");
    layout_center_agnostic(m_layout, true, true);
    menu_reset_layout(m);
    return m;
}

void input_load_keybinding_config(const char *filepath)
{
    FILE *f = fopen(filepath, "r");
    if (!f) {
	fprintf(stderr, "Error: failed to open file at %s\n", filepath);
	return;
    }
    char c;
    int i=0;
    char buf[255];
    
    /* Get through comments */
    bool comment = true;
    while (comment) {
	while ((c = fgetc(f)) != '\n') {}
	if ((c = fgetc(f)) != '#' && c != '\n') {
	    comment = false;
	}
    }
    
    bool more_modes = true;

    while (more_modes) {
	/* Get mode name */

	while ((c = fgetc(f)) == '-' || c == ' ' || c == '\t' || c == '\n') {}
	/* ungetc(c, f); */
	i = 0;
	do {
	    buf[i] = c;
	    i++;
	    c = fgetc(f);
	} while (c != ':');
	buf[i] = '\0';
	
	InputMode mode = input_mode_from_str(buf);
	if (mode == -1) {
	    fprintf(stderr, "Error: no mode under name %s\n", buf);
	    return;
	    /* exit(1); */
	}
	

	bool more_bindings = true;

	while (more_bindings) {
	    
	    uint16_t i_state = 0;
	    SDL_Keycode key = 0;
	    UserFn *fn = NULL;


	    
	    int next_line_indent;
	    
	    if ((next_line_indent = check_next_line_indent(f)) < 0) {
		more_modes = false;
		break;
	    } else if (next_line_indent == 0) {
		break;
	    }
	    
	    while ((c = fgetc(f)) == ' ' || c == '\t' || c == '\n') {}
	    if (c != '-') {
		fprintf(stderr, "YAML parse error; expected '-' and got '%c'\n", c);
		exit(1);
	    }
	    while ((c = fgetc(f)) == ' ' || c == '\t') {}
	    
	    /* Get input pattern */
	    i = 0;
	    do {
		buf[i] = c;
		i++;
		c = fgetc(f);
	    } while (c != ':' && c != ' ' && c != '\t');
	    buf[i] = '\0';
	    input_read_keycmd(buf, &i_state, &key);
	    
	    while ((c = fgetc(f)) == ' ' || c == '\t' || c == ':') {}

	    /* Get fn id*/
	    i = 0;
	    do {
		buf[i] = c;
		i++;
		c = fgetc(f);
	    } while (c != '\n' && c != EOF);

	    if (c == EOF) {
		more_bindings = false;
		more_modes = false;
	    }
	    buf[i] = '\0';
	    ungetc(c, f);
	    fn = input_get_fn_by_id(buf, mode);

	    if (fn) {
		input_bind_fn(fn, i_state, key, mode);
	    } else {
		fprintf(stderr, "Error: no function found with id %s\n", buf);
	    }
	}

    }
}


void input_quit()
{
    KeybNode *node = NULL;
    for (int i=0; i<INPUT_HASH_SIZE; i++) {
	node = input_hash_table[i];
	KeybNode *nodes[64];
	uint8_t num_nodes = 0;
	while (node) {
	    nodes[num_nodes] = node;
	    num_nodes++;
	    node = node->next;
	}
	while (num_nodes > 0) {
	    node = nodes[num_nodes - 1];
	    free(node->kb->keycmd_str);
	    free(node->kb);
	    free(node);
	    num_nodes--;
	}
	
    }
    for (uint8_t i=0; i<NUM_INPUT_MODES; i++) {
	Mode *mode = modes[i];
	for (uint8_t j=0; j<mode->num_subcats; j++) {
	    ModeSubcat *sc = mode->subcats[j];
	    for (uint8_t k=0; k<sc->num_fns; k++) {
		UserFn *fn = sc->fns[k];
		free(fn);
	    }
	    free(sc);
	}
	free(mode);
    }
}

void input_create_function_reference()
{
    fprintf(stdout, "Creating function reference at %s\n", FN_REFERENCE_FILENAME);
    FILE *f = fopen(FN_REFERENCE_FILENAME, "w");
    if (!f) {
	fprintf(stderr, "Error: unable to open file %s\n", FN_REFERENCE_FILENAME);
    }
    for (uint8_t i=0; i<NUM_INPUT_MODES; i++) {
	Mode *mode = modes[i];
	fprintf(f, "### %s mode\n", mode->name);
	for (uint8_t j=0; j<mode->num_subcats; j++) {
	    ModeSubcat *sc = mode->subcats[j];
	    if (mode->num_subcats > 1) {
		fprintf(f, "#### %s\n", sc->name);
	    }
	    for (uint8_t k=0; k<sc->num_fns; k++) {
		UserFn *fn = sc->fns[k];
		char buf[255];
		char *c = fn->annotation;
		int i=0;
		/* bool had_multiple = false; */
		strncpy(buf, "<kbd>", 5);
		i += 5;
		while (*c != '\0') {
		    if (*c == '\t') {
			/* if (had_multiple) { */
			/*     strncpy(buf + i, ", <kbd>", 5); */
			/*     i+=5; */
			/* } */
			strncpy(buf + i, "</kbd>, <kbd>", 13);
			i+=13;
			/* had_multiple = true; */
			c+=3;
		    } else if (*c == '<') {
			strncpy(buf + i, "\\<", 2);
			i+=2;
			c++;
		    } else if (*c == '>') {
			strncpy(buf + i, "\\>", 2);
			i+=2;
			c++;
		    } else {
			strncpy(buf + i, c, 1);
			c++;
			i++;
		    }
	
		}
		buf[i] = '\0';
		fprintf(f, "- %s : %s</kbd>\n", fn->fn_display_name, buf);
		/* fprintf(f, "     - <kbd>%s</kbd>\n", fn->annotation); */
	    }
	}
    }

    fclose(f);
}
