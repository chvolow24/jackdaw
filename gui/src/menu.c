#include "SDL_render.h"
#include "color.h"
#include "geometry.h"
#include "layout.h"
#include "menu.h"
#include "text.h"
#include "textbox.h"

#define MENU_TXT_SIZE 12
#define MENU_STD_ITEM_PAD_H 4
#define MENU_STD_ITEM_PAD_V 2
#define MENU_STD_ITEM_V_SPACING 2
#define MENU_STD_SECTION_PAD 16
#define MENU_STD_COL_LABEL_H 8
#define MENU_STD_COLUMN_PAD 8
#define MENU_STD_HEADER_PAD 10

#define MENU_STD_CORNER_RAD STD_CORNER_RAD


extern Window *main_win;

SDL_Color menu_std_clr_inner_border = (SDL_Color) {200, 200, 200, 255};
SDL_Color menu_std_clr_outer_border = (SDL_Color) {10, 10, 10, 255};
SDL_Color menu_std_clr_bckgrnd = (SDL_Color) {40, 40, 40, 255};
SDL_Color menu_std_clr_txt = (SDL_Color) {255, 255, 255, 255};
SDL_Color menu_std_clr_annot_txt = (SDL_Color) {200, 200, 200, 255};
SDL_Color menu_std_clr_highlight = (SDL_Color) {50, 50, 255, 255};
SDL_Color menu_std_clr_sctn_div = (SDL_Color) {200, 200, 200, 255};

SDL_Color color_clear = (SDL_Color) {0, 0, 0, 0};

extern struct colors colors;
/* int menu_std_border_rad = 24; */

	
static void menu_reset_textboxes(Menu *menu)
{
    for (int c=0; c<menu->num_columns; c++) {
	MenuColumn *col = menu->columns[c];
	for (int s=0; s<col->num_sections; s++) {
	    MenuSection *sctn = col->sections[s];
	    for (int i=0; i<sctn->num_items; i++) {
		MenuItem *item = sctn->items[i];
		txt_reset_display_value(item->tb->text);
		if (item->annot_tb) {
		    txt_reset_display_value(item->annot_tb->text);
		}		    
	    }
	}
    }
}

/* Sets section ys, assuming h already set */
/* static void menu_column_rectify_sections(MenuColumn *col) */
/* { */
/*     int y_logical = MENU_STD_SECTION_PAD; */
/*     if (col->label_tb) { */
/* 	y_logical += col->label_tb->layout->h.value; */
/*     } */
/*     for (int i=0; i<col->num_sections; i++) { */
/* 	MenuSection *sctn = col->sections[i]; */
/* 	sctn->layout->y.value = y_logical; */
/* 	y_logical += sctn->layout->h.value + MENU_STD_SECTION_PAD; */
/*     } */
/* } */

/* /\* Sets col x and menu dims. Assumes columns width has already been set *\/ */
/* static void menu_rectify_columns(Menu *menu) */
/* { */
/*     layout_reset(menu->layout); */
/*     layout_size_to_fit_children(menu->layout->children[1], true, MENU_STD_COLUMN_PAD); */
/*     layout_size_to_fit_children(menu->layout, true, 0); */
/*     layout_reset(menu->layout); */
/* } */
	
    

Menu *menu_create(Layout *layout, Window *window)
{
    Menu *menu = calloc(1, sizeof(Menu));
    menu->num_columns = 0;
    menu->layout = layout;
    menu->selected = NULL;
    menu->sel_col = 255;
    /* Creat header layout */
    menu->header_container = layout_add_child(layout);
    /* NOTE: header is sized to fit children, and therefore not SCALE */
    /* header->w.type = SCALE; */
    /* header->w.value = 1.0; */
    menu->cols_container = layout_add_child(layout);
    menu->cols_container->w.type = SCALE;
    menu->cols_container->w.value = 1.0;
    menu->cols_container->y.type = STACK;
    menu->cols_lt = layout_add_child(menu->cols_container);
    menu->cols_lt->w.type = SCALE;
    menu->cols_lt->h.type = SCALE;
    menu->cols_lt->w.value = 1.0;
    menu->cols_lt->h.value = 1.0;
    /* main_contents->h.type = REVREL; */
    /* layout_add_complementary_child(layout, H); */
    
    menu->window = window;
    /* menu->font = ttf_get_font_at_size(window->std_font, MENU_TXT_SIZE); */
    menu->font = window->std_font;
    /* menu->text_size = MENU_TXT_SIZE; */
    menu->title = NULL;
    menu->description = NULL;
    menu->header = NULL;
    return menu;
}


