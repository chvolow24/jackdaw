 #include <stdio.h>
#include <sys/param.h>
#include "SDL_events.h"
#include "audio_connection.h"
#include "autocompletion.h"
#include "dir.h"
#include "endpoint.h"
#include "function_lookup.h"
#include "project_endpoint_ops.h"
#include "input.h"
/* #include "loading */
#include "menu.h"
#include "modal.h"
#include "panel.h"
#include "project.h"
#include "settings.h"
#include "status.h"
#include "test.h"
#include "text.h"
#include "textbox.h"
#include "transport.h"
#include "timeline.h"
#include "wav.h"

#define MENU_MOVE_BY 40
#define TL_DEFAULT_XSCROLL 60
#define SLOW_PLAYBACK_SPEED 0.2f

#define ACTIVE_TL (proj->timelines[proj->active_tl_index])
/* #define ACTIVE_TRACK(timeline) (tl->tracks[tl->track_selector]) */
#define TRACK_AUTO_SELECTED(track) (track->num_automations != 0 && track->selected_automation != -1)
#define TABVIEW_BLOCK(str)	    \
    if (main_win->active_tabview) { \
	status_set_errstr("Cannot " #str " when tabview active"); \
	return; \
    }

extern Window *main_win;
extern Project *proj;
extern Mode **modes;

extern char DIRPATH_SAVED_PROJ[MAX_PATHLEN];
extern char DIRPATH_OPEN_FILE[MAX_PATHLEN];
extern char DIRPATH_EXPORT[MAX_PATHLEN];

/* extern SDL_Color color_global_black; */
/* extern SDL_Color color_global_grey; */
/* extern SDL_Color color_global_white; */
/* extern SDL_Color control_bar_bckgrnd; */

extern SDL_Color color_global_light_grey;
extern SDL_Color color_global_quickref_button_pressed;
extern SDL_Color color_global_quickref_button_blue;
/* extern SDL_Color color_global_white; */

#define NO_TRACK_ERRSTR "No track. Add new with C-t"

int quickref_button_press_callback(void *self_v, void *target)
{
    /* Timeline *tl = proj->timelines[proj->active_tl_index]; */
    Timeline *tl = ACTIVE_TL;
    tl->needs_redraw = true;
    return 0;
}

void jdaw_write_project(const char *path);

void user_global_menu(void *nullarg)
{
    if (main_win->num_modes <= 0) {
	status_set_errstr("Error: no modes active in user_global_expose_help");
	fprintf(stderr, "Error: no modes active in user_global_expose_help\n");
	return;
    }
    Menu *new = input_create_master_menu();
    window_add_menu(main_win, new);
}

static int quit_yes_action(void *self, void *xarg)
{
    window_pop_modal(main_win);
    main_win->i_state |= I_STATE_QUIT;
    return 0;
}

void user_modal_dismiss(void *nullarg);
static int quit_no_action(void *self, void *xarg)
{
    proj->quit_count = 0;
    /* user_modal_dismiss(NULL); */
    if (main_win->num_modals > 0)
	window_pop_modal(main_win);
    Timeline *tl = ACTIVE_TL;
    tl->needs_redraw = true;

    return 0;
}

void user_tl_activate_source_mode(void *nullarg);
    
/* Escape a confusing or illegal program state */
void user_global_escape(void *nullarg)
{
    if (proj->source_mode) {
	user_tl_activate_source_mode(NULL);
    } else if (main_win->num_modes > 1) {
	window_pop_mode(main_win);
    } else if (main_win->modes[0] != TIMELINE) {
	main_win->modes[0] = TIMELINE;
	main_win->num_modes = 1;
    }
}

void user_global_quit(void *nullarg)
{
    if (proj->quit_count == 0) {
	Layout *lt = layout_add_child(main_win->layout);
	layout_set_default_dims(lt);
	Modal *m = modal_create(lt);
	modal_add_header(m, "Really quit?", &color_global_light_grey, 3);
	modal_add_button(m, "Yes", quit_yes_action);
	modal_add_button(m, "No", quit_no_action);
	m->x->action = quit_no_action;
	modal_reset(m);
	window_push_modal(main_win, m);
    } else if (proj->quit_count > 1) {
	quit_yes_action(NULL, NULL);
    }
    proj->quit_count++;

    /* exit(0); //TODO: add the i_state to window (or proj?) to quit naturally. */
}

void user_global_undo(void *nullarg)
{
    user_event_do_undo(&proj->history);
    timeline_reset(ACTIVE_TL, false);
}

void user_global_redo(void *nullarg)
{
    user_event_do_redo(&proj->history);
    timeline_reset(ACTIVE_TL, false);
}

void user_global_show_output_freq_domain(void *nullarg)
{
    /* proj->show_output_freq_domain = !proj->show_output_freq_domain; */
    panel_page_refocus(proj->panels, "Output spectrum", 0);
}


static int submit_server_form(void *mod_v, void *target)
{
    int port = atoi((char *)((Modal *)mod_v)->stashed_obj);
    fprintf(stderr, "STARTING SERVER ON PORT: %d\n", port);
    if (api_start_server(proj, port) == 0) {
	window_pop_modal(main_win);
	ACTIVE_TL->needs_redraw = true;
	return 0;
    } else {
	ACTIVE_TL->needs_redraw = true;
	status_set_errstr("Unable to start server; port may be in use");
    }
    return 0;
}

txt_int_range_completion(1023, 49152);
void user_global_start_server(void *nullarg)
{

    Layout *mod_lt = layout_add_child(main_win->layout);
    layout_set_default_dims(mod_lt);
    Modal *m = modal_create(mod_lt);
    modal_add_header(m, "Start server", &color_global_light_grey, 3);
    modal_add_header(m, "Run a UDP server on port:", &color_global_light_grey, 5);
    static char port[6] = {'9', '3', '0', '2'};
    modal_add_textentry(
	m,
	port,
	6,
	txt_integer_validation,
	txt_int_range_completion_1023_49152,
	NULL);
    modal_add_button(m, "Start", submit_server_form);
    m->stashed_obj = port;
    m->submit_form = submit_server_form;
    window_push_modal(main_win, m);
    modal_reset(m);
    /* fprintf(stdout, "about to call move onto\n"); */
    modal_move_onto(m);


}


int path_updir_name(char *pathname);
static int submit_save_as_form(void *mod_v, void *target)
{
    Modal *modal = (Modal *)mod_v;
    char *name;
    char *dirpath;
    ModalEl *el;
    for (uint8_t i=0; i<modal->num_els; i++) {
	switch ((el = modal->els[i])->type) {
	case MODAL_EL_TEXTENTRY:
	    name = ((TextEntry *)el->obj)->tb->text->value_handle;
	    break;
	case MODAL_EL_DIRNAV: {
	    DirNav *dn = (DirNav *)el->obj;
	    /* DirPath *dir = (DirPath *)dn->lines->items[dn->current_line]->obj; */
	    dirpath = dn->current_path_tb->text->value_handle;
	    break;
	}
	default:
	    break;
	}
    }
    char buf[255];
    memset(buf, '\0', sizeof(buf));
    strcat(buf, dirpath);
    strcat(buf, "/");
    strcat(buf, name);
    fprintf(stdout, "SAVE AS: %s\n", buf);
    jdaw_write_project(buf);
    char *last_slash_pos = strrchr(buf, '/');
    if (last_slash_pos) {
	*last_slash_pos = '\0';
	char *realpath_ret;
	fprintf(stdout, "Real path of \"%s\" called on %p, w current val: %s\n", dirpath, DIRPATH_SAVED_PROJ, DIRPATH_SAVED_PROJ);
	if (!(realpath_ret = realpath(dirpath, NULL))) {
	    perror("Error in realpath:");
	} else {
	    fprintf(stdout, "Realpath returned: %s\n", realpath_ret);
	    strncpy(DIRPATH_SAVED_PROJ, realpath_ret, MAX_PATHLEN);
	    free(realpath_ret);
	}
	fprintf(stdout, "Resolved path: %s\n", DIRPATH_SAVED_PROJ);
    }
    window_pop_modal(main_win);
    Timeline *tl = ACTIVE_TL;
    tl->needs_redraw = true;
    return 0;
}

static int dir_to_tline_filter_save(void *dp_v, void *dn_v)
{
    DirPath *dp = *(DirPath **)dp_v;
    if (strcmp(dp->path, ".") == 0) return 0;
    if (dp->hidden) return 0;
    if (dp->type != DT_DIR) {
	char *dotpos = strrchr(dp->path, '.');
	if (!dotpos) {
	    return 0;
	}
	char *ext = dotpos + 1;
	if (strcmp(ext, "wav") == 0 ||
	    strcmp(ext, "jdaw") == 0 ||
	    strcmp(ext, "WAV") == 0 ||
	    strcmp(ext, "JDAW") == 0) {
	    return 1;
	} else {
	    return 0;
	}

    }
    return 1;
}

static int dir_to_tline_filter_open(void *dp_v, void *dn_v)
{
    DirPath *dp = *(DirPath **)dp_v;
    if (dp->hidden) {
	return 0;
    }

    if (dp->type != DT_DIR) {
	char *dotpos = strrchr(dp->path, '.');
	if (!dotpos) {
	    return 0;
	}
	char *ext = dotpos + 1;
	if (strncmp("wav", ext, 3) *
	    strncmp("jdaw", ext, 4) *
	    strncmp("WAV", ext, 3) *
	    strncmp("JDAW", ext, 4) *
	    strncmp("bak", ext, 3) == 0) {
	    return 1;
	}
	return 0;
    } else {
	return 1;
    }
    /* else { */
    /* 	if (strcmp(dp->path, ".") == 0) { */
    /* 	    fprintf(stderr, "(fail) dp path: %s\n", dp->path); */
    /* 	    return 1; */
    /* 	} */
    /* } */
    /* return 0; */
    
}

static int file_ext_completion_wav(Text *txt, void *obj)
{
    char *dotpos = strrchr(txt->display_value, '.');
    int retval = 0;
    /* fprintf(stdout, "COMPLETION %s\n", dotpos); */
    if (!dotpos) {
	strcat(txt->display_value, ".wav");
	txt->len = strlen(txt->display_value);
	txt->cursor_end_pos = txt->len;
	txt_reset_drawable(txt);
	retval = 0;
    } else if (strcmp(dotpos, ".wav") != 0 && (strcmp(dotpos, ".wav") != 0)) {
	retval = 1;
    }
    if (retval == 1) {
	txt->cursor_start_pos = dotpos + 1 - txt->display_value;
	txt->cursor_end_pos = txt->len;
	status_set_errstr("Export file must have \".wav\" extension");
    }
    return retval;
}

static int file_ext_completion_jdaw(Text *txt, void *obj)
{
    char *dotpos = strrchr(txt->display_value, '.');
    int retval = 0;
    /* fprintf(stdout, "COMPLETION %s\n", dotpos); */
    if (!dotpos) {
	strcat(txt->display_value, ".jdaw");
	txt->len = strlen(txt->display_value);
	txt->cursor_end_pos = txt->len;
	txt_reset_drawable(txt);
	retval = 0;
    } else if (strcmp(dotpos, ".jdaw") != 0 && (strcmp(dotpos, ".JDAW") != 0)) {
	retval = 1;
    }
    if (retval == 1) {
	txt->cursor_start_pos = dotpos + 1 - txt->display_value;
	txt->cursor_end_pos = txt->len;
	status_set_errstr("Project file must have \".jdaw\" extension");
    }
    return retval;
}
void user_global_save_project(void *nullarg)
{
    Layout *mod_lt = layout_add_child(main_win->layout);
    layout_set_default_dims(mod_lt);
    Modal *save_as = modal_create(mod_lt);
    modal_add_header(save_as, "Save as:", &color_global_light_grey, 3);
    modal_add_header(save_as, "Project name:", &color_global_light_grey, 5);
    modal_add_textentry(
	save_as,
	proj->name,
	MAX_NAMELENGTH,
	txt_name_validation,
	file_ext_completion_jdaw,
	NULL);
    modal_add_p(save_as, "\t\t|\t\t<tab>\tv\t\t|\t\t\tS-<tab>\t^\t\t|\t\tC-<ret>\tSubmit (save as)\t\t|", &color_global_light_grey);
    modal_add_header(save_as, "Project location:", &color_global_light_grey, 5);
    modal_add_dirnav(save_as, DIRPATH_SAVED_PROJ, dir_to_tline_filter_save);
    modal_add_button(save_as, "Save", submit_save_as_form);
    save_as->submit_form = submit_save_as_form;
    window_push_modal(main_win, save_as);
    modal_reset(save_as);
    /* fprintf(stdout, "about to call move onto\n"); */
    modal_move_onto(save_as);
}

static NEW_EVENT_FN(undo_load_wav, "undo load wav")
    ClipRef *cr = (ClipRef *)obj1;
    clipref_delete(cr);
}

