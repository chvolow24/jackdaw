#include <stdio.h>
#include "menu.h"

extern Menu *main_menu;

#define MENU_MOVE_BY 20

void user_global_expose_help()
{
    fprintf(stdout, "user_global_expose_help\n");
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
    Menu *m = main_menu; /* TODO: replace 'main_menu' */
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
    }
    
    fprintf(stdout, "user_menu_nav_next_item\n");
}

void user_menu_nav_prev_item()
{
    Menu *m = main_menu; /* TODO: replace 'main_menu' */
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
    Menu *m = main_menu;
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
    Menu *m = main_menu;
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
    Menu *m = main_menu; /* Todo: replace main menu */
    if (m->sel_col < m->num_columns) {
	MenuColumn *col = m->columns[m->sel_col];
	if (col->sel_sctn < col->num_sections) {
	    MenuSection *sctn = col->sections[col->sel_sctn];
	    if (sctn->sel_item < sctn->num_items) {
		MenuItem *item = sctn->items[sctn->sel_item];

		if (item->onclick != user_menu_nav_choose_item) {  /* lol */
		    item->onclick(NULL, NULL);
		}
	    }
	}
    }
   
}

void user_menu_translate_up()
{
    Menu *m = main_menu;
    menu_translate(m, 0, -1 * MENU_MOVE_BY);
}

void user_menu_translate_down()
{
    Menu *m = main_menu;
    menu_translate(m, 0, MENU_MOVE_BY);

}


void user_menu_translate_left()
{
    Menu *m = main_menu;
    menu_translate(m, -1 * MENU_MOVE_BY, 0);

}

void user_menu_translate_right()
{
    Menu *m = main_menu;
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