void layout_write(FILE *out, Layout *lt, int indent);
Menu *menu_create_at_point(int x_pix, int y_pix)
{
    Layout *lt = layout_add_child(main_win->layout);
    /* layout_set_default_dims(lt); */
    lt->x.value = x_pix / main_win->dpi_scale_factor;
    lt->y.value = y_pix / main_win->dpi_scale_factor;
    /* lt->w.value.intval = 1200; */
    /* lt->h.value.intval = 900; */
    layout_reset(lt);
    /* layout_write(stdout, lt, 0); */
    Menu *menu = menu_create(lt, main_win);
    return menu;
}


void menu_reset_layout(Menu *menu)
{
    layout_force_reset(menu->layout);
    layout_size_to_fit_children(menu->cols_lt, true, MENU_STD_COLUMN_PAD);
    layout_size_to_fit_children(menu->cols_container, true, MENU_STD_COLUMN_PAD);
    layout_size_to_fit_children(menu->layout, true, MENU_STD_COLUMN_PAD);

    /* Translate if overflows window */
    int overflow_h = menu->layout->rect.x + menu->layout->rect.w - main_win->w_pix;
    int overflow_v = menu->layout->rect.y + menu->layout->rect.h - main_win->h_pix;
    if (overflow_h > 0) {
	menu->layout->x.value -= overflow_h;
	if (menu->layout->x.value < 0) menu->layout->x.value = 0;
    }
    if (overflow_v > 0) {
	menu->layout->y.value -= overflow_v;
	if (menu->layout->y.value < 0) menu->layout->y.value = 0;
    }

    /* Overflow individual columns */
    if (menu->layout->rect.h > menu->window->h_pix) {
	menu->layout->h.value = menu->window->layout->h.value;
	menu->cols_container->h.type = REVREL;
	menu->cols_container->h.value = MENU_STD_SECTION_PAD;
	menu->cols_lt->h.type = REVREL;
	menu->cols_lt->h.value = 0;

	/* fprintf(stderr, "SET COLS CONTAINER TO REVREL\n"); */
	/* menu->layout->children[1]->h.value =  */
	layout_reset(menu->layout);
	for (int i=0; i<menu->num_columns; i++) {
	    int col_btm_y = menu->columns[i]->layout->rect.y + menu->columns[i]->layout->rect.h;
	    int menu_btm_y = menu->layout->rect.y + menu->layout->rect.h;
	    if (col_btm_y > menu_btm_y - MENU_STD_SECTION_PAD) {
		menu->columns[i]->overflow = true;
		menu->columns[i]->container->h.type = REVREL;
		menu->columns[i]->container->h.value = 0;
		layout_force_reset(menu->columns[i]->container);
	    }
	}
    }

    /* Overflow columns container */
    if (menu->layout->rect.w > menu->window->w_pix) {
	menu->layout->w.value = menu->window->layout->w.value;
	menu->cols_container->w.type = REVREL;
	menu->cols_container->w.value = 0;
	menu->cols_overflow = true;
    }
    
    layout_reset(menu->layout);
    menu_reset_textboxes(menu);
}

void menu_add_header(Menu *menu, const char *title, const char *description)
{
    menu->title = title;
    menu->description = description;
    Layout *header_lt = menu->header_container;
    header_lt->x.value = MENU_STD_COLUMN_PAD + MENU_STD_ITEM_PAD_H * 2;
    header_lt->y.value = MENU_STD_SECTION_PAD + MENU_STD_ITEM_PAD_V;
    /* header_lt->w.type = REL; */
    /* header_lt->w.value = menu->layout->w.value - 2 * header_lt->x.value; */
    layout_reset(header_lt);
    TextArea *header = txt_area_create(description, header_lt, main_win->mono_font, MENU_TXT_SIZE, menu_std_clr_annot_txt, menu->window);
    layout_size_to_fit_children_h(header_lt, true, 0);
    
    menu->cols_container->y.type = STACK;
    menu->cols_container->y.value = MENU_STD_HEADER_PAD;
    /* layout_size_to_fit_children_h(header_lt, true, 0); */
    /* layout_size_to_fit_children_h(menu->layout, true, 0); */
    menu->header = header;
    /* int header_h = header->layout->h.value; */

    menu_reset_layout(menu);
}