static NEW_EVENT_FN(redo_load_wav, "redo load wav")
    ClipRef *cr = (ClipRef *)obj1;
    clipref_undelete(cr);
}

static NEW_EVENT_FN(dispose_forward_load_wav, "")
    ClipRef *cr = (ClipRef *)obj1;
    clipref_destroy_no_displace(cr);
}


Project *jdaw_read_file(char *path);
static void openfile_file_select_action(DirNav *dn, DirPath *dp)
{
    char *dotpos = strrchr(dp->path, '.');
    if (!dotpos) {
	status_set_errstr("Cannot open file without a .jdaw or .wav extension");
	fprintf(stderr, "Cannot open file without a .jdaw or .wav extension\n");
	return;
    }
    char *ext = dotpos + 1;
    /* fprintf(stdout, "ext char : %c\n", *ext); */
    if (strcmp("wav", ext) * strcmp("WAV", ext) == 0) {
	fprintf(stdout, "Wav file selected\n");
	Timeline *tl = ACTIVE_TL;
	if (!tl) return;
	Track *track = timeline_selected_track(tl);
	if (!track) {
	    status_set_errstr("Error: at least one track must exist to load a wav file");
	    fprintf(stderr, "Error: at least one track must exist to load a wav file\n");
	    return;
	}
	ClipRef *cr = wav_load_to_track(track, dp->path, tl->play_pos_sframes);
	if (!cr) {
	    Timeline *tl = ACTIVE_TL;
	    tl->needs_redraw = true;
	    window_pop_modal(main_win);
	    return;
	}
	Value nullval = {.int_v = 0};
	user_event_push(
	    &proj->history,
	    undo_load_wav,
	    redo_load_wav,
	    NULL, dispose_forward_load_wav,
	    (void *)cr, NULL,
	    nullval, nullval, nullval, nullval,
	    0, 0, false, false);
	    
	
    } else if (strcmp("jdaw", ext) * strcmp("JDAW", ext) * strcmp("bak", ext) == 0) {
	fprintf(stdout, "Jdaw file selected\n");
	if (proj->recording) transport_stop_recording();
	else if (proj->playing) transport_stop_playback();
	api_quit(proj);
	Project *new_proj = jdaw_read_file(dp->path);
	if (new_proj) {
	    project_destroy(proj);
	    proj = new_proj;
	    /* layout_destroy(main_win->layout); */
	    main_win->layout = new_proj->layout;
	    layout_reset(main_win->layout);
	    /* window_resize_passive(main_win, , main_win->h_pix); */
	    /* layout_force_reset(new_proj->layout); */

	} else {
	    status_set_errstr("Error opening jdaw project");
	}
	timeline_reset_full(proj->timelines[0]);
    }
    char *last_slash_pos = strrchr(dp->path, '/');
    if (last_slash_pos) {
	*last_slash_pos = '\0';
	char *realpath_ret;
	if (!(realpath_ret = realpath(dp->path, NULL))) {
	    perror("Error in realpath");
	} else {
	    strncpy(DIRPATH_OPEN_FILE, realpath_ret, MAX_PATHLEN);
	    free(realpath_ret);
	}
    }
    window_pop_modal(main_win);
    Timeline *tl = ACTIVE_TL;
    tl->needs_redraw = true;
}

void user_global_open_file(void *nullarg)
{
    Layout *mod_lt = layout_add_child(main_win->layout);
    layout_set_default_dims(mod_lt);
    Modal *openfile = modal_create(mod_lt);
    modal_add_header(openfile, "Open file:", &color_global_light_grey, 3);
    /* modal_add_header(openfile, "Project name:", &color_global_light_grey, 5); */
    /* modal_add_textentry(openfile, proj->name); */
    /* modal_add_p(openfile, "\t\t\t^\t\tS-p (S-d)\t\t\t\tv\t\tS-n (S-d)", &color_global_black); */
    /* modal_add_header(openfile, "Project location:", &color_global_light_grey, 5); */
    ModalEl *dirnav_el = modal_add_dirnav(openfile, DIRPATH_OPEN_FILE, dir_to_tline_filter_open);
    DirNav *dn = (DirNav *)dirnav_el->obj;
    dn->file_select_action = openfile_file_select_action;
    /* openfile->submit_form = submit_openfile_form; */
    window_push_modal(main_win, openfile);
    modal_reset(openfile);
}

void user_global_function_lookup(void *nullarg)
{
    function_lookup();
}

/* void user_global_start_or_stop_screenrecording(void *nullarg) */
/* { */
/*     main_win->screenrecording = !main_win->screenrecording; */
/* } */

void user_global_chaotic_user_test(void *nullarg)
{
    proj->do_tests = true;
}

void api_node_print_all_routes(APINode *node);
void user_global_api_print_all_routes(void *nullarg)
{
    api_node_print_all_routes(&proj->server.api_root);
}

static void menu_nav_mode_error()
{
    fprintf(stderr, "ERROR: in mode menu_nav, no menu on main window");
    breakfn();
    window_pop_mode(main_win);
}

void user_menu_nav_next_item(void *nullarg)
{
    Menu *m = window_top_menu(main_win);
    if (!m) {
	menu_nav_mode_error();
	return;
    }

    if (m->sel_col == 255) {
	m->sel_col = 0;
    }
    MenuColumn *c = m->columns[m->sel_col];
    if (c->sel_sctn == 255) {
	c->sel_sctn = 0;
    }
    MenuSection *s = c->sections[c->sel_sctn];
    if (s->sel_item == 255) {
	s->sel_item = 0;
    } else if (s->sel_item < s->num_items - 1) {
	s->items[s->sel_item]->selected = false;
	s->sel_item++;
	s->items[s->sel_item]->selected = true;
    } else if (c->sel_sctn < c->num_sections - 1) {
	c->sel_sctn++;
	s = c->sections[c->sel_sctn];
	s->sel_item = 0;
    }
}

void user_menu_nav_prev_item(void *nullarg)
{
    Menu *m = window_top_menu(main_win);
    if (!m) {
	menu_nav_mode_error();
	return;
    }
    if (m->sel_col == 255) {
	m->sel_col = 0;
    }
    MenuColumn *c = m->columns[m->sel_col];
    if (c->sel_sctn == 255) {
	c->sel_sctn = 0;
    }
    MenuSection *s = c->sections[c->sel_sctn];
    if (s->sel_item == 255) {
	s->sel_item = 0;
    } else if (s->sel_item > 0) {
	/* s->items[s->sel_item]->selected = false; */
	s->sel_item--;
	/* s->items[s->sel_item]->selected = true; */
    } else if (c->sel_sctn > 0) {
	c->sel_sctn--;
	
    }
}


void user_menu_nav_next_sctn(void *nullarg)
{

}

void user_menu_nav_prev_sctn(void *nullarg)
{

}

void user_menu_nav_column_right(void *nullarg)
{
    Menu *m = window_top_menu(main_win);
    if (!m) {
	menu_nav_mode_error();
	return;
    }
    if (m->sel_col < m->num_columns - 1) {
	fprintf(stdout, "Sel col to inc: %d, num: %d\n", m->sel_col, m->num_columns);
	m->sel_col++;
	MenuColumn *c = m->columns[m->sel_col];
	if (c->sel_sctn == 255) {
	    c->sel_sctn = 0;
	    MenuSection *s = c->sections[c->sel_sctn];
	    if (s->sel_item == 255) {
		s->sel_item = 0;
	    }
	}
	
    }
}

void user_menu_nav_column_left(void *nullarg)
{
    Menu *m = window_top_menu(main_win);
    if (!m) {
	menu_nav_mode_error();
	return;
    }
    if (m->sel_col == 255) {
	m->sel_col = 0;
    } else if (m->sel_col > 0) {
	m->sel_col--;
    }
}

void user_menu_nav_choose_item(void *nullarg)
{
    Menu *m = window_top_menu(main_win);
    if (!m) {
	fprintf(stderr, "No menu on main window\n");	
	exit(1);
    }
    if (m->sel_col < m->num_columns) {
	MenuColumn *col = m->columns[m->sel_col];
	if (col->sel_sctn < col->num_sections) {
	    MenuSection *sctn = col->sections[col->sel_sctn];
	    if (sctn->sel_item < sctn->num_items) {
		MenuItem *item = sctn->items[sctn->sel_item];
		if (item->onclick != user_menu_nav_choose_item) {
		    item->onclick(item->target);
		    /* window_pop_menu(main_win); */
		}
	    }
	}
    }
}

void user_menu_translate_up(void *nullarg)
{
    Menu *m = window_top_menu(main_win);
    if (!m) {
	fprintf(stderr, "No menu on main window\n");	
	exit(1);
    }
    menu_translate(m, 0, -1 * MENU_MOVE_BY);
    Timeline *tl = ACTIVE_TL;
    tl->needs_redraw = true;
}

void user_menu_translate_down(void *nullarg)
{
    Menu *m = window_top_menu(main_win);
    if (!m) {
	fprintf(stderr, "No menu on main window\n");	
	exit(1);
    }
    menu_translate(m, 0, MENU_MOVE_BY);
    Timeline *tl = ACTIVE_TL;
    tl->needs_redraw = true;

}

void user_menu_translate_left(void *nullarg)
{
    Menu *m = window_top_menu(main_win);
    if (!m) {
	fprintf(stderr, "No menu on main window\n");	
	exit(1);
    }
    menu_translate(m, -1 * MENU_MOVE_BY, 0);
    Timeline *tl = ACTIVE_TL;
    tl->needs_redraw = true;
}

void user_menu_translate_right(void *nullarg)
{
    Menu *m = window_top_menu(main_win);
    if (!m) {
	fprintf(stderr, "No menu on main window\n");	
	exit(1);
    }
    menu_translate(m, MENU_MOVE_BY, 0);
    Timeline *tl = ACTIVE_TL;
    tl->needs_redraw = true;
}

void user_menu_dismiss(void *nullarg)
{
    window_pop_menu(main_win);
    Timeline *tl = ACTIVE_TL;
    tl->needs_redraw = true;
    /* window_pop_mode(main_win); */
}

