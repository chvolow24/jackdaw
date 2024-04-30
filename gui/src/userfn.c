#include <stdio.h>

#include "audio_device.h"
#include "input.h"
#include "menu.h"
#include "project.h"
#include "textbox.h"
#include "transport.h"
#include "timeline.h"

#define MENU_MOVE_BY 40
#define TL_DEFAULT_XSCROLL 60
#define SLOW_PLAYBACK_SPEED 0.2f

extern Window *main_win;
extern Project *proj;
extern Mode **modes;

void user_global_expose_help()
{
    if (main_win->num_modes <= 0) {
	fprintf(stderr, "Error: no modes active in user_global_expose_help\n");
	return;
    }
    InputMode current_mode = main_win->modes[main_win->num_modes - 1];
    
    /* Menu *new = input_create_master_menu(); */
    Menu *new = input_create_menu_from_mode(current_mode);
    window_add_menu(main_win, new);
    window_push_mode(main_win, MENU_NAV);   
}

void user_global_quit()
{
    fprintf(stdout, "user_global_quit\n");
    exit(0); //TODO: add the i_state to window (or proj?) to quit naturally.
}

void user_global_undo()
{
    fprintf(stdout, "user_global_undo\n");
}

void user_global_redo()
{
    fprintf(stdout, "user_global_redo\n");
}



void user_menu_nav_next_item()
{
    Menu *m = window_top_menu(main_win);
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
    
    fprintf(stdout, "user_menu_nav_next_item\n");
}

void user_menu_nav_prev_item()
{
    Menu *m = window_top_menu(main_win);
    if (!m) {
	fprintf(stderr, "No menu on main window\n");	
	exit(1);
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
    
    fprintf(stdout, "user_menu_nav_prev_item\n");
}


void user_menu_nav_next_sctn()
{
    fprintf(stdout, "user_menu_nav_next_sctn\n");
}

void user_menu_nav_prev_sctn()
{

    fprintf(stdout, "user_menu_nav_prev_sctn\n");
}

void user_menu_nav_column_right()
{
    Menu *m = window_top_menu(main_win);
    if (!m) {
	fprintf(stderr, "No menu on main window\n");	
	exit(1);
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
    fprintf(stdout, "user_menu_nav_column_right\n");
}

void user_menu_nav_column_left()
{
    Menu *m = window_top_menu(main_win);
    if (!m) {
	fprintf(stderr, "No menu on main window\n");	
	exit(1);
    }

    if (m->sel_col == 255) {
	m->sel_col = 0;
    } else if (m->sel_col > 0) {
	m->sel_col--;
    }
    fprintf(stdout, "user_menu_nav_column_left\n");
}

void user_menu_nav_choose_item()
{
    fprintf(stdout, "user_menu_nav_choose_item\n");
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
		    item->onclick();
		    /* window_pop_menu(main_win); */
		}
	    }
	}
    }
}

void user_menu_translate_up()
{
    Menu *m = window_top_menu(main_win);
    if (!m) {
	fprintf(stderr, "No menu on main window\n");	
	exit(1);
    }
    menu_translate(m, 0, -1 * MENU_MOVE_BY);
}

void user_menu_translate_down()
{
    Menu *m = window_top_menu(main_win);
    if (!m) {
	fprintf(stderr, "No menu on main window\n");	
	exit(1);
    }
    menu_translate(m, 0, MENU_MOVE_BY);

}

void user_menu_translate_left()
{
    Menu *m = window_top_menu(main_win);
    if (!m) {
	fprintf(stderr, "No menu on main window\n");	
	exit(1);
    }
    menu_translate(m, -1 * MENU_MOVE_BY, 0);
}

void user_menu_translate_right()
{
    Menu *m = window_top_menu(main_win);
    if (!m) {
	fprintf(stderr, "No menu on main window\n");	
	exit(1);
    }
    menu_translate(m, MENU_MOVE_BY, 0);
}

void user_menu_dismiss()
{
    window_pop_menu(main_win);
    window_pop_mode(main_win);
}

void user_tl_play()
{
    if (proj->play_speed <= 0.0f) {
	proj->play_speed = 1.0f;
	transport_start_playback();
    } else {
	proj->play_speed *= 2.0f;
    }
    fprintf(stdout, "user_tl_play\n");
}