MenuColumn *menu_column_add(Menu *menu, const char *label)
{
    Layout *content_lt = menu->cols_lt;
    MenuColumn *col = calloc(1, sizeof(MenuColumn));
    col->num_sections = 0;
    col->sel_sctn = 255;
    col->label = label;
    col->menu = menu;
    col->container = layout_add_child(content_lt);
    col->container->x.type = STACK;
    col->container->x.value = MENU_STD_COLUMN_PAD;
    col->container->y.value = MENU_STD_COLUMN_PAD;
    col->container->h.value = MENU_STD_SECTION_PAD;
    col->layout = layout_add_child(col->container);
    
    /* col->layout */
    /* char lt_name[7]; */
    /* snprintf(lt_name, 7, "col_%02d", menu->num_columns); */
    /* lt_name[6] = '\0'; */
    /* layout_set_name(col->layout, lt_name); */
    /* col->layout->x.type = STACK; */
    /* col->layout->x.value = MENU_STD_COLUMN_PAD; */
    /* col->layout->y.value = MENU_STD_COLUMN_PAD; */
    /* col->layout->h.value = MENU_STD_SECTION_PAD; */
    if (col->label && strlen(col->label) != 0) {
	Layout *label_lt = layout_add_child(col->layout);
	label_lt->w.type = SCALE;
	label_lt->w.value = 1.0;
	label_lt->y.value = MENU_STD_SECTION_PAD;
	label_lt->h.value = MENU_STD_COL_LABEL_H;
	col->label_tb = textbox_create_from_str(
	    label,
	    label_lt,
	    main_win->bold_font,
	    MENU_TXT_SIZE,
	    main_win);
	col->label_tb->text->align = CENTER_LEFT;
	textbox_set_trunc(col->label_tb, false);
	textbox_set_text_color(col->label_tb, &menu_std_clr_txt);
	textbox_set_background_color(col->label_tb, &colors.clear);
	textbox_set_border_color(col->label_tb, &colors.clear);
	textbox_size_to_fit(col->label_tb, MENU_STD_ITEM_PAD_H, MENU_STD_ITEM_PAD_V);
	col->layout->h.value += label_lt->h.value;// + label_lt->y.value;

    }
    menu->columns[menu->num_columns] = col;
    menu->num_columns++;
    return col;
}

MenuSection *menu_section_add(MenuColumn *col, const char *label)
{
    MenuSection *sctn = malloc(sizeof(MenuSection));
    sctn->num_items = 0;
    sctn->sel_item = 255;
    sctn->label = label;
    sctn->column = col;
    sctn->layout = layout_add_child(col->layout);
    sctn->layout->y.type = STACK;
    sctn->layout->y.value = 0.0;
    sctn->layout->w.type = SCALE;
    sctn->layout->w.value = 1.0;
    char sctn_name[8];
    snprintf(sctn_name, 8, "sctn_%02d", col->num_sections);
    sctn_name[7] = '\0';
    layout_set_name(sctn->layout, sctn_name);
    col->sections[col->num_sections] = sctn;
    col->num_sections++;
    col->layout->h.value += MENU_STD_SECTION_PAD;
    return sctn;
}