void user_tl_play(void *nullarg)
{
    Timeline *tl = ACTIVE_TL;
    if (proj->dragging && !proj->playing && tl->num_grabbed_clips > 0) {
	timeline_cache_grabbed_clip_positions(tl);
    }
    bool started = false;
    if (proj->play_speed <= 0.0f) {
	/* proj->play_speed = 1.0f; */
	timeline_play_speed_set(1.0);
    } else if (!started) {
	timeline_play_speed_mult(2.0);
	/* /\* proj->play_speed *= 2.0f; *\/ */
	/* status_stat_playspeed(); */
    }

    if (!proj->playing) {
	started = true;
	transport_start_playback();
    }
    /* PageEl *el = panel_area_get_el_by_id(proj->panels, "panel_quickref_play"); */
    /* Button *btn = (Button *)el->component; */
    /* button_press_color_change( */
    /* 	btn, */
    /* 	&color_global_quickref_button_pressed, */
    /* 	&color_global_quickref_button_blue, */
    /* 	quickref_button_press_callback, */
    /* 	NULL); */
}

void user_tl_pause(void *nullarg)
{

    Timeline *tl = ACTIVE_TL;
    proj->play_speed = 0;
    transport_stop_playback();
    PageEl *el = panel_area_get_el_by_id(proj->panels, "panel_quickref_pause");
    Button *btn = (Button *)el->component;
    button_press_color_change(
	btn,
	&color_global_quickref_button_pressed,
	&color_global_quickref_button_blue,
	quickref_button_press_callback,
	NULL);

    tl->needs_redraw = true;
    if (proj->dragging && tl->num_grabbed_clips > 0) {
	timeline_push_grabbed_clip_move_event(tl);
    }
}

void user_tl_rewind(void *nullarg)
{

    Timeline *tl = ACTIVE_TL;
    if (proj->dragging && !proj->playing && tl->num_grabbed_clips > 0) {
	timeline_cache_grabbed_clip_positions(tl);
    }
    if (proj->play_speed >= 0.0f) {
	timeline_play_speed_set(-1.0);
	/* proj->play_speed = -1.0f; */
	transport_start_playback();
    } else {
	timeline_play_speed_mult(2.0);
	/* proj->play_speed *= 2.0f; */
	/* status_stat_playspeed(); */
    }
    PageEl *el = panel_area_get_el_by_id(proj->panels, "panel_quickref_rewind");
    Button *btn = (Button *)el->component;
    button_press_color_change(
	btn,
	&color_global_quickref_button_pressed,
	&color_global_quickref_button_blue,
	quickref_button_press_callback,
	NULL);
}

void user_tl_play_slow(void *nullarg)
{
    Timeline *tl = ACTIVE_TL;
    if (proj->dragging && !proj->playing && tl->num_grabbed_clips > 0) {
	timeline_cache_grabbed_clip_positions(tl);
    }
    timeline_play_speed_set(SLOW_PLAYBACK_SPEED);
    /* proj->play_speed = SLOW_PLAYBACK_SPEED; */
    /* status_stat_playspeed(); */
    transport_start_playback();
}

void user_tl_rewind_slow(void *nullarg)
{
    Timeline *tl = ACTIVE_TL;
    if (proj->dragging && !proj->playing && tl->num_grabbed_clips > 0) {
	timeline_cache_grabbed_clip_positions(tl);
    }
    timeline_play_speed_set(-1 * SLOW_PLAYBACK_SPEED);
    /* proj->play_speed = -1 * SLOW_PLAYBACK_SPEED; */
    /* status_stat_playspeed(); */
    transport_start_playback();
}

static void playhead_incr(float by)
{
    if (!proj->playhead_do_incr) {
	proj->playhead_do_incr = true;
	proj->playhead_frame_incr = by;
    }
}

void user_tl_move_playhead_left(void *nullarg)
{
    playhead_incr(-3.0);
}
    

void user_tl_move_playhead_right(void *nullarg)
{
    playhead_incr(3.0);
}

void user_tl_move_playhead_left_slow(void *nullarg)
{
    playhead_incr(-0.2);
}

void user_tl_move_playhead_right_slow(void *nullarg)
{
    playhead_incr(0.2);
}

void user_tl_nudge_left(void *nullarg)
{
    Timeline *tl = ACTIVE_TL;
    timeline_set_play_position(tl, tl->play_pos_sframes - 500);
}

void user_tl_nudge_right(void *nullarg)
{
    Timeline *tl = ACTIVE_TL;
    timeline_set_play_position(tl, tl->play_pos_sframes + 500);
}

void user_tl_small_nudge_left(void *nullarg)
{
    Timeline *tl = ACTIVE_TL;
    timeline_set_play_position(tl, tl->play_pos_sframes - 100);
}

void user_tl_small_nudge_right(void *nullarg)
{
    Timeline *tl = ACTIVE_TL;
    timeline_set_play_position(tl, tl->play_pos_sframes + 100);
}

void user_tl_one_sample_left(void *nullarg)
{
    Timeline *tl = ACTIVE_TL;
    timeline_set_play_position(tl, tl->play_pos_sframes - 1);
}

void user_tl_one_sample_right(void *nullarg)
{
    Timeline *tl = ACTIVE_TL;
    timeline_set_play_position(tl, tl->play_pos_sframes + 1);
}

void user_tl_move_right(void *nullarg)
{
    timeline_scroll_horiz(ACTIVE_TL, TL_DEFAULT_XSCROLL);
    PageEl *el = panel_area_get_el_by_id(proj->panels, "panel_quickref_right");
    Button *btn = (Button *)el->component;
    button_press_color_change(
	btn,
	&color_global_quickref_button_pressed,
	&color_global_quickref_button_blue,
	quickref_button_press_callback,
	NULL);
}

void user_tl_move_left(void *nullarg)
{
    timeline_scroll_horiz(ACTIVE_TL, TL_DEFAULT_XSCROLL * -1);
    PageEl *el = panel_area_get_el_by_id(proj->panels, "panel_quickref_left");
    Button *btn = (Button *)el->component;
    button_press_color_change(
	btn,
	&color_global_quickref_button_pressed,
	&color_global_quickref_button_blue,
	quickref_button_press_callback,
	NULL);
}

void user_tl_zoom_in(void *nullarg)
{
    timeline_rescale(ACTIVE_TL, 1.2, false);
    PageEl *el = panel_area_get_el_by_id(proj->panels, "panel_quickref_zoom_in");
    Button *btn = (Button *)el->component;
    button_press_color_change(
	btn,
	&color_global_quickref_button_pressed,
	&color_global_quickref_button_blue,
	quickref_button_press_callback,
	NULL);
}

void user_tl_zoom_out(void *nullarg)
{
    timeline_rescale(ACTIVE_TL, 0.8, false);

    PageEl *el = panel_area_get_el_by_id(proj->panels, "panel_quickref_zoom_out");
    Button *btn = (Button *)el->component;
    button_press_color_change(
	btn,
	&color_global_quickref_button_pressed,
	&color_global_quickref_button_blue,
	quickref_button_press_callback,
	NULL);
}

static NEW_EVENT_FN(undo_redo_set_mark, "undo/redo set mark")
    int32_t *mark = (int32_t *)obj1;
    *mark = val1.int32_v;
    Timeline *tl = (Timeline *)obj2;
    tl->needs_redraw = true;
}

/* NEW_EVENT_FN(redo_set_mark) */
/* { */
/*     int32_t *mark = (int32_t *)obj1; */
/*     *mark = val1.int32_v; */
/* } */

void user_tl_set_mark_out(void *nullarg)
{
    Timeline *tl = ACTIVE_TL;
    Value old_mark = {.int32_v = tl->out_mark_sframes};
    transport_set_mark(tl, false);
    Value new_mark = {.int32_v = tl->out_mark_sframes};

    user_event_push(
	&proj->history,
	undo_redo_set_mark,
	undo_redo_set_mark,
	NULL, NULL,
	(void *)&tl->out_mark_sframes,
	(void *)tl,
	old_mark,old_mark,
	new_mark,new_mark,
	0,0,false,false);
}
void user_tl_set_mark_in(void *nullarg)
{
    Timeline *tl = ACTIVE_TL;

    Value old_mark = {.int32_v = tl->in_mark_sframes};
    transport_set_mark(tl, true);
    Value new_mark = {.int32_v = tl->in_mark_sframes};

    user_event_push(
	&proj->history,
	undo_redo_set_mark,
	undo_redo_set_mark,
	NULL, NULL,
	(void *)&tl->in_mark_sframes,
	(void *)tl,
	old_mark, old_mark,
	new_mark, new_mark,
	0, 0, false, false);
}

void user_tl_goto_mark_out(void *nullarg)
{
    Timeline *tl = ACTIVE_TL;
    transport_goto_mark(tl, false);
}

void user_tl_goto_mark_in(void *nullarg)
{
    Timeline *tl = ACTIVE_TL;
    transport_goto_mark(tl, true);
}

void user_tl_goto_zero(void *nullarg)
{
    Timeline *tl = ACTIVE_TL;
    timeline_set_play_position(tl, 0);
    tl->display_offset_sframes = 0;
    timeline_reset(tl, false);
}

void user_tl_toggle_loop_playback(void *nullarg)
{
    proj->loop_play = !proj->loop_play;
}

void user_tl_goto_previous_clip_boundary(void *nullarg)
{
    Timeline *tl = ACTIVE_TL;
    if (timeline_selected_track(tl)) {
	ClipRef *cr = clipref_at_cursor();
	int32_t pos;
	if (cr) {
	    if (cr->pos_sframes == tl->play_pos_sframes) {
		goto goto_previous_clip;
	    }
	    timeline_set_play_position(tl, cr->pos_sframes);
	    timeline_reset(tl, false);
	} else {
	goto_previous_clip:
	    if (clipref_before_cursor(&pos)) {
		timeline_set_play_position(tl, pos);
		timeline_reset(tl, false);
	    } else {
		status_set_errstr("No previous clip on selected track");
	    }	
	}
    } else {
	ClickTrack *tt = timeline_selected_click_track(tl);
	if (tt) {
	    click_track_goto_prox_beat(tt, -1, BP_BEAT);
	}
    }

}


void user_tl_goto_next_clip_boundary(void *nullarg)
{
    Timeline *tl = ACTIVE_TL;
    if (timeline_selected_track(tl)) {
	ClipRef *cr = clipref_at_cursor();
	int32_t pos;
	if (cr) {
	    if (cr->pos_sframes + clipref_len(cr) == tl->play_pos_sframes) {
		goto goto_next_clip;
	    }
	    timeline_set_play_position(tl, cr->pos_sframes + clipref_len(cr));
	    timeline_reset(tl, false);
	} else {
	goto_next_clip:
	    if (clipref_after_cursor(&pos)) {
		timeline_set_play_position(tl, pos);
		timeline_reset(tl, false);
	    } else {
		status_set_errstr("No subsequent clip on selected track");
	    }
	}
    } else {
	ClickTrack *tt = timeline_selected_click_track(tl);
	if (tt) {
	    click_track_goto_prox_beat(tt, 1, BP_BEAT);
	}
    }
}

static void tl_goto_prox_click(Timeline *tl, int direction, enum beat_prominence bp)
{
    ClickTrack *ct = NULL;
    for (int i=tl->num_click_tracks - 1; i>=0; i--) {
	if (tl->click_tracks[i]->layout->index <= tl->layout_selector) {
	    ct = tl->click_tracks[i];
	    break;
	}
    }
    if (ct) {
	click_track_goto_prox_beat(ct, direction, bp);
    }
}

void user_tl_goto_next_beat(void *nullarg)
{
    tl_goto_prox_click(ACTIVE_TL, 1, BP_BEAT);
}

void user_tl_goto_prev_beat(void *nullarg)
{
    tl_goto_prox_click(ACTIVE_TL,-1, BP_BEAT);
}

