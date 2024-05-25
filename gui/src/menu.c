#include "color.h"
#include "geometry.h"
#include "draw.h"
#include "menu.h"
#include "text.h"
#include "textbox.h"

#define MENU_TXT_SIZE 12
#define MENU_STD_ITEM_PAD_H 4
#define MENU_STD_ITEM_PAD_V 2
#define MENU_STD_ITEM_V_SPACING 2
#define MENU_STD_SECTION_PAD 16
#define MENU_STD_COLUMN_PAD 8
#define MENU_STD_HEADER_PAD 20

#define MENU_STD_CORNER_RAD 10

extern SDL_Color color_global_clear;
extern Window *main_win;

SDL_Color menu_std_clr_inner_border = (SDL_Color) {200, 200, 200, 200};
SDL_Color menu_std_clr_outer_border = (SDL_Color) {10, 10, 10, 220};
SDL_Color menu_std_clr_bckgrnd = (SDL_Color) {40, 40, 40, 245};
SDL_Color menu_std_clr_txt = (SDL_Color) {255, 255, 255, 255};
SDL_Color menu_std_clr_annot_txt = (SDL_Color) {200, 200, 200, 250};
SDL_Color menu_std_clr_highlight = (SDL_Color) {50, 50, 255, 250};
SDL_Color menu_std_clr_sctn_div = (SDL_Color) {200, 200, 200, 250};

SDL_Color color_clear = (SDL_Color) {0, 0, 0, 0};

int menu_std_border_rad = 24;

	
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
static void menu_column_rectify_sections(MenuColumn *col)
{
    int y_logical = MENU_STD_SECTION_PAD;
    for (int i=0; i<col->num_sections; i++) {
	MenuSection *sctn = col->sections[i];
	sctn->layout->y.value.intval = y_logical;
	y_logical += sctn->layout->h.value.intval + MENU_STD_SECTION_PAD;
    }
}

/* Sets col x and menu dims. Assumes columns width has already been set */
static void menu_rectify_columns(Menu *menu)
{
    int h_max = 0;
    int h = 0;
    int w_cumulative = MENU_STD_COLUMN_PAD;
    for (int i=0; i<menu->num_columns; i++) {
	MenuColumn *column = menu->columns[i];
	column->layout->x.value.intval = w_cumulative;
	w_cumulative += column->layout->w.value.intval + 4 * MENU_STD_COLUMN_PAD; 
	if ((h = column->layout->h.value.intval) > h_max) {
	    h_max = h;
	}
    }
    menu->layout->w.value.intval = w_cumulative;
    if (menu->header) {
	h_max += menu->header->layout->h.value.intval + MENU_STD_HEADER_PAD;
    }
    menu->layout->h.value.intval = h_max;
    layout_reset(menu->layout);
}
	
    

Menu *menu_create(Layout *layout, Window *window)
{
    Menu *menu = malloc(sizeof(Menu));
    menu->num_columns = 0;
    menu->layout = layout;
    menu->selected = NULL;
    menu->sel_col = 255;
    layout_add_child(layout);
    layout_add_complementary_child(layout, H);
    
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
    lt->x.value.intval = x_pix / main_win->dpi_scale_factor;
    lt->y.value.intval = y_pix / main_win->dpi_scale_factor;
    /* lt->w.value.intval = 1200; */
    /* lt->h.value.intval = 900; */
    layout_reset(lt);
    layout_write(stdout, lt, 0);
    Menu *menu = menu_create(lt, main_win);
    return menu;

}


void menu_reset_layout(Menu *menu)
{
    layout_force_reset(menu->layout);
    menu_reset_textboxes(menu);
}