void user_tl_pause()
{
    proj->play_speed = 0;
    transport_stop_playback();
    fprintf(stdout, "user_tl_pause\n");
}

void user_tl_rewind()
{
    if (proj->play_speed >= 0.0f) {
	proj->play_speed = -1.0f;
	transport_start_playback();
    } else {
	proj->play_speed *= 2.0f;
    }
    fprintf(stdout, "user_tl_rewind\n");
}

void user_tl_play_slow()
{
    proj->play_speed = SLOW_PLAYBACK_SPEED;
    transport_start_playback();
}

void user_tl_rewind_slow()
{
    proj->play_speed = -1 * SLOW_PLAYBACK_SPEED;
    transport_start_playback();
}

void user_tl_move_right()
{
    timeline_scroll_horiz(TL_DEFAULT_XSCROLL);
}

void user_tl_move_left()
{
    timeline_scroll_horiz(TL_DEFAULT_XSCROLL * -1);
}

void user_tl_zoom_in()
{
    timeline_rescale(1.2, false);
}

void user_tl_zoom_out()
{
    timeline_rescale(0.8, false);
}

void user_tl_set_mark_out()
{
    Timeline *tl = proj->timelines[proj->active_tl_index];
    transport_set_mark(tl, false);
    fprintf(stdout, "user_tl_set_mark_out\n");
}
void user_tl_set_mark_in()
{
    Timeline *tl = proj->timelines[proj->active_tl_index];
    transport_set_mark(tl, true);
    fprintf(stdout, "user_tl_set_mark_in\n");
}

void user_tl_goto_mark_out()
{
    Timeline *tl = proj->timelines[proj->active_tl_index];
    transport_goto_mark(tl, false);
}

void user_tl_goto_mark_in()
{
    Timeline *tl = proj->timelines[proj->active_tl_index];
    transport_goto_mark(tl, true);
}


void user_tl_add_track()
{
    fprintf(stdout, "user_tl_add_track\n");
    if (!proj) {
	fprintf(stderr, "Error: user call to add track w/o global project\n");
	exit(1);
    }
    Timeline *tl = proj->timelines[proj->active_tl_index]; // TODO: get active timeline;
    timeline_add_track(tl);   
}

static void track_select_n(int n)
{
    Timeline *tl = proj->timelines[proj->active_tl_index];
    if (tl->num_tracks <= n) {
	return;
    }
    Track *track = tl->tracks[n];
    bool *active = &(track->active);
    *active = !(*active);
    /* track->input->active = *active; */
    /* fprintf(stdout, "SETTING %s to %d\n", track->input->name, track->input->active); */
}

void user_tl_track_select_1()
{
    track_select_n(0);

}
void user_tl_track_select_2()
{
    track_select_n(1);
}

void user_tl_track_select_3()
{
    track_select_n(2);
}

void user_tl_track_select_4()
{
    track_select_n(3);
}

void user_tl_track_select_5()
{
    track_select_n(4);
}

void user_tl_track_select_6()
{
    track_select_n(5);
}


void user_tl_track_select_7()
{
    track_select_n(6);
}


void user_tl_track_select_8()
{
    track_select_n(7);
}

void user_tl_track_select_9()
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
    return ret;

}

static void deactivate_all_tracks(Timeline *tl)
{
    for (uint8_t i=0; i<tl->num_tracks; i++) {
	tl->tracks[i]->active = false;
    }	

}

void user_tl_track_activate_all()
{

    Timeline *tl = proj->timelines[proj->active_tl_index];
    if (activate_all_tracks(tl)) {
	deactivate_all_tracks(tl);
    }
}


void user_tl_track_selector_up()
{
    Timeline *tl = proj->timelines[proj->active_tl_index];
    if (tl->track_selector > 0) {
	tl->track_selector--;
    }
    
}

void user_tl_track_selector_down()
{
    Timeline *tl = proj->timelines[proj->active_tl_index];
    if (tl->track_selector < tl->num_tracks -1) {
	tl->track_selector++;
    }
}

void user_tl_track_activate_selected()
{
    Timeline *tl = proj->timelines[proj->active_tl_index];
    track_select_n(tl->track_selector);
}