MenuItem *menu_item_add(
    MenuSection *sctn,
    const char *label,
    const char *annotation,
    void (*onclick)(void *target),
    void *target
)
{
    MenuItem *item = malloc(sizeof(MenuItem));
    item->annot_tb = NULL;
    item->label = label;
    item->annotation = annotation;
    item->available = true;
    item->onclick = onclick;
    item->target = target;
    item->free_target_on_destroy = false;
    item->section = sctn;
    item->selected = false;
    item->layout = layout_add_child(sctn->layout);
    char item_name[8];
    snprintf(item_name, 8, "item_%02d", sctn->num_items);
    item_name[7] = '\0';
    layout_set_name(item->layout, item_name);
    sctn->items[sctn->num_items] = item;
    sctn->num_items++;

    Layout *tb_lt = layout_add_child(item->layout);
    Layout *annot_lt = layout_add_child(item->layout);
    /* tb_lt->h.type = SCALE; */
    /* tb_lt->h.value = 1.0; */
    /* annot_lt->h.type = SCALE; */
    /* annot_lt->h.value = 1.0; */

    Window *win = sctn->column->menu->window;
    /* TTF_Font *font = sctn->column->menu->font; */
    Font *font = sctn->column->menu->font;



    /* Create and initialize main tb */
    item->tb = textbox_create_from_str(
	(char *)label,
	tb_lt,
	font,
	MENU_TXT_SIZE,
	win
	);
    item->tb->text->align = CENTER_LEFT;
    textbox_set_trunc(item->tb, false);
    textbox_set_text_color(item->tb, &menu_std_clr_txt);
    textbox_set_background_color(item->tb, &colors.clear);
    textbox_set_border_color(item->tb, &colors.clear);
    

    /* Create and initialize annotation tb */
    if (annotation) {
	item->annot_tb = textbox_create_from_str(
	    (char *)annotation,
	    annot_lt,
	    font,
	    MENU_TXT_SIZE,
	    win
	    );
	item->annot_tb->text->align = CENTER_RIGHT;
	textbox_set_trunc(item->annot_tb, false);
	textbox_set_text_color(item->annot_tb, &menu_std_clr_annot_txt);
	textbox_set_background_color(item->annot_tb, &colors.clear);
	textbox_set_border_color(item->annot_tb, &colors.clear);
    }


    textbox_size_to_fit(item->tb, MENU_STD_ITEM_PAD_H, MENU_STD_ITEM_PAD_V);
    
    if (annotation) {
	textbox_size_to_fit(item->annot_tb, MENU_STD_ITEM_PAD_H, MENU_STD_ITEM_PAD_V);
	item->annot_tb->layout->w.type = COMPLEMENT;
	item->annot_tb->layout->x.value = item->tb->layout->w.value;
    }
    
    int annot_pad = annotation ? item->annot_tb->layout->w.value + 5 * MENU_STD_ITEM_PAD_H : 0;
    int w_logical = item->tb->layout->w.value + annot_pad + 2 * MENU_STD_ITEM_PAD_H;
    int h_logical = item->tb->layout->h.value + MENU_STD_ITEM_PAD_V;    

    item->layout->x.value = 0;
    item->layout->y.type = STACK;
    item->layout->y.value = MENU_STD_ITEM_V_SPACING;
    item->layout->h.value = h_logical;
    item->layout->w.value = 1.0;
    item->layout->w.type = SCALE;
    MenuColumn *col = sctn->column;
    sctn->layout->w.value = 1.0;
    sctn->layout->w.type = SCALE;

    /* Reset layout before sizing to fit (reset sizes children) */
    layout_force_reset(col->container);
    layout_size_to_fit_children_v(sctn->layout, true, MENU_STD_ITEM_V_SPACING);
    if (w_logical > col->layout->w.value) {
	col->layout->w.value = w_logical;
	col->container->w.value = w_logical;
	layout_reset(col->container);
    }
    
    layout_size_to_fit_children_v(col->layout, true, MENU_STD_ITEM_V_SPACING);
    layout_size_to_fit_children_v(col->container, true, 0);
    menu_reset_layout(col->menu);
    return item;
}

void menu_item_destroy(MenuItem *item)
{
    if (item->target && item->free_target_on_destroy) {
	free(item->target);
    }
    textbox_destroy(item->tb);
    if (item->annot_tb) textbox_destroy(item->annot_tb);
    free(item);
}

void menu_destroy(Menu *menu)
{
    for (int c=0; c<menu->num_columns; c++) {
	MenuColumn *column = menu->columns[c];
	if (column->label_tb) textbox_destroy(column->label_tb);
	for (int s=0; s < column->num_sections; s++) {
	    MenuSection *sctn = column->sections[s];
	    for (int i=0; i<sctn->num_items; i++) {
		MenuItem *item = sctn->items[i];
		/* TODO: get rid of global hovering. */
		menu_item_destroy(item);
	    }
	    free(sctn);
	}
	free(column);
    }
    if (menu->header) {
	txt_area_destroy(menu->header);
    }
    if (menu->layout) {
	layout_destroy(menu->layout);
    }
    free(menu);
}