void menu_add_header(Menu *menu, const char *title, const char *description)
{
    menu->title = title;
    menu->description = description;
    Layout *header_lt = menu->layout->children[0];
    header_lt->x.value.intval = MENU_STD_COLUMN_PAD + MENU_STD_ITEM_PAD_H * 2;
    header_lt->y.value.intval = MENU_STD_SECTION_PAD + MENU_STD_ITEM_PAD_V;
    header_lt->w.type = REL;
    header_lt->w.value.intval = menu->layout->w.value.intval - 2 * header_lt->x.value.intval;
    layout_reset(header_lt);
    TextArea *header = txt_area_create(description, header_lt, menu->font, MENU_TXT_SIZE, menu_std_clr_annot_txt, menu->window);
    menu->header = header;
    int header_h = header->layout->h.value.intval;

    /* Set y value of content layout */
    menu->layout->children[1]->y.value.intval = header_h + MENU_STD_HEADER_PAD;
    menu->layout->h.value.intval += menu->layout->children[1]->y.value.intval;
    
    menu_reset_layout(menu);
}

MenuColumn *menu_column_add(Menu *menu, const char *label)
{
    Layout *content_lt = menu->layout->children[1];
    MenuColumn *col = malloc(sizeof(MenuColumn));
    col->num_sections = 0;
    col->sel_sctn = 255;
    col->label = label;
    col->menu = menu;
    col->layout = layout_add_child(content_lt);
    char lt_name[7];
    snprintf(lt_name, 7, "col_%02d", menu->num_columns);
    lt_name[6] = '\0';
    layout_set_name(col->layout, lt_name);
    col->layout->y.value.intval = 0;
    col->layout->h.value.intval = MENU_STD_SECTION_PAD;
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
    sctn->layout->w.type = SCALE;
    sctn->layout->w.value.floatval = 1.0;
    char sctn_name[8];
    snprintf(sctn_name, 8, "sctn_%02d", col->num_sections);
    sctn_name[7] = '\0';
    layout_set_name(sctn->layout, sctn_name);
    col->sections[col->num_sections] = sctn;
    col->num_sections++;
    col->layout->h.value.intval += MENU_STD_SECTION_PAD;
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
    /* fprintf(stdout, "ADD item label: \"%s\", annot: \"%s\"\n", label, annotation); */
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
    /* tb_lt->h.value.floatval = 1.0; */
    /* annot_lt->h.type = SCALE; */
    /* annot_lt->h.value.floatval = 1.0; */

    Window *win = sctn->column->menu->window;
    /* TTF_Font *font = sctn->column->menu->font; */
    Font *font = sctn->column->menu->font;



    /* Create and initialize main tb */
    item->tb = textbox_create_from_str(
	label,
	tb_lt,
	font,
	MENU_TXT_SIZE,
	win
	);
    item->tb->text->align = CENTER_LEFT;
    textbox_set_trunc(item->tb, false);
    textbox_set_text_color(item->tb, &menu_std_clr_txt);
    textbox_set_background_color(item->tb, &color_global_clear);
    textbox_set_border_color(item->tb, &color_global_clear);
    

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
	textbox_set_background_color(item->annot_tb, &color_global_clear);
	textbox_set_border_color(item->annot_tb, &color_global_clear);
    }


    textbox_size_to_fit(item->tb, MENU_STD_ITEM_PAD_H, MENU_STD_ITEM_PAD_V);
    
    if (annotation) {
	textbox_size_to_fit(item->annot_tb, MENU_STD_ITEM_PAD_H, MENU_STD_ITEM_PAD_V);
	item->annot_tb->layout->w.type = COMPLEMENT;
	item->annot_tb->layout->x.value.intval = item->tb->layout->w.value.intval;
    }

    /* layout_fprint(stdout, item->tb->layout); */
    /* layout_fprint(stdout, item->annot_tb->layout); */
    /* exit(0); */
    
    int annot_pad = annotation ? item->annot_tb->layout->w.value.intval + 5 * MENU_STD_ITEM_PAD_H : 0;
    int w_logical = item->tb->layout->w.value.intval + annot_pad + 2 * MENU_STD_ITEM_PAD_H;
    int h_logical = item->tb->layout->h.value.intval + MENU_STD_ITEM_PAD_V;    

    item->layout->x.value.intval = 0;
    item->layout->y.value.intval = (h_logical + MENU_STD_ITEM_V_SPACING) * (sctn->num_items - 1);
    item->layout->h.value.intval = h_logical;
    item->layout->w.value.floatval = 1.0;
    item->layout->w.type = SCALE;
    MenuColumn *col = sctn->column;
    sctn->layout->w.value.floatval = 1.0;
    sctn->layout->w.type = SCALE;
    sctn->layout->h.value.intval += h_logical + MENU_STD_ITEM_V_SPACING;
    
    if (w_logical > col->layout->w.value.intval) {
	/* fprintf(stdout, "Setting col w to %d\n", w_logical); */
	col->layout->w.value.intval = w_logical;
    }
    col->layout->h.value.intval += h_logical + MENU_STD_ITEM_V_SPACING;
    menu_column_rectify_sections(col);
    /* fprintf(stderr, "Height before: %d\n", col->menu->layout->h.value.intval); */
    menu_rectify_columns(col->menu);

    /* fprintf(stderr, "Height after: %d\n", col->menu->layout->h.value.intval); */
    txt_reset_display_value(item->tb->text);
    if (annotation) {
	txt_reset_display_value(item->annot_tb->text);
    }

    menu_reset_layout(col->menu);
    return item;
}