void project_draw();
void user_tl_track_rename()
{
    /* window_pop_menu(main_win); */
    /* window_pop_mode(main_win); */
    Timeline *tl = proj->timelines[proj->active_tl_index];
    Track *track = tl->tracks[tl->track_selector];
    if (track) {
	txt_edit(track->tb_name->text, project_draw);
    }
    main_win->i_state = 0;
    /* fprintf(stdout, "DONE track edit\n"); */
}

SDL_Color mute_red = {255, 0, 0, 100};
SDL_Color solo_yellow = {255, 200, 0, 130};
extern SDL_Color textbox_default_bckgrnd_clr;

void user_tl_mute()
{
    Timeline *tl = proj->timelines[proj->active_tl_index];
    Track *track = tl->tracks[tl->track_selector];
    track->muted = !track->muted;
    if (track->muted) {
	textbox_set_background_color(track->tb_mute_button, &mute_red);
    } else {
	textbox_set_background_color(track->tb_mute_button, &textbox_default_bckgrnd_clr);
    }
}

/* void user_tl_track_vol_up_toggle() */
/* { */
/*     fprintf(stdout, "vol up toggle\n"); */
/*     Timeline *tl = proj->timelines[proj->active_tl_index]; */
/*     if (proj->vol_changing) { */
/* 	proj->vol_changing = NULL; */
/*     } else { */
/*         proj->vol_changing = tl->tracks[tl->track_selector]; */
/* 	proj->vol_up = true; */
/*     } */
/* } */


void user_tl_track_vol_up()
{
    Timeline *tl = proj->timelines[proj->active_tl_index];
    /* proj->vol_changing = tl->tracks[tl->track_selector]; */
    proj->vol_changing = true;
    proj->vol_up = true;
    
}

void user_tl_track_vol_down()
{
    Timeline *tl = proj->timelines[proj->active_tl_index];
    /* proj->vol_changing = tl->tracks[tl->track_selector]; */
    proj->vol_changing = true;
    proj->vol_up = false;

}

void user_tl_track_pan_left()
{
    Timeline *tl = proj->timelines[proj->active_tl_index];
    proj->pan_changing = tl->tracks[tl->track_selector];
    proj->pan_right = false;

}

void user_tl_track_pan_right()
{
   Timeline *tl = proj->timelines[proj->active_tl_index];
    proj->pan_changing = tl->tracks[tl->track_selector];
    proj->pan_right = true;
}


void user_tl_solo()
{
    /* Timeline *tl = proj->timelines[proj->active_tl_index]; */
    /* Track *track = tl->tracks[tl->track_selector]; */
    /* track->solo = !track->muted; */
    /* if (track->muted) { */
    /* 	textbox_set_background_color(track->tb_mute_button, &mute_red); */
    /* } else { */
    /* 	textbox_set_background_color(track->tb_mute_button, &textbox_default_bckgrnd_clr); */
    /* } */

}


void user_tl_record()
{
    /* Timeline *tl = pro
       j->timelines[proj->active_tl_index]; */
    fprintf(stdout, "user_tl_record\n");
    if (proj->recording) {
	transport_stop_recording();
    } else {
	transport_start_recording();
    }
}

static int32_t cr_len(ClipRef *cr)
{
    if (cr->out_mark_sframes <= cr->in_mark_sframes) {
	return cr->clip->len_sframes;
    } else {
	return cr->out_mark_sframes - cr->in_mark_sframes;
    }
}

static ClipRef *clip_at_point_in_track(Track *track)
{
    for (int i=track->num_clips-1; i>=0; i--) {
	ClipRef *cr = track->clips[i];
	if (cr->pos_sframes <= track->tl->play_pos_sframes && cr->pos_sframes + cr_len(cr) >= track->tl->play_pos_sframes) {
	    return cr;
	}
    }
    return NULL;
}

static ClipRef *clip_at_point()
{
    Timeline *tl = proj->timelines[proj->active_tl_index];
    Track *track = tl->tracks[tl->track_selector];
    for (int i=track->num_clips -1; i>=0; i--) {
	ClipRef *cr = track->clips[i];
	if (cr->pos_sframes <= tl->play_pos_sframes && cr->pos_sframes + cr_len(cr) >= tl->play_pos_sframes) {
	    return cr;
	}
    }
    return NULL;
}