void user_tl_goto_next_subdiv(void *nullarg)
{
    tl_goto_prox_click(ACTIVE_TL, 1, BP_SUBDIV2);
}

void user_tl_goto_prev_subdiv(void *nullarg)
{
    tl_goto_prox_click(ACTIVE_TL, -1, BP_SUBDIV2);
}

void user_tl_goto_next_measure(void *nullarg)
{
    tl_goto_prox_click(ACTIVE_TL, 1, BP_MEASURE);
}

void user_tl_goto_prev_measure(void *nullarg)
{
    tl_goto_prox_click(ACTIVE_TL, -1, BP_MEASURE);
}

void user_tl_bring_rear_clip_to_front(void *nullarg)
{
    clipref_bring_to_front();
}

void user_tl_set_default_out(void *nullarg) {
    project_set_default_out(proj);
}

static NEW_EVENT_FN(add_track_undo, "undo add track")
    Track *track = (Track *)obj1;
    track_delete(track);
}

static NEW_EVENT_FN(add_track_redo, "redo add track")
    Track *track = (Track *)obj1;
    track_undelete(track);
}

static NEW_EVENT_FN(add_track_dispose_forward, "")
    Track *track = (Track *)obj1;
    track_destroy(track, false);
}

void user_tl_add_track(void *nullarg)
{
    if (!proj) {
	fprintf(stderr, "Error: user call to add track w/o global project\n");
	exit(1);
    }
    Timeline *tl = ACTIVE_TL;
    Track *track = timeline_add_track(tl);
    if (!track) return;
    PageEl *el = panel_area_get_el_by_id(proj->panels, "panel_quickref_add_track");
    Button *btn = (Button *)el->component;
    button_press_color_change(
	btn,
	&color_global_quickref_button_pressed,
	&color_global_quickref_button_blue,
	quickref_button_press_callback,
	NULL);
    tl->needs_redraw = true;

    Value nullval = {.int_v = 0};
    user_event_push(
	&proj->history,
	add_track_undo,
	add_track_redo,
	NULL, add_track_dispose_forward,
	(void *)track,
	NULL,
	nullval, nullval, nullval, nullval,
	0,0,false,false);
	
}

static void track_select_n(int n)
{
    Timeline *tl = ACTIVE_TL;
    if (tl->num_tracks <= n) return;
    Track *track = tl->tracks[n];
    bool *active = &(track->active);
    *active = !(*active);
    tl->needs_redraw = true;
    /* track->input->active = *active; */
    /* fprintf(stdout, "SETTING %s to %d\n", track->input->name, track->input->active); */
}

void user_tl_track_select_1(void *nullarg)
{
    track_select_n(0);

}
void user_tl_track_select_2(void *nullarg)
{
    track_select_n(1);
}

void user_tl_track_select_3(void *nullarg)
{
    track_select_n(2);
}

void user_tl_track_select_4(void *nullarg)
{
    track_select_n(3);
}

void user_tl_track_select_5(void *nullarg)
{
    track_select_n(4);
}

void user_tl_track_select_6(void *nullarg)
{
    track_select_n(5);
}


void user_tl_track_select_7(void *nullarg)
{
    track_select_n(6);
}


void user_tl_track_select_8(void *nullarg)
{
    track_select_n(7);
}

void user_tl_track_select_9(void *nullarg)
{
    track_select_n(8);
}

/* Returns true if and only if all tracks were already active */
static bool activate_all_tracks(Timeline *tl)
{
    bool ret = true;
    Track *track;
    for (uint8_t i=0; i<tl->num_tracks; i++) {
	track = tl->tracks[i];
	if (!track->active) {
	    track->active = true;
	    ret = false;
	}
    }
    tl->needs_redraw = true;
    return ret;
}

static void deactivate_all_tracks(Timeline *tl)
{
    for (uint8_t i=0; i<tl->num_tracks; i++) {
	tl->tracks[i]->active = false;
    }
    tl->needs_redraw = true;
}

void user_tl_track_activate_all(void *nullarg)
{
    Timeline *tl = ACTIVE_TL;
    if (activate_all_tracks(tl)) {
	deactivate_all_tracks(tl);
    }
}

/* static void timeline_rectify_selectors(Timeline *tl) */
/* { */
/*     int click_track_index = -1; */
/*     int track_index = -1; */
/*     bool click_track_selected = false; */
/*     for (int i=0; i<tl->track_area->num_children; i++) { */
/* 	Layout *child = tl->track_area->children[i]; */
/* 	if (strcmp(child->name, "click_track") == 0) { */
/* 	    click_track_index++; */
/* 	    click_track_selected = true; */
/* 	} else { */
/* 	    track_index++; */
/* 	    click_track_selected = false; */
/* 	} */
/* 	if (tl->layout_selector == i) { */
/* 	    if (click_track_selected) { */
/* 		tl->click_track_selector = click_track_index; */
/* 		tl->track_selector = -1; */
/* 	    } else { */
/* 		tl->track_selector = track_index; */
/* 		tl->click_track_selector = -1; */
/* 	    } */
/* 	} */
/*     } */
/*     fprintf(stderr, "SELECTED TRACK: %p; TEMPO TRACK: %p\n", timeline_selected_track(tl), timeline_selected_click_track(tl)); */
/*     fprintf(stderr, "Track selector: %d; tempo track: %d\n", tl->track_selector, tl->click_track_selector); */
/* } */

void user_tl_track_selector_up(void *nullarg)
{
    Timeline *tl = ACTIVE_TL;
    Track *selected = timeline_selected_track(tl);

    if (selected) {
	int old_selection = selected->selected_automation;
	if (old_selection != track_select_prev_automation(selected)) {
	    goto button_animation_and_exit;
	}
	timeline_cache_grabbed_clip_offsets(tl);
    }
    Track *prev_selected = selected;
    if (tl->layout_selector > 0) {
	tl->layout_selector--;
    }
    timeline_rectify_track_indices(tl);
    selected = timeline_selected_track(tl);


    if (selected) {
	if (prev_selected && selected != prev_selected) {
	    /*  Select last shown automation */
	    track_select_last_shown_automation(selected);
	}
	if (proj->dragging && tl->num_grabbed_clips > 0) {
	    timeline_cache_grabbed_clip_positions(tl);
	    bool some_clip_moved = false;
	    for (uint8_t i=0; i<tl->num_grabbed_clips; i++) {
		int offset = tl->grabbed_clip_pos_cache[i].track_offset;
		int new_index = tl->track_selector + offset;
		if (new_index >=0 && new_index < tl->num_tracks) {
		    some_clip_moved = true;
		    ClipRef *cr = tl->grabbed_clips[i];
		    clipref_move_to_track(cr, tl->tracks[new_index]);
		}
		/* clipref_displace(cr, -1 + offset); */
	    }
	    if (some_clip_moved) {
		timeline_push_grabbed_clip_move_event(tl);
	    }
	}
	TabView *tv;
	if ((tv = main_win->active_tabview)) {
	    if (strcmp(tv->title, "Track Effects") == 0) {
		settings_track_tabview_set_track(tv, selected);
	    }
	}
    } else {
	TabView *tv;
	if ((tv = main_win->active_tabview)) {
	    click_track_populate_settings_tabview(timeline_selected_click_track(tl), tv);
	}
    }

button_animation_and_exit:
    
    if (selected) {
	timeline_refocus_track(tl, selected, false);
    } else {
	ClickTrack *ct = timeline_selected_click_track(tl);
	if (ct) {
	    timeline_refocus_click_track(tl, timeline_selected_click_track(tl), false);
	}
    }

    tl->needs_redraw = true;

    PageEl *el = panel_area_get_el_by_id(proj->panels, "panel_quickref_previous");
    Button *btn = (Button *)el->component;
    button_press_color_change(
	btn,
	&color_global_quickref_button_pressed,
	&color_global_quickref_button_blue,
	quickref_button_press_callback,
	NULL);  
}

void user_tl_track_selector_down(void *nullarg)
{
    Timeline *tl = ACTIVE_TL;
    Track *selected = timeline_selected_track(tl);

    if (selected) {
	/* if (selected->selected_automation != selected->num_automations - 1) { */
	    
	int auto_sel = track_select_next_automation(selected);
	if (auto_sel >= 0) {
	    goto button_animation_and_exit;
	}
	/* } else if { */
	/*     selected->selected_automation = -1; */
	/* } */
	timeline_cache_grabbed_clip_offsets(tl);
    }
    
    Track *prev_selected = selected;
    if (tl->layout_selector < tl->track_area->num_children - 1) {
	tl->layout_selector++;
    }
    timeline_rectify_track_indices(tl);
    selected = timeline_selected_track(tl);


    if (selected) {
	if (prev_selected && selected != prev_selected) {
	    selected->selected_automation = -1;
	}
	if (proj->dragging && tl->num_grabbed_clips > 0) {
	    timeline_cache_grabbed_clip_positions(tl);
	    for (uint8_t i=0; i<tl->num_grabbed_clips; i++) {
		int offset = tl->grabbed_clip_pos_cache[i].track_offset;
		int new_index = tl->track_selector + offset;
		if (new_index >=0 && new_index < tl->num_tracks) {
		    ClipRef *cr = tl->grabbed_clips[i];
		    clipref_move_to_track(cr, tl->tracks[new_index]);
		}
	    }
	    timeline_push_grabbed_clip_move_event(tl);
	}
	TabView *tv;
	if ((tv = main_win->active_tabview)) {
	    if (strcmp(tv->title, "Track Effects") == 0) {
		settings_track_tabview_set_track(tv, selected);
	    }
	}
    } else {
	TabView *tv;
	if ((tv = main_win->active_tabview)) {
	    click_track_populate_settings_tabview(timeline_selected_click_track(tl), tv);
	}
    }



button_animation_and_exit:

    if (selected) {
	timeline_refocus_track(tl, selected, true);
    } else {
	ClickTrack *ct = timeline_selected_click_track(tl);
	if (ct) {
	    timeline_refocus_click_track(tl, timeline_selected_click_track(tl), true);
	}
    }

    /* if (selected) timeline_refocus_track(tl, selected, true); */
    tl->needs_redraw = true;

    PageEl *el = panel_area_get_el_by_id(proj->panels, "panel_quickref_next");
    Button *btn = (Button *)el->component;
    button_press_color_change(
	btn,
	&color_global_quickref_button_pressed,
	&color_global_quickref_button_blue,
	quickref_button_press_callback,
	NULL);  
}

/* Moves automation track if applicable */
/* static void move_track(int direction) */
/* { */
/*     /\* Timeline *tl = ACTIVE_TL; *\/ */
/*     /\* Track *track = timeline_selected_track(tl); *\/ */
/*     /\* if (!track) { *\/ */
/*     /\* 	status_set_errstr(NO_TRACK_ERRSTR); *\/ */
/*     /\* 	return; *\/ */
/*     /\* } *\/ */
/*     /\* /\\* if (track->num_automations == 0 || track->selected_automation == -1) { *\\/ *\/ */
/*     /\* if (TRACK_AUTO_SELECTED(track)) { *\/ */
/*     /\* 	track_move_automation(track->automations[track->selected_automation], direction, false); *\/ */
/*     /\* } else { *\/ */
/*     /\* 	timeline_move_track(tl, track, direction, false); *\/ */
/*     /\* } *\/ */
/*     /\* tl->needs_redraw = true; *\/ */
/*     Timeline *tl = ACTIVE_TL; */
/*     if (tl->layout_selector + direction >= tl->track_area->num_children) return; */
/*     if (tl->layout_selector + direction < 0) return; */
    
