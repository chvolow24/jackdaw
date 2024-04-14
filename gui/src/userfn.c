#include <stdio.h>

#include "input.h"
#include "menu.h"
#include "project.h"

extern Window *main_win;
extern Project *proj;

#define MENU_MOVE_BY 40

void user_global_expose_help()
{
    fprintf(stdout, "user_global_expose_help\n");
    Menu *new = input_create_master_menu();
    window_add_menu(main_win, new);
    window_push_mode(main_win, MENU_NAV);
    
}

void user_global_quit()
{
    fprintf(stdout, "user_global_quit\n");
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
		    item->onclick(NULL, NULL);
		    window_pop_menu(main_win);
		    window_pop_mode(main_win);
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

void user_tl_play()
{
    fprintf(stdout, "user_tl_play\n");
}

void user_tl_pause()
{
    fprintf(stdout, "user_tl_pause\n");
}

void user_tl_rewind()
{
    fprintf(stdout, "user_tl_rewind\n");
}

void user_tl_set_mark_out()
{
    fprintf(stdout, "user_tl_set_mark_out\n");
}
void user_tl_set_mark_in()
{
    fprintf(stdout, "user_tl_set_mark_in\n");
}


void user_tl_add_track()
{
    fprintf(stdout, "user_tl_add_track\n");
    if (!proj) {
	fprintf(stderr, "Error: user call to add track w/o global project\n");
	exit(1);
    }
    Timeline *tl = proj->timelines[0]; // TODO: get active timeline;

    timeline_add_track(tl);   
}

static void track_select_n(int n)
{
    Timeline *tl = proj->timelines[proj->active_tl_index];
    if (tl->num_tracks <= n) {
	return;
    }
    bool *active = &(tl->tracks[n]->active);
    *active = !(*active);

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
    Track *track;
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