bool menu_triage_mouse(Menu *menu, SDL_Point *mousep, bool click)
{
    if (menu->selected && !SDL_PointInRect(mousep, &menu->selected->layout->rect)) {
	menu->selected->selected = false;
	/* textbox_set_background_color(hovering->tb, &menu_std_clr_tb_bckgrnd); */
	menu->selected = NULL;
    }
    if (!SDL_PointInRect(mousep, &menu->layout->rect)) {
	if (click) {
	    window_pop_menu(main_win);
	    /* window_pop_mode(main_win); */
	}
	return false;
    }
    for (int c=0; c<menu->num_columns; c++) {

	MenuColumn *col = menu->columns[c];
	if (!SDL_PointInRect(mousep, &col->layout->rect)) {
	    continue;
	}
	for (int s=0; s<col->num_sections; s++) {
	    MenuSection *sctn = col->sections[s];
	    if (!SDL_PointInRect(mousep, &sctn->layout->rect)) {
		continue;
	    }
	    for (int i=0; i<sctn->num_items; i++) {
		MenuItem *item = sctn->items[i];
		if (SDL_PointInRect(mousep, &item->layout->rect)) {
		    /* Layout *lt = layout_deepest_at_point(mmenu->layout, mousep); */
		    menu->selected = item; /* TODO: eliminate */
		    item->selected = true;
		    
		    menu->sel_col = c;
		    col->sel_sctn = s;
		    sctn->sel_item = i;
		    if (click && item->onclick) {
			item->onclick(item->target);
			/* window_pop_menu(main_win); */
		    }
		    
		    /* textbox_set_background_color(item->tb, &menu_std_clr_highlight); */
		    return true;
		}
	    }
	}
    }
    return false;
}


Layout *menu_scroll(Menu *menu, SDL_Point mousep, int x, int y, bool dynamic)
{
    if (abs(x) > abs(y)) {
	if (menu->cols_overflow && SDL_PointInRect(&mousep, &menu->cols_container->rect)) {
	    layout_scroll(menu->cols_lt, -x, 0, dynamic);
	    return menu->cols_lt;
	}
    } else {
	for (int i=0; i<menu->num_columns; i++) {
	    MenuColumn *mc = menu->columns[i];
	    if (mc->overflow && SDL_PointInRect(&mousep, &mc->container->rect)) {
		layout_scroll(mc->layout, 0, y, dynamic);
		return mc->layout;
	    }
	}
    }
    return NULL;
    
}

MenuItem *menu_item_at_index(Menu *menu, int index)
{
    int c, s, i;
    c = s = i = 0;
    MenuColumn *col = menu->columns[c];
    MenuSection *sctn = col->sections[s];
    MenuItem *item = NULL;
    while (index >= sctn->num_items) {
	if (s < col->num_sections - 1) {
	    index -= sctn->num_items;
	    s++;
	    sctn = col->sections[s];
	} else {
	    if (c < menu->num_columns - 1) {
		index -= sctn->num_items;
		s = 0;
		c++;
		col = menu->columns[c];
		sctn = col->sections[s];
	    } else {
		return NULL;
	    }
	}
    }
    item = sctn->items[index];
    return item;
}

static MenuItem *menu_sel_item(Menu *m)
{
    MenuItem *item = NULL;
    if (m->sel_col < m->num_columns) {
	MenuColumn *col = m->columns[m->sel_col];
	if (col->sel_sctn < col->num_sections) {
	    MenuSection *sctn = col->sections[col->sel_sctn];
	    if (sctn->sel_item < sctn->num_items) {
		item = sctn->items[sctn->sel_item];
	    }
	}
    }
    return item;
}

static void menu_rectify_scroll(Menu *m, int direction)
{
    MenuItem *sel = menu_sel_item(m);
    if (!sel) return;
    MenuColumn *col = sel->section->column;
    if (!col->overflow) return;
    int min_y = col->container->rect.y + MENU_STD_SECTION_PAD * m->window->dpi_scale_factor;
    int max_y = col->container->rect.y + col->container->rect.h - MENU_STD_SECTION_PAD * m->window->dpi_scale_factor;
    int undermin = min_y - sel->layout->rect.y;
    int overmax;
    bool reset = false;
    if (undermin > 0) {
	/* fprintf(stderr, "PAST MIN scroll %d\n", col->layout->scroll_offset_v); */
	col->layout->scroll_offset_v += undermin;
	if (col->layout->scroll_offset_v > 0) col->layout->scroll_offset_v = 0;
	reset = true;
    } else if ((overmax = sel->layout->rect.y + sel->layout->rect.h - max_y) > 0) {
	col->layout->scroll_offset_v -= overmax;
	reset = true;
	/* fprintf(stderr, "PAST MAX scroll %d\n", col->layout->scroll_offset_v); */
    }
    if (reset) layout_reset(col->layout);
}