/*     Layout *sel = tl->track_area->children[tl->layout_selector]; */
/*     Layout *swap = tl->track_area->children[tl->layout_selector + direction]; */
/*     layout_swap_children(sel, swap); */
/*     layout_force_reset(tl->track_area); */
/*     /\* tl->needs_redraw = true; *\/ */
/*     tl->layout_selector += direction; */
/*     timeline_rectify_track_indices(tl); */
/*     Track *sel_track = timeline_selected_track(tl); */
/*     if (sel_track) { */
/* 	timeline_refocus_track(tl, sel_track, direction > 0); */
/*     } else { */
/* 	ClickTrack *sel_tt = timeline_selected_click_track(tl); */
/* 	timeline_refocus_click_track(tl, sel_tt, direction > 0); */
/*     } */
    
/* } */

void user_tl_move_track_up(void *nullarg)
{
    timeline_move_track_or_automation(ACTIVE_TL, -1);
}

void user_tl_move_track_down(void *nullarg)
{
    timeline_move_track_or_automation(ACTIVE_TL, 1);
}

void user_tl_tracks_minimize(void *nullarg)
{
    timeline_minimize_track_or_tracks(ACTIVE_TL);
}


void user_tl_track_activate_selected(void *nullarg)
{
    Timeline *tl = ACTIVE_TL;
    if (tl->track_selector >= 0) {
	track_select_n(tl->track_selector);
    }
}

void project_draw(void *nullarg);
void user_tl_track_rename(void *nullarg)
{
    /* window_pop_menu(main_win); */
    /* window_pop_mode(main_win); */
    Timeline *tl = ACTIVE_TL;
    Track *track = timeline_selected_track(tl);
    if (track) {
	track_rename(track);
    }
    tl->needs_redraw = true;
    /* fprintf(stdout, "DONE track edit\n"); */
}

void user_tl_rename_clip_at_cursor(void *nullarg)
{
    ClipRef *cr = clipref_at_cursor();
    if (cr) {
	clipref_rename(cr);
	cr->track->tl->needs_redraw = true;
    }
}


void user_tl_track_set_in(void *nullarg)
{
    Timeline *tl = ACTIVE_TL;
    Track *track = timeline_selected_track(tl);
    if (!track) {
	return;
    }
    track_set_input(track);
}

static NEW_EVENT_FN(undo_track_delete, "undo delete track")
    Track *track = (Track *)obj1;
    track_undelete(track);
}

static NEW_EVENT_FN(redo_track_delete, "redo delete track")
    Track *track = (Track *)obj1;
    track_delete(track);
}

static NEW_EVENT_FN(dispose_track_delete, "")
    Track *track = (Track *)obj1;
    track_destroy(track, false);
}

void user_tl_track_delete(void *nullarg)
{
    TABVIEW_BLOCK(delete track);
    if (proj->recording) transport_stop_recording();
    Timeline *tl = ACTIVE_TL;
    Track *track = timeline_selected_track(tl);
    if (track) {
	if (TRACK_AUTO_SELECTED(track)) {
	    Automation *a = track->automations[track->selected_automation];
	    automation_delete(a);
	    return;
	}
	track_delete(track);
	/* timeline_reset(tl); */
	if (tl->track_selector > 0 && tl->track_selector > tl->num_tracks - 1) {
	    tl->track_selector--;
	}
	Value nullval = {.int_v = 0};
	user_event_push(
	    &proj->history,
	    undo_track_delete,
	    redo_track_delete,
	    dispose_track_delete, NULL,
	    (void *)track,
	    NULL,
	    nullval,nullval,nullval,nullval,
	    0,0,false,false);	

    } else {
	if (!timeline_click_track_delete(tl)) {   
	    status_set_errstr("Error: no track to delete");
	}
    }
}

void user_tl_mute(void *nullarg)
{
    Timeline *tl = ACTIVE_TL;
    track_or_tracks_mute(tl);
}
void user_tl_solo(void *nullarg)
{
    Timeline *tl = ACTIVE_TL;
    track_or_tracks_solo(tl, NULL);
    /* tl->needs_redraw = true; */
}

static Value vol_incr = {.float_v = 0.04};
static Value pan_incr = {.float_v = 0.02};
void user_tl_track_vol_up(void *nullarg)
{

    /* if (proj->vol_changing) return; */
    /* proj->vol_changing = true; */
    /* proj->vol_up = false; */

    bool has_active_track = false;
    Timeline *tl = ACTIVE_TL;
    for (int i=0; i<tl->num_tracks; i++) {
	Track *trk = tl->tracks[i];
	if (trk->active) {
	    has_active_track = true;
	    endpoint_start_continuous_change(&trk->vol_ep, true, vol_incr, JDAW_THREAD_MAIN, endpoint_safe_read(&trk->vol_ep, NULL));
	}
    }
    if (!has_active_track) {
	Track *trk = timeline_selected_track(tl);
	if (trk) {
	    endpoint_start_continuous_change(&trk->vol_ep, true, vol_incr, JDAW_THREAD_MAIN, endpoint_safe_read(&trk->vol_ep, NULL));
	} else {
	    ClickTrack *tt = timeline_selected_click_track(tl);
	    if (tt) {
		endpoint_start_continuous_change(&tt->metronome_vol_ep, true, vol_incr, JDAW_THREAD_MAIN, endpoint_safe_read(&tt->metronome_vol_ep, NULL));
	    }
	}

    }

    /* Timeline *tl = ACTIVE_TL; */
    /* if (tl->num_tracks == 0) return; */
    /* if (proj->vol_changing) return; */
    /* /\* proj->vol_changing = timeline_selected_track(tl); *\/ */
    /* proj->vol_changing = true; */
    /* proj->vol_up = true; */
    /* /\* Track *track = timeline_selected_track(tl); *\/ */
    /* /\* endpoint_start_continuous_change(&track->vol_ep); *\/ */
}

void user_tl_track_vol_down(void *nullarg)
{
    /* if (proj->vol_changing) return; */
    /* proj->vol_changing = true; */
    /* proj->vol_up = false; */

    bool has_active_track = false;
    Timeline *tl = ACTIVE_TL;
    Value vol_decr = jdaw_val_negate(vol_incr, JDAW_FLOAT);
    for (int i=0; i<tl->num_tracks; i++) {
	Track *trk = tl->tracks[i];
	if (trk->active) {
	    has_active_track = true;
	    endpoint_start_continuous_change(&trk->vol_ep, true, vol_decr, JDAW_THREAD_MAIN, endpoint_safe_read(&trk->vol_ep, NULL));
	}
    }
    if (!has_active_track) {
	Track *trk = timeline_selected_track(tl);
	if (trk) {
	    endpoint_start_continuous_change(&trk->vol_ep, true, vol_decr, JDAW_THREAD_MAIN, endpoint_safe_read(&trk->vol_ep, NULL));
	} else {
	    ClickTrack *tt = timeline_selected_click_track(tl);
	    if (tt) {
		endpoint_start_continuous_change(&tt->metronome_vol_ep, true, vol_decr, JDAW_THREAD_MAIN, endpoint_safe_read(&tt->metronome_vol_ep, NULL));
	    }
	}
    }
}

void user_tl_track_pan_left(void *nullarg)
{
    /* Timeline *tl = ACTIVE_TL; */
    /* /\* if (tl->num_tracks ==0) return; *\/ */
    /* if (proj->pan_changing) return; */
    /* proj->pan_changing = timeline_selected_track(tl); */
    /* proj->pan_right = false; */
    /* tl->needs_redraw = true; */


    bool has_active_track = false;
    Timeline *tl = ACTIVE_TL;
    Value pan_decr = jdaw_val_negate(pan_incr, JDAW_FLOAT);
    for (int i=0; i<tl->num_tracks; i++) {
	Track *trk = tl->tracks[i];
	if (trk->active) {
	    has_active_track = true;
	    endpoint_start_continuous_change(&trk->pan_ep, true, pan_decr, JDAW_THREAD_MAIN, endpoint_safe_read(&trk->pan_ep, NULL));
	}
    }
    if (!has_active_track) {
	Track *trk = timeline_selected_track(tl);
	if (trk)
	    endpoint_start_continuous_change(&trk->pan_ep, true, pan_decr, JDAW_THREAD_MAIN, endpoint_safe_read(&trk->pan_ep, NULL));
    }
}

void user_tl_track_pan_right(void *nullarg)
{
    bool has_active_track = false;
    Timeline *tl = ACTIVE_TL;
    for (int i=0; i<tl->num_tracks; i++) {
	Track *trk = tl->tracks[i];
	if (trk->active) {
	    has_active_track = true;
	    endpoint_start_continuous_change(&trk->pan_ep, true, pan_incr, JDAW_THREAD_MAIN, endpoint_safe_read(&trk->pan_ep, NULL));
	}
    }
    if (!has_active_track) {
	Track *trk = timeline_selected_track(tl);
	if (trk)
	    endpoint_start_continuous_change(&trk->pan_ep, true, pan_incr, JDAW_THREAD_MAIN, endpoint_safe_read(&trk->pan_ep, NULL));
    }

    /* Timeline *tl = ACTIVE_TL; */
    /* if (tl->num_tracks == 0) return; */
    /* if (proj->pan_changing) return; */
    /* proj->pan_changing = timeline_selected_track(tl); */
    /* proj->pan_right = true; */
    /* tl->needs_redraw = true; */
}

void user_tl_track_add_effect(void *nullarg)
{
    TABVIEW_BLOCK(add automation);
    Timeline *tl = ACTIVE_TL;
    Track *track = timeline_selected_track(tl);
    if (track) {
	track_add_new_effect(track);
	/* track_add_new_automation(track); */
	/* track_automations_show_all(track); */
    } else {
	status_set_errstr(NO_TRACK_ERRSTR);
    }
}


void user_tl_track_open_settings(void *nullarg)
{
    Timeline *tl = ACTIVE_TL;
    if (main_win->active_tabview) {
	tabview_close(main_win->active_tabview);
	tl->needs_redraw = true;
	return;
    }
    Track *track = timeline_selected_track(tl);

    if (track) {
	if (track->num_effects == 0) {
	    user_tl_track_add_effect(NULL);
	    return;
	}

	TabView *tv = settings_track_tabview_create(track);
	tabview_activate(tv);
	tl->needs_redraw = true;
    } else {
	timeline_click_track_edit(tl);
    }
}



void user_tl_track_add_automation(void *nullarg)
{
    TABVIEW_BLOCK(add automation);
    Timeline *tl = ACTIVE_TL;
    Track *track = timeline_selected_track(tl);
    if (track) {
	track_add_new_automation(track);
	track_automations_show_all(track);
    } else {
	status_set_errstr(NO_TRACK_ERRSTR);
    }
}

void user_tl_track_show_hide_automations(void *nullarg)
{
    Timeline *tl = ACTIVE_TL;
    Track *track = timeline_selected_track(tl);
    if (!track) {
	status_set_errstr(NO_TRACK_ERRSTR);
	return;
    }
    if (track->num_automations == 0) {
	status_set_errstr("No automation tracks to show. C-a to add");
	return;
    }
    bool some_shown = false;
    for (uint8_t i=0; i<track->num_automations; i++) {
	if (track->automations[i]->shown) {
	    some_shown = true;
	    break;
	}
    }
    if (some_shown) {
	track_automations_hide_all(track);
    } else {
	track_automations_show_all(track);
    }
    tl->needs_redraw = true;
}