void user_tl_clipref_grab_ungrab()
{
    Timeline *tl = proj->timelines[proj->active_tl_index];
    Track *track = NULL;
    ClipRef *cr =  NULL;
    ClipRef *crs[MAX_GRABBED_CLIPS];
    uint8_t num_clips = 0;
    bool clip_grabbed = false;
    bool had_active_track = false;
    for (uint8_t i=0; i<tl->num_tracks; i++) {
	track = tl->tracks[i];
	if (track->active) {
	    had_active_track = true;
	    cr = clip_at_point_in_track(track);
	    if (cr && !cr->grabbed) {
		tl->grabbed_clips[tl->num_grabbed_clips] = cr;
		tl->num_grabbed_clips++;
		cr->grabbed = true;
		clip_grabbed = true;
		
	    }
	}
    }
    if (!had_active_track) {
	track = tl->tracks[tl->track_selector];
	cr = clip_at_point_in_track(track);
	if (cr && !cr->grabbed) {
	    tl->grabbed_clips[tl->num_grabbed_clips] = cr;
	    tl->num_grabbed_clips++;
	    cr->grabbed = true;
	    clip_grabbed = true;
	}
    }
    if (!clip_grabbed) {
	for (uint8_t i=0; i<tl->num_grabbed_clips; i++) {
	    cr = tl->grabbed_clips[i];
	    cr->grabbed = false;
	}
	tl->num_grabbed_clips = 0;
    }
    
}


void user_tl_load_clip_at_point_to_src()
{
    ClipRef *cr = clip_at_point();
    if (cr) {
	proj->src_clip = cr->clip;
	proj->src_in_sframes = 0;
	proj->src_play_pos_sframes = 0;
	proj->src_out_sframes = 0;
	fprintf(stdout, "Src clip name? %s\n", proj->src_clip->name);
	txt_set_value_handle(proj->source_name_tb->text, proj->src_clip->name);
    }
}

void user_tl_activate_source_mode()
{
    if (!proj->source_mode) {
	if (proj->src_clip) {
	    proj->source_mode = true;
	    window_push_mode(main_win, SOURCE);
	}
    } else {
	proj->source_mode = false;
	window_pop_mode(main_win);
    }
}

void user_tl_drop_from_source()
{
    Timeline *tl = proj->timelines[proj->active_tl_index];
    Track *track = tl->tracks[tl->track_selector];
    ClipRef *cr = track_create_clip_ref(track, proj->src_clip, tl->play_pos_sframes, false);
    cr->in_mark_sframes = proj->src_in_sframes;
    cr->out_mark_sframes = proj->src_out_sframes;
    clipref_reset(cr);
}


void user_tl_add_new_timeline()
{
    proj->active_tl_index = project_add_timeline(proj, "New Timeline");
    project_reset_tl_label(proj);
}

void user_tl_previous_timeline()
{
    if (proj->active_tl_index > 0) {
	proj->active_tl_index--;
	project_reset_tl_label(proj);
    }
}

void user_tl_next_timeline()
{
    if (proj->active_tl_index < proj->num_timelines - 1) {
	proj->active_tl_index++;
	project_reset_tl_label(proj);
    }
}

/* source mode */

void user_source_play()
{
    if (proj->src_play_speed <= 0.0f) {
	proj->src_play_speed = 1.0f;
	transport_start_playback();
    } else {
	proj->src_play_speed *= 2.0f;
    }
}

void user_source_pause()
{
    proj->src_play_speed = 0;
    transport_stop_playback();
}

void user_source_rewind()
{
    if (proj->src_play_speed >= 0.0f) {
	proj->src_play_speed = -1.0f;
	transport_start_playback();
    } else {
	proj->src_play_speed *= 2.0f;
    }
}

void user_source_play_slow()
{
    proj->src_play_speed = SLOW_PLAYBACK_SPEED;
    transport_start_playback();
}

void user_source_rewind_slow()
{
    proj->src_play_speed = -1 * SLOW_PLAYBACK_SPEED;
    transport_start_playback();
}

void user_source_set_in_mark()
{
    transport_set_mark(NULL, true);
}

void user_source_set_out_mark()
{
    transport_set_mark(NULL, false);
}




/*
}

void user_tl_pause()
{


void user_tl_rewind()
{
*/