static void menu_rectify_scroll_horizontal(Menu *m, int direction)
{
    if (!m->cols_overflow) return;
    if (m->sel_col > m->num_columns) return;
    MenuColumn *col = m->columns[m->sel_col];
    int min_x = m->cols_container->rect.x + MENU_STD_COLUMN_PAD * m->window->dpi_scale_factor;
    int max_x = m->cols_container->rect.x + m->cols_container->rect.w - MENU_STD_COLUMN_PAD * m->window->dpi_scale_factor;
    int undermin = min_x - col->layout->rect.x;
    int overmax;
    bool reset = false;
    if (undermin > 0) {
	/* fprintf(stderr, "PAST MIN scroll %d\n", m->cols_layout->scroll_offset_v); */
	m->cols_lt->scroll_offset_h += undermin;
	if (m->cols_lt->scroll_offset_h > 0) m->cols_lt->scroll_offset_h = 0;
	reset = true;
    } else if ((overmax = col->layout->rect.x + col->layout->rect.w - max_x) > 0) {
	m->cols_lt->scroll_offset_h -= overmax;
	reset = true;
	/* fprintf(stderr, "PAST MAX scroll %d\n", m->cols_lt->scroll_offset_v); */
    }
    if (reset) layout_reset(m->layout);
}

void menu_next(Menu *m)
{
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
    menu_rectify_scroll(m, 1);

}


void menu_prev(Menu *m)
{
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
    menu_rectify_scroll(m, -1);
}