void user_tl_track_automation_toggle_read(void *nullarg)
{
    Timeline *tl = ACTIVE_TL;
    Track *track = timeline_selected_track(tl);
    if (!track) {
	status_set_errstr(NO_TRACK_ERRSTR);
	return;
    }
    if (TRACK_AUTO_SELECTED(track)) {
	Automation *a = track->automations[track->selected_automation];
	automation_toggle_read(a);
    }
    tl->needs_redraw = true;

}

void user_tl_record(void *nullarg)
{
    if (proj->recording) {
	transport_stop_recording();
	return;
    }

    if (proj->automation_recording) {
	automation_record(proj->automation_recording);
	proj->automation_recording = NULL;
	return;
    }
    Timeline *tl = ACTIVE_TL;
    Track *sel_track = timeline_selected_track(tl);
    if (!sel_track) {
	status_set_errstr("Select an audio track to record");
	return;
    }
    if (sel_track && sel_track->selected_automation >= 0) {
	Automation *sel_auto = sel_track->automations[sel_track->selected_automation];
	automation_record(sel_auto);
	proj->automation_recording = sel_auto;
	tl->needs_redraw = true;
	return;
    }
    transport_start_recording();

    /* tl->needs_redraw = true; */
}

void user_tl_click_track_add(void *nullarg)
{
    timeline_add_click_track(ACTIVE_TL);
}

/* void user_tl_click_track_cut(void *nullarg) */
/* { */
/*     timeline_cut_click_track_at_cursor(ACTIVE_TL); */
/* } */

void user_tl_click_track_set_tempo(void *nullarg)
{
    timeline_click_track_set_tempo_at_cursor(ACTIVE_TL);
}

void user_tl_clipref_grab_ungrab(void *nullarg)
{
    Timeline *tl = ACTIVE_TL;
    tl->needs_redraw = true;
    if (tl->num_tracks == 0) return;
    Track *track = NULL;
    ClipRef *cr =  NULL;
    
    ClipRef *clips_to_grab[MAX_GRABBED_CLIPS];
    uint8_t num_clips = 0;
    
    bool clip_grabbed = false;
    bool had_active_track = false;
    for (uint8_t i=0; i<tl->num_tracks; i++) {
	track = tl->tracks[i];
	if (track->active) {
	    had_active_track = true;
	    cr = clipref_at_cursor_in_track(track);
	    if (cr && !cr->grabbed) {
		clips_to_grab[num_clips] = cr;
		num_clips++;
		/* clipref_grab(cr); */
		clip_grabbed = true;
		
		/* undo_crs[num_undo_crs] = cr; */
		/* num_undo_crs++; */
	    }
	}
    }
    track = timeline_selected_track(tl);
    if (!had_active_track && track) {
	track = timeline_selected_track(tl);
	cr = clipref_at_cursor_in_track(track);
	if (cr && !cr->grabbed) {
	    /* clipref_grab(cr); */
	    clips_to_grab[num_clips] = cr;
	    num_clips++;
	    clip_grabbed = true;

	    /* undo_crs[num_undo_crs] = cr; */
	    /* num_undo_crs++; */
	}
    }

    if (clip_grabbed) {
	for (uint8_t i=0; i<num_clips; i++) {
	    clipref_grab(clips_to_grab[i]);
	}
	if (proj->dragging && proj->playing) {
	    timeline_cache_grabbed_clip_positions(tl);
	}
    } else {
	if (proj->dragging && proj->playing) {
	    timeline_push_grabbed_clip_move_event(tl);
	}
	for (uint8_t i=0; i<tl->num_grabbed_clips; i++) {
	    cr = tl->grabbed_clips[i];
	    cr->grabbed = false;
	}
	tl->num_grabbed_clips = 0;
    }
    
    if (proj->dragging) {
	status_stat_drag();
    }
    
    /* timeline_cache_grabbed_clip_positions(tl); */
    /* if (num_undo_crs > 0) { */
    /* 	ClipRef **clips = calloc(num_undo_crs, sizeof(ClipRef *)); */
    /* 	memcpy(clips, undo_crs, num_undo_crs * sizeof(ClipRef *)); */
    /* 	Value num = {.uint8_v = num_undo_crs}; */
    /* 	user_event_push( */
    /* 	    &proj->history, */
    /* 	    undo_redo_grab_clips, */
    /* 	    undo_redo_grab_clips, */
    /* 	    NULL, */
    /* 	    clips, */
    /* 	    NULL, */
    /* 	    num, */
    /* 	    num, */
    /* 	    num, */
    /* 	    num, */
    /* 	    0, */
    /* 	    0, */
    /* 	    true, */
    /* 	    false); */
    /* } */
    
    /* if (tl->num_grabbed_clips > 0) { */

    /* 	if (proj->playing && proj->dragging) */
    /* 	    timeline_push_grabbed_clip_move_event(tl); */
    /* } */
    /* tl->needs_redraw = true; */
}

void user_tl_grab_marked_range(void *nullarg)
{
    Timeline *tl = ACTIVE_TL;
    bool had_active_track = false;
    for (uint8_t i=0; i<tl->num_tracks; i++) {
	Track *t = tl->tracks[i];
	if (t->active) {
	    had_active_track = true;
	    for (int i=0; i<t->num_clips; i++) {
		ClipRef *cr = t->clips[i];
		if (clipref_marked(tl, cr) && !cr->grabbed) {
		    clipref_grab(cr);
		}
	    }
	}
    }
    Track *t = timeline_selected_track(tl);
    if (!had_active_track && t) {
	/* Track *t = timeline_selected_track(tl); */
	for (int i=0; i<t->num_clips; i++) {
	    ClipRef *cr = t->clips[i];
	    if (clipref_marked(tl, cr) && !cr->grabbed) {
		clipref_grab(cr);
	    }
	}
    }
    tl->needs_redraw = true;
}

void user_tl_copy_grabbed_clips(void *nullarg)
{
    Timeline *tl = ACTIVE_TL;
    memcpy(tl->clipboard, tl->grabbed_clips, sizeof(ClipRef *) * tl->num_grabbed_clips);
    tl->num_clips_in_clipboard = tl->num_grabbed_clips;
}

NEW_EVENT_FN(undo_paste_grabbed_clips, "undo paste grabbed clips")
    ClipRef **clips = (ClipRef **)obj1;
    uint8_t num = val1.uint8_v;
     for (int i=0; i<num; i++) {
	 clipref_delete(clips[i]);
     }
}

NEW_EVENT_FN(redo_paste_grabbed_clips, "redo paste grabbed clips")
    ClipRef **clips = (ClipRef **)obj1;
    uint8_t num = val1.uint8_v;
     for (int i=0; i<num; i++) {
	 clipref_undelete(clips[i]);
     }
}

NEW_EVENT_FN(dispose_forward_paste_grabbed_clips, "redo paste grabbed clips")
    ClipRef **clips = (ClipRef **)obj1;
    uint8_t num = val1.uint8_v;
     for (int i=0; i<num; i++) {
	 clipref_destroy_no_displace(clips[i]);
     }
}

void user_tl_paste_grabbed_clips(void *nullarg)
{
    Timeline *tl = ACTIVE_TL;
    timeline_ungrab_all_cliprefs(tl);
    if (tl->num_clips_in_clipboard == 0) {
	status_set_errstr("No clips copied to clipboard");
	return;
    }
    int32_t leftmost = tl->clipboard[0]->pos_sframes;
    for (int i=0; i<tl->num_clips_in_clipboard; i++) {
	ClipRef *cr = tl->clipboard[i];
	if (cr->pos_sframes < leftmost) leftmost = cr->pos_sframes;
    }

    
    ClipRef **undo_cache = calloc(tl->num_clips_in_clipboard, sizeof(ClipRef *));
    uint8_t actual_num = 0;
    for (int i=0; i<tl->num_clips_in_clipboard; i++) {
	ClipRef *cr = tl->clipboard[i];
	if (!cr->deleted && !cr->track->deleted) {
	    int32_t offset = cr->pos_sframes - leftmost;
	    ClipRef *copy = track_create_clip_ref(cr->track, cr->clip, tl->play_pos_sframes + offset, false);
	    if (!copy) continue;
	    snprintf(copy->name, MAX_NAMELENGTH, "%s copy", cr->name);
	    copy->in_mark_sframes = cr->in_mark_sframes;
	    copy->out_mark_sframes = cr->out_mark_sframes;
	    copy->start_ramp_len = cr->start_ramp_len;
	    copy->end_ramp_len = cr->end_ramp_len;
	    clipref_grab(copy);
	    undo_cache[actual_num] = copy;
	    actual_num++;
	}	    
    }

    Value num = {.uint8_v = actual_num};
    user_event_push(
	&proj->history,
	undo_paste_grabbed_clips,
	redo_paste_grabbed_clips,
	NULL, dispose_forward_paste_grabbed_clips,
	undo_cache, NULL,
	num, num, num, num,
	0, 0, true, false);
    timeline_reset(tl, false);
}

void user_tl_toggle_drag(void *nullarg)
{
    proj->dragging = !proj->dragging;
    status_stat_drag();
    Timeline *tl = ACTIVE_TL;
    if (proj->playing) {
	if (proj->dragging) {
	    timeline_cache_grabbed_clip_positions(tl);
	} else {
	    timeline_push_grabbed_clip_move_event(tl);
	}
    }
}

void user_tl_cut_clipref(void *nullarg)
{
    Timeline *tl = ACTIVE_TL;
    timeline_cut_at_cursor(tl);
    tl->needs_redraw = true;
}

void user_tl_split_stereo_clipref(void *nullarg)
{
    ClipRef *cr = clipref_at_cursor();
    if (cr) {
	clipref_split_stereo_to_mono(cr, NULL, NULL);
    }
}

void user_tl_load_clip_at_cursor_to_src(void *nullarg)
{
    /* Timeline *tl = ACTIVE_TL; */
    ClipRef *cr = clipref_at_cursor();
    if (cr && !cr->clip->recording) {
	proj->src_clip = cr->clip;
	proj->src_in_sframes = cr->in_mark_sframes;
	proj->src_play_pos_sframes = 0;
	proj->src_out_sframes = cr->out_mark_sframes;
	/* fprintf(stdout, "Src clip name? %s\n", proj->src_clip->name); */
	/* txt_set_value_handle(proj->source_name_tb->text, proj->src_clip->name); */
	Timeline *tl = ACTIVE_TL;
	tl->needs_redraw = true;
	PageEl *el = panel_area_get_el_by_id(proj->panels, "panel_source_clip_name_tb");
	Textbox *tb = (Textbox *)el->component;
	textbox_set_value_handle(tb, cr->clip->name);
	panel_page_refocus(proj->panels, "Sample source", 1);
    }

}

void user_tl_activate_source_mode(void *nullarg)
{
    if (!proj->source_mode) {
	if (proj->src_clip) {
	    proj->source_mode = true;
	    window_push_mode(main_win, SOURCE);
	    panel_page_refocus(proj->panels, "Sample source", 1);
	} else {
	    status_set_errstr("Load a clip to source before activating source mode");
	}
    } else {
	proj->source_mode = false;
	window_pop_mode(main_win);
    }
    Timeline *tl = ACTIVE_TL;
    tl->needs_redraw = true;

}

/* static int32_t get_drop_pos() */
/* { */
/*     double latency_est_ms = 44.0f; */
/*     struct timespec now; */
/*     clock_gettime(CLOCK_MONOTONIC, &now); */
/*     struct realtime_tick pb = proj->playback_conn->callback_time; */
/*     double elapsed_pb_chunk_ms = TIMESPEC_DIFF_MS(now, pb.ts); */
/*     int32_t tl_pos_now = pb.timeline_pos + (int32_t)((elapsed_pb_chunk_ms - latency_est_ms) * proj->sample_rate * proj->play_speed / 1000.0f); */
/*     return tl_pos_now; */
/* } */