void menu_item_destroy(MenuItem *item)
{
    if (item->target && item->free_target_on_destroy) {
	free(item->target);
    }
    textbox_destroy(item->tb);
    free(item);
}

void menu_destroy(Menu *menu)
{
    /* fprintf(stdout, "\t->menu destroy\n"); */
    /* if (menu->title) { */
    /* 	free(menu->title); */
    /* } */
    /* fprintf(stdout, "\t->successfully destroyed the title\n"); */
    /* if (menu->description) { */
    /* 	free(menu->description); */
    /* } */
    /* fprintf(stdout, "\t->successfully destroyed the desc\n"); */
    /* fprintf(stdout, "\t->prelim destroy\n"); */
    for (int c=0; c<menu->num_columns; c++) {
	MenuColumn *column = menu->columns[c];
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


void triage_mouse_menu(Menu *menu, SDL_Point *mousep, bool click)
{
    if (menu->selected && !SDL_PointInRect(mousep, &menu->selected->layout->rect)) {
	menu->selected->selected = false;
	/* textbox_set_background_color(hovering->tb, &menu_std_clr_tb_bckgrnd); */
	menu->selected = NULL;
    }
    if (!SDL_PointInRect(mousep, &menu->layout->rect)) {
	if (click) {
	    window_pop_menu(main_win);
	    window_pop_mode(main_win);
	}
	return;
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
		    /* fprintf(stdout, "Deepest at point: %s\n", lt->name); */
		    menu->selected = item; /* TODO: eliminate */
		    item->selected = true;
		    
		    menu->sel_col = c;
		    col->sel_sctn = s;
		    sctn->sel_item = i;
		    if (click && item->onclick) {
			item->onclick(item->target);
		    }
		    
		    /* textbox_set_background_color(item->tb, &menu_std_clr_highlight); */
		    return;
		}
	    }
	}
    }
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
    geom_fill_rounded_rect(rend, &menu->layout->rect, menu_std_border_rad); 

    SDL_Rect border = menu->layout->rect;
    SDL_SetRenderDrawColor(rend, sdl_color_expand(menu_std_clr_outer_border));
    geom_draw_rounded_rect(rend, &border, menu_std_border_rad);
    border.x++;
    border.y++;
    border.w -= 2;
    border.h -= 2;
    SDL_SetRenderDrawColor(rend, sdl_color_expand(menu_std_clr_inner_border));
    geom_draw_rounded_rect(rend, &border, menu_std_border_rad - 1);
    for (int i=0; i<menu->num_columns; i++) {
	MenuColumn *col = menu->columns[i];
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
	}
    }

    if (menu->header) {
	txt_area_draw(menu->header);
    }
}