void menu_right(Menu *m)
{
    if (m->sel_col < m->num_columns - 1) {
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
    menu_rectify_scroll_horizontal(m, 1);

}

void menu_left(Menu *m)
{
    if (m->sel_col == 255) {
	m->sel_col = 0;
    } else if (m->sel_col > 0) {
	m->sel_col--;
    }
    menu_rectify_scroll_horizontal(m, -1);

}

void menu_translate(Menu *menu, int translate_x, int translate_y)
{
    menu->layout->rect.x += translate_x * menu->window->dpi_scale_factor;
    menu->layout->rect.y += translate_y * menu->window->dpi_scale_factor;
    layout_set_values_from_rect(menu->layout);
    menu_reset_layout(menu);
}

void menu_draw(Menu *menu)
{
    int itemh_pixels = 0;
    SDL_Renderer *rend = menu->window->rend;
    
    SDL_SetRenderDrawColor(rend, sdl_color_expand(menu_std_clr_bckgrnd));
    geom_fill_rounded_rect(rend, &menu->layout->rect, MENU_STD_CORNER_RAD); 

    SDL_Rect border = menu->layout->rect;
    SDL_SetRenderDrawColor(rend, sdl_color_expand(menu_std_clr_outer_border));
    geom_draw_rounded_rect(rend, &border, MENU_STD_CORNER_RAD);
    border.x++;
    border.y++;
    border.w -= 2;
    border.h -= 2;
    SDL_SetRenderDrawColor(rend, sdl_color_expand(menu_std_clr_inner_border));
    geom_draw_rounded_rect(rend, &border, MENU_STD_CORNER_RAD - 1);
    SDL_Rect saved_cliprect = {0};
    bool was_clipped = SDL_RenderIsClipEnabled(rend);
    SDL_RenderGetClipRect(rend, &saved_cliprect);
    for (int i=0; i<menu->num_columns; i++) {
	MenuColumn *col = menu->columns[i];
	SDL_RenderSetClipRect(rend, &col->container->rect);
	if (col->label_tb) textbox_draw(col->label_tb);
	for (int j=0; j<col->num_sections; j++) {
	    MenuSection *sctn = col->sections[j];
	    if (j > 0) {
		SDL_SetRenderDrawColor(menu->window->rend, sdl_color_expand(menu_std_clr_sctn_div));
		SDL_Rect *sctnrect = &sctn->layout->rect;
		int y = sctnrect->y - itemh_pixels / 2;
		SDL_RenderDrawLine(menu->window->rend, sctnrect->x, y, sctnrect->x + sctnrect->w, y);
	    }

	    for (int k=0; k<sctn->num_items; k++) {
		MenuItem *item = sctn->items[k];
	        if (k == 0) {   
		    itemh_pixels= item->layout->rect.h;
		}
		/* draw_layout(menu->window, item->annot_tb->layout); */
		/* if (item->selected) { */
		if (i == menu->sel_col && j == col->sel_sctn && k == sctn->sel_item) {
		    SDL_SetRenderDrawColor(menu->window->rend, sdl_color_expand(menu_std_clr_highlight));
		    geom_fill_rounded_rect(menu->window->rend, &item->layout->rect, MENU_STD_CORNER_RAD);
		}
		textbox_draw(item->tb);
		if (item->annot_tb) {
		    textbox_draw(item->annot_tb);
		}
	    }
	    if (col->overflow) {
		const int r = menu_std_clr_bckgrnd.r;
		const int g = menu_std_clr_bckgrnd.g;
		const int b = menu_std_clr_bckgrnd.b;
		int a = 255;
		int x = col->container->rect.x;
		int x2 = col->container->rect.x + col->container->rect.w;
		int y = col->container->rect.y;
		int ybottom = y + col->container->rect.h;
		int num = MENU_STD_SECTION_PAD * menu->window->dpi_scale_factor * 2;
		for (int i=0; i<num; i++) {
		    a = 255 - 255.0 *  (double)i / num;
		    SDL_SetRenderDrawColor(rend, r, g, b, a);
		    if (col->layout->scroll_offset_v != 0) {
			SDL_RenderDrawLine(rend, x, y + i, x2, y + i);
		    }
		    SDL_RenderDrawLine(rend, x, ybottom - i, x2, ybottom - i);
		}
	    }
	}
    }
    if (was_clipped) {
	SDL_RenderSetClipRect(rend, &saved_cliprect);
    } else {
	SDL_RenderSetClipRect(rend, NULL);
     }
    if (menu->cols_overflow) {
	const int r = menu_std_clr_bckgrnd.r;
	const int g = menu_std_clr_bckgrnd.g;
	const int b = menu_std_clr_bckgrnd.b;
	int a = 255;
	int x = menu->cols_container->rect.x;
	/* int x2 = col->container->rect.x + col->container->rect.w; */
	int y1 = menu->cols_container->rect.y;
	int y2 = menu->cols_container->rect.y + menu->cols_container->rect.w;
	int xright = menu->cols_container->rect.x + menu->cols_container->rect.w;
	/* int ybottom = y + col->container->rect.h; */
	int num = MENU_STD_SECTION_PAD * menu->window->dpi_scale_factor * 2;
	for (int i=0; i<num; i++) {
	    a = 255 - 255.0 *  (double)i / num;
	    SDL_SetRenderDrawColor(rend, r, g, b, a);
	    if (menu->cols_container->scroll_offset_v != 0) {
		SDL_RenderDrawLine(rend, x, y1, x + i, y2);
	    }
	    SDL_RenderDrawLine(rend, xright - i, y1, xright - i, y2);
	}
    }

    if (menu->header) {
	txt_area_draw(menu->header);
    }

    if (menu != menu->window->menus[menu->window->num_menus - 1]) {
	
	SDL_SetRenderDrawColor(rend, menu_std_clr_bckgrnd.r, menu_std_clr_bckgrnd.g, menu_std_clr_bckgrnd.b, 200);
	geom_fill_rounded_rect(rend, &menu->layout->rect, MENU_STD_CORNER_RAD);
    }
    
    /* SDL_SetRenderDrawColor(main_win->rend, 255, 0, 0, 200); */
    /* SDL_RenderDrawRect(main_win->rend, &menu->cols_lt->rect); */
    /* SDL_SetRenderDrawColor(main_win->rend, 0, 255, 0, 200); */
    /* SDL_RenderDrawRect(main_win->rend, &menu->cols_container->rect); */
    /* for (int i=0; i<menu->num_columns; i++) { */
    /* 	SDL_SetRenderDrawColor(main_win->rend, 0, 255, 0, 255); */
    /* 	/\* SDL_RenderDrawRect(main_win->rend, &menu->layout->children[0]->rect); *\/ */
    /* 	SDL_RenderDrawRect(main_win->rend, &menu->columns[i]->container->rect); */
    /* } */
    /* SDL_SetRenderDrawColor(main_win->rend, 255, 0, 0, 255); */
    /* SDL_RenderDrawRect(main_win->rend, &menu->cols_container->rect); */
	
    /* layout_draw(main_win, menu->layout); */
}