static NEW_EVENT_FN(undo_drop, "Undo drop clip from source")
    ClipRef *cr = (ClipRef *)obj1;
    clipref_delete(cr);
}

static NEW_EVENT_FN(redo_drop, "Redo drop clip from source")
    ClipRef *cr = (ClipRef *)obj1;
    clipref_undelete(cr);
}

static NEW_EVENT_FN(dispose_forward_drop, "")
    ClipRef *cr = (ClipRef *)obj1;
    clipref_destroy_no_displace(cr);
}

void user_tl_drop_from_source(void *nullarg)
{
    Timeline *tl = ACTIVE_TL;
    if (tl->num_tracks == 0) return;
    Track *track = timeline_selected_track(tl);
    if (!track) {
	status_set_errstr(NO_TRACK_ERRSTR);
	return;
    }
    if (proj->src_clip) {
	/* int32_t drop_pos = tl->play_pos_sframes - proj->play_speed * 2 * proj->chunk_size_sframes; */
	if (proj->src_in_sframes >= proj->src_out_sframes) return;
	int32_t drop_pos = tl->play_pos_sframes;
	/* int32_t drop_pos = get_drop_pos(); */
	ClipRef *cr = track_create_clip_ref(track, proj->src_clip, drop_pos, false);
	if (!cr) return;
	cr->in_mark_sframes = proj->src_in_sframes;
	cr->out_mark_sframes = proj->src_out_sframes;
	clipref_reset(cr, true);
	struct drop_save current_drop = (struct drop_save){cr->clip, cr->in_mark_sframes, cr->out_mark_sframes};
	/* struct drop_save drop_zero =  proj->saved_drops[0]; */
	/* fprintf(stdout, "Current: %p, %d, %d\nzero: %p, %d, %d\n", current_drop.clip, current_drop.in, current_drop.out, drop_zero.clip, drop_zero.in, drop_zero.out); */
	if (proj->num_dropped == 0 || memcmp(&current_drop, &(proj->saved_drops[0]), sizeof(struct drop_save)) != 0) {
	    for (int i=4; i>0; i--) {
		proj->saved_drops[i] = proj->saved_drops[i-1];
	    }
	    /* memcpy(proj->saved_drops + 1, proj->saved_drops, 3 * sizeof(struct drop_save)); */
	    proj->saved_drops[0] = (struct drop_save){cr->clip, cr->in_mark_sframes, cr->out_mark_sframes};
	    if (proj->num_dropped <= 4) proj->num_dropped++;
	    /* fprintf(stdout, "MET condition, num dropped: %d\n", proj->num_dropped); */
	}
	tl->needs_redraw = true;
	Value nullval = {.int_v = 0};
	user_event_push(
	    &proj->history,
	    undo_drop, redo_drop,
	    NULL, dispose_forward_drop,
	    (void *)cr, NULL,
	    nullval, nullval, nullval, nullval,
	    0, 0, false, false);
	    
    }
}

static void user_tl_drop_savedn_from_source(int n)
{
    if (n < proj->num_dropped) {
	/* fprintf(stdout, "N: %d, num dropped: %d\n", n, proj->num_dropped); */
	Timeline *tl = ACTIVE_TL;
	if (tl->num_tracks == 0) return;
	Track *track = timeline_selected_track(tl);
	if (!track) return;
	struct drop_save drop = proj->saved_drops[n];
	if (!drop.clip) return;
	/* int32_t drop_pos = tl->play_pos_sframes - proj->play_speed * 2 * proj->chunk_size_sframes; */
	int32_t drop_pos = tl->play_pos_sframes;
	/* int32_t drop_pos = get_drop_pos(); */
	ClipRef *cr = track_create_clip_ref(track, drop.clip, drop_pos, false);
	if (!cr) return;
	cr->in_mark_sframes = drop.in;
	cr->out_mark_sframes = drop.out;
	clipref_reset(cr, true);

	tl->needs_redraw = true;
	Value nullval = {.int_v = 0};
	user_event_push(
	    &proj->history,
	    undo_drop, redo_drop,
	    NULL, dispose_forward_drop,
	    (void *)cr, NULL,
	    nullval, nullval, nullval, nullval,
	    0, 0, false, false);
	/* ClipRef *cr = track_create_clip_ref(track, drop.cr->clip,  */
    }
}

void user_tl_drop_saved1_from_source(void *nullarg)
{
    user_tl_drop_savedn_from_source(1);
}
void user_tl_drop_saved2_from_source(void *nullarg)
{
    user_tl_drop_savedn_from_source(2);
}
void user_tl_drop_saved3_from_source(void *nullarg)
{
    user_tl_drop_savedn_from_source(3);
}
void user_tl_drop_saved4_from_source(void *nullarg)
{
    user_tl_drop_savedn_from_source(4);
}

static int new_tl_submit_form(void *mod_v, void *target)
{
    Modal *mod = (Modal *)mod_v;
    for (uint8_t i=0; i<mod->num_els; i++) {
	ModalEl *el = mod->els[i];
	if (el->type == MODAL_EL_TEXTENTRY) {
	    project_reset_tl_label(proj);
	    break;
	}
    }
    window_pop_modal(main_win);
    Timeline *tl = ACTIVE_TL;
    tl->needs_redraw = true;
    return 0;
}

void user_tl_add_new_timeline(void *nullarg)
{
    if (proj->recording) transport_stop_recording(); else  transport_stop_playback();
    
    ACTIVE_TL->layout->hidden = true;
    proj->active_tl_index = project_add_timeline(proj, "New Timeline");
    project_reset_tl_label(proj);

    Layout *mod_lt = layout_add_child(main_win->layout);
    layout_set_default_dims(mod_lt);
    Modal *mod = modal_create(mod_lt);
    modal_add_header(mod, "Create new timeline:", &color_global_light_grey, 5);
    modal_add_textentry(
	mod,
	ACTIVE_TL->name,
	MAX_NAMELENGTH,
	txt_name_validation,
	NULL, NULL);
    modal_add_button(mod, "Create", new_tl_submit_form);
    mod->submit_form = new_tl_submit_form;
    window_push_modal(main_win, mod);
    modal_reset(mod);
    modal_move_onto(mod);
    ACTIVE_TL->needs_redraw = true;
}

void user_tl_previous_timeline(void *nullarg)
{
    if (proj->active_tl_index > 0) {
	if (proj->recording) transport_stop_recording(); else  transport_stop_playback();
	timeline_switch(proj->active_tl_index - 1);
    } else {
	status_set_errstr("No timeline to the left.");
    }
}

void user_tl_next_timeline(void *nullarg)
{
    if (proj->active_tl_index < proj->num_timelines - 1) {
	if (proj->recording) transport_stop_recording(); else  transport_stop_playback();
	timeline_switch(proj->active_tl_index + 1);
	/* ACTIVE_TL->layout->hidden = true; */
	/* proj->active_tl_index++; */
	/* ACTIVE_TL->layout->hidden = false; */
	/* project_reset_tl_label(proj); */
	/* timeline_reset_full(ACTIVE_TL); */
	/* ACTIVE_TL->needs_redraw = true; */
    } else {
	status_set_errstr("No timeline to the right. Create new with A-t");
    }
}

void user_tl_delete_timeline(void *nullarg)
{
    Timeline *tl = ACTIVE_TL;
    timeline_delete(tl, false);
}


/* void _____user_global_save_project(void *) */
/* { */
/*     fprintf(stdout, "user_global_save\n"); */
/*     Layout *mod_lt = layout_add_child(main_win->layout); */
/*     layout_set_default_dims(mod_lt); */
/*     Modal *save_as = modal_create(mod_lt); */
/*     modal_add_header(save_as, "Save as:", &color_global_light_grey, 3); */
/*     modal_add_header(save_as, "Project name:", &color_global_light_grey, 5); */
/*     modal_add_textentry(save_as, proj->name); */
/*     modal_add_p(save_as, "\t\t|\t\t<tab>\tv\t\t|\t\t\tS-p\t^\t\t|\t\tC-<ret>\tSubmit (save as)\t\t|", &color_global_black); */
/*     /\* modal_add_op(save_as, "\t\t(type <ret> to accept name)", &color_global_light_grey); *\/ */
/*     modal_add_header(save_as, "Project location:", &color_global_light_grey, 5); */
/*     modal_add_dirnav(save_as, DIRPATH_SAVED_PROJ, dir_to_tline_filter_save); */
/*     save_as->submit_form = submit_save_as_form; */
/*     window_push_modal(main_win, save_as); */
/*     modal_reset(save_as); */
/*     /\* fprintf(stdout, "about to call move onto\n"); *\/ */
/*     modal_move_onto(save_as); */
/* } */

static int submit_save_wav_form(void *mod_v, void *target)
{
    Modal *modal = (Modal *)mod_v;
    char *name;
    char *dirpath;
    ModalEl *el;
    for (uint8_t i=0; i<modal->num_els; i++) {
	switch ((el = modal->els[i])->type) {
	case MODAL_EL_TEXTENTRY:
	    name = ((TextEntry *)el->obj)->tb->text->value_handle;
	    break;
	case MODAL_EL_DIRNAV: {
	    DirNav *dn = (DirNav *)el->obj;
	    /* DirPath *dir = (DirPath *)dn->lines->items[dn->current_line]->obj; */
	    dirpath = dn->current_path_tb->text->value_handle;
	    break;
	}
	default:
	    break;
	}
    }
    char buf[255];
    memset(buf, '\0', sizeof(buf));
    strcat(buf, dirpath);
    strcat(buf, "/");
    strcat(buf, name);
    fprintf(stdout, "SAVE WAV: %s\n", buf);
    wav_write_mixdown(buf);
    /* jdaw_write_project(buf); */
    char *last_slash_pos = strrchr(buf, '/');
    if (last_slash_pos) {
	*last_slash_pos = '\0';
	/* fprintf(stdout, "Real path of %s:\n", dirpath); */
	char *realpath_ret;
	if (!(realpath_ret = realpath(dirpath, NULL))) {
	    perror("Error in realpath");
	} else {
	    /* if (DIRPATH_EXPORT != realpath_ret) { */
	    strncpy(DIRPATH_EXPORT, realpath_ret, MAX_PATHLEN);
	    free(realpath_ret);
	    /* } */
	}

	/* fprintf(stdout, " is %s\n", DIRPATH_SAVED_PROJ); */
    }
    window_pop_modal(main_win);
    Timeline *tl = ACTIVE_TL;
    tl->needs_redraw = true;

    return 0;
}

void user_tl_write_mixdown_to_wav(void *nullarg)
{
    Layout *mod_lt = layout_add_child(main_win->layout);
    layout_set_default_dims(mod_lt);
    Modal *save_wav = modal_create(mod_lt);
    modal_add_header(save_wav, "Export WAV", &color_global_light_grey, 3);
    modal_add_p(save_wav, "Export a mixdown of the current timeline, from the in-mark to the out-mark, in .wav format.", &color_global_light_grey);
    modal_add_header(save_wav, "Filename:", &color_global_light_grey, 5);
    static char wavfilename[MAX_NAMELENGTH];
    /* char *wavfilename = malloc(sizeof(char) * 255); */
    int i=0;
    char c;
    while ((c = proj->name[i]) != '.' && c != '\0') {
	wavfilename[i] = c;
	i++;
    }
    wavfilename[i] = '\0';
    strcat(wavfilename, ".wav");
    modal_add_textentry(
	save_wav,
	wavfilename,
	MAX_NAMELENGTH,
	txt_name_validation,
	file_ext_completion_wav,
	NULL);
    
    modal_add_p(save_wav, "\t\t|\t\t<tab>\tv\t\t|\t\t\tS-<tab>\t^\t\t|\t\tC-<ret>\tSubmit (save as)\t\t|", &color_global_light_grey);
    /* modal_add_op(save_wav, "\t\t(type <ret> to accept name)", &color_global_light_grey); */
    modal_add_header(save_wav, "Location:", &color_global_light_grey, 5);
    modal_add_dirnav(save_wav, DIRPATH_EXPORT, dir_to_tline_filter_save);
    modal_add_button(save_wav, "Save .wav file", submit_save_wav_form);
    /* save_as->submit_form = submit_save_as_form; */
    save_wav->submit_form = submit_save_wav_form;
    window_push_modal(main_win, save_wav);
    modal_reset(save_wav);
    /* fprintf(stdout, "about to call move onto\n"); */
    modal_move_onto(save_wav);
}


/* Deprecated; replaced by user_tl_cliprefs_delete */
void DEPRECATED_user_tl_cliprefs_destroy(void *nullarg)
{
    Timeline *tl = ACTIVE_TL;
    if (tl) {
	timeline_destroy_grabbed_cliprefs(tl);
    }
    if (proj->dragging) {
	status_stat_drag();
    }
    tl->needs_redraw = true;
}

void user_tl_delete_generic(void *nullarg)
{
    TabView *tv;
    if ((tv = main_win->active_tabview) && strcmp(tv->title, "Track Effects") == 0) {
	Page *active = tv->tabs[tv->current_tab];
	Effect *e = active->linked_obj;
	status_cat_callstr(" effect");
	effect_delete(e, false);
	return;
    }
    if (proj->recording) transport_stop_recording();
    Timeline *tl = ACTIVE_TL;
    Track *t;
    ClickTrack *ct;
    if ((t = timeline_selected_track(tl)) && TRACK_AUTO_SELECTED(t)) {
	if (automation_handle_delete(t->automations[t->selected_automation])) {
	    tl->needs_redraw = true;
	    return;
	}
    } else if (tl->dragging_keyframe) {
	status_cat_callstr(" selected keyframe");
	keyframe_delete(tl->dragging_keyframe);
	tl->dragging_keyframe = NULL;
	tl->needs_redraw = true;
	return;
    } else if ((ct = timeline_selected_click_track(tl))) {
	click_track_delete_segment_at_cursor(ct);
	status_cat_callstr(" click track segment");
    } else {
	/* if (tl->dragging_keyframe) { */
	/* 	keyframe_delete(tl->dragging_keyframe); */
	/* 	tl->dragging_keyframe = NULL; */
	/* 	return; */
	/* } */
	if (tl) {
	    status_cat_callstr(" grabbed clip(s)");
	    timeline_delete_grabbed_cliprefs(tl);
	}
	if (proj->dragging) {
	    status_stat_drag();
	}
    }
    tl->needs_redraw = true;
}

/* source mode */

void user_source_play(void *nullarg)
{
    if (proj->src_play_speed <= 0.0f) {
	proj->src_play_speed = 1.0f;
	transport_start_playback();
    } else {
	proj->src_play_speed *= 2.0f;
	status_stat_playspeed();
    }
}

void user_source_pause(void *nullarg)
{
    proj->src_play_speed = 0;
    transport_stop_playback();
}

void user_source_rewind(void *nullarg)
{
    if (proj->src_play_speed >= 0.0f) {
	proj->src_play_speed = -1.0f;
	transport_start_playback();
    } else {
	proj->src_play_speed *= 2.0f;
	status_stat_playspeed();
    }
}

void user_source_play_slow(void *nullarg)
{
    proj->src_play_speed = SLOW_PLAYBACK_SPEED;
    status_stat_playspeed();
    transport_start_playback();
}

void user_source_rewind_slow(void *nullarg)
{
    proj->src_play_speed = -1 * SLOW_PLAYBACK_SPEED;
    status_stat_playspeed();
    transport_start_playback();
}

void user_source_set_in_mark(void *nullarg)
{
    Timeline *tl = ACTIVE_TL;
    transport_set_mark(tl, true);
}

void user_source_set_out_mark(void *nullarg)
{
    Timeline *tl = ACTIVE_TL;
    transport_set_mark(tl, false);
}

void user_modal_next(void *nullarg)
{
    if (main_win->num_modals == 0) {
	return;
    }
    Modal *modal = main_win->modals[main_win->num_modals - 1];
    modal_next(modal);
}

void user_modal_previous(void *nullarg)
{
    if (main_win->num_modals == 0) {
	return;
    }
    Modal *modal = main_win->modals[main_win->num_modals - 1];
    modal_previous(modal);
}

void user_modal_next_escape(void *nullarg)
{
    if (main_win->num_modals == 0) {
	return;
    }
    Modal *modal = main_win->modals[main_win->num_modals - 1];
    modal_next_escape(modal);
}

void user_modal_previous_escape(void *nullarg)
{
    if (main_win->num_modals == 0) {
	return;
    }
    Modal *modal = main_win->modals[main_win->num_modals - 1];
    modal_previous_escape(modal);

}

void user_modal_select(void *nullarg)
{
    if (main_win->num_modals == 0) {
	return;
    }
    Modal *modal = main_win->modals[main_win->num_modals - 1];
    modal_select(modal);
}

void user_modal_dismiss(void *nullarg)
{
    while (main_win->num_menus > 0) {
	window_pop_menu(main_win);
    }
    if (main_win->num_modals == 0) return;
    Modal *m = main_win->modals[main_win->num_modals - 1];
    if (m->x->action) m->x->action(NULL, NULL);
    /* window_pop_modal(main_win); */
    Timeline *tl = ACTIVE_TL;
    tl->needs_redraw = true;
}

void user_modal_submit_form(void *nullarg)
{
    fprintf(stdout, "submit form\n");
    if (main_win->num_modals == 0) {
	return;
    }
    Modal *modal = main_win->modals[main_win->num_modals - 1];
    modal_submit_form(modal);
}

void user_text_edit_escape(void *nullarg)
{
    if (main_win->txt_editing) {
	txt_stop_editing(main_win->txt_editing);
    }
    Timeline *tl = ACTIVE_TL;
    timeline_reset(tl, false);

    /* In modals / tabview, push a new tab keypress to move up or down through fields */
    /* fprintf(stderr, "ESCAPE TEXT EDIT!! TOP MODE: %d\n", TOP_MODE); */
    if (TOP_MODE == MODAL || TOP_MODE == TABVIEW) {
	/* fprintf(stderr, "PUSH\n"); */
	SDL_Event e;
	e.type = SDL_KEYDOWN;
	e.key.keysym.scancode = SDL_SCANCODE_TAB;
	e.key.keysym.sym = '\t';

	SDL_PushEvent(&e);

	e.type = SDL_KEYUP;
	e.key.keysym.scancode = SDL_SCANCODE_TAB;
	e.key.keysym.sym = '\t';

	SDL_PushEvent (&e);
    }
}

void user_text_edit_full_escape(void *nullarg)
{
    if (main_win->txt_editing) {
	txt_stop_editing(main_win->txt_editing);
    }
    Timeline *tl = ACTIVE_TL;
    timeline_reset(tl, false);

}

void user_text_edit_backspace(void *nullarg)
{
    if (main_win->txt_editing) {
	txt_edit_backspace(main_win->txt_editing);
    }
}

void user_text_edit_cursor_left(void *nullarg)
{
    if (main_win->txt_editing) {
	txt_edit_move_cursor(main_win->txt_editing, true);
    }

}

void user_text_edit_cursor_right(void *nullarg)
{
    if (main_win->txt_editing) {
	txt_edit_move_cursor(main_win->txt_editing, false);
    }
}

void user_text_edit_select_all(void *nullarg)
{
    if (main_win->txt_editing) {
	txt_edit_select_all(main_win->txt_editing);
    }
}

void user_tabview_next_escape(void *nullarg)
{
    TabView *tv = main_win->active_tabview;
    Page *page = NULL;
    if (tv) {
	page = tv->tabs[tv->current_tab];
    } else {
	page = main_win->active_page;
    }
    if (page) {
	page_next_escape(page);
    }
}

void user_tabview_previous_escape(void *nullarg)
{
    TabView *tv = main_win->active_tabview;
    Page *page = NULL;
    if (tv) {
	page = tv->tabs[tv->current_tab];
    } else {
	page = main_win->active_page;
    }
    if (page) {
	page_previous_escape(page);
    }
}

void user_tabview_enter(void *nullarg)
{
    TabView *tv = main_win->active_tabview;
    Page *page = NULL;
    if (tv) {
	page = tv->tabs[tv->current_tab];
    } else {
	page = main_win->active_page;
    }
    if (page) {
	page_enter(page);
    }
}

void user_tabview_left(void *nullarg)
{
    TabView *tv = main_win->active_tabview;
    Page *page = NULL;
    if (tv) {
	page = tv->tabs[tv->current_tab];
    } else {
	page = main_win->active_page;
    }
    if (page) {
	page_left(page);
    }
}

void user_tabview_right(void *nullarg)
{
    TabView *tv = main_win->active_tabview;
    Page *page = NULL;
    if (tv) {
	page = tv->tabs[tv->current_tab];
    } else {
	page = main_win->active_page;
    }
    if (page) {
	page_right(page);
    }
}

void user_tabview_next_tab(void *nullarg)
{
     TabView *tv = main_win->active_tabview;
     if (tv) {
	 tabview_next_tab(tv);
     }
}


void user_tabview_previous_tab(void *nullarg)
{
     TabView *tv = main_win->active_tabview;
     if (tv) {
	 tabview_previous_tab(tv);
     }
}

void user_tabview_move_current_tab_left(void *nullarg)
{
     TabView *tv = main_win->active_tabview;
     if (tv) {
	 tabview_move_current_tab_left(tv);
     }
}

void user_tabview_move_current_tab_right(void *nullarg)
{
     TabView *tv = main_win->active_tabview;
     if (tv) {
	 tabview_move_current_tab_right(tv);
     }
}

void user_tabview_escape(void *nullarg)
{
    Timeline *tl = ACTIVE_TL;
    if (main_win->active_tabview) {
	tabview_close(main_win->active_tabview);
	tl->needs_redraw = true;
	return;
    }
}

void user_autocomplete_next(void *nullarg)
{
    if (!main_win->ac_active) {
	fprintf(stderr, "Error: in AC mode without active ac\n");
	window_pop_mode(main_win);
	return;
    }

    AutoCompletion *ac = &main_win->ac;
    autocompletion_reset_selection(ac, ac->selection + 1);
}
void user_autocomplete_previous(void *nullarg)
{
    if (!main_win->ac_active) {
	fprintf(stderr, "Error: in AC mode without active ac\n");
	window_pop_mode(main_win);
	return;
    }

    AutoCompletion *ac = &main_win->ac;
    autocompletion_reset_selection(ac, ac->selection - 1);

}

void user_autocomplete_escape(void *nullarg)
{
    autocompletion_escape();
    Timeline *tl = ACTIVE_TL;
    tl->needs_redraw = true;
}

void user_autocomplete_select(void *nullarg)
{
    if (!main_win->ac_active) {
	fprintf(stderr, "Error: in AC mode without active ac\n");
	window_pop_mode(main_win);
	return;
    }
    /* user_autocomplete_escape(NULL); */
    AutoCompletion *ac = &main_win->ac;
    autocompletion_select(ac);
    Timeline *tl = ACTIVE_TL;
    tl->needs_redraw = true;
}

