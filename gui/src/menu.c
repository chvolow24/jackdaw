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

#define MENU_STD_CORNER_RAD 10

SDL_Color CLR_CLR = (SDL_Color) {0, 0, 0, 0};

SDL_Color menu_std_clr_inner_border = (SDL_Color) {130, 130, 130, 250};
SDL_Color menu_std_clr_outer_border = (SDL_Color) {10, 10, 10, 220};
SDL_Color menu_std_clr_bckgrnd = (SDL_Color) {40, 40, 40, 238};

SDL_Color menu_std_clr_txt = (SDL_Color) {255, 255, 255, 255};
SDL_Color menu_std_clr_annot_txt = (SDL_Color) {200, 200, 200, 250};
SDL_Color menu_std_clr_highlight = (SDL_Color) {50, 50, 255, 250};
/* SDL_Color menu_std_clr_tb_bckgrnd0 = (SDL_Color) {0, 0, 0, 0}; */

SDL_Color menu_std_clr_sctn_div = (SDL_Color) {200, 200, 200, 250};

SDL_Color color_clear = (SDL_Color) {0, 0, 0, 0};

int menu_std_border_rad = 24;


/* JDAW_Color menulist_bckgrnd = (JDAW_Color) {,{40, 40, 40, 248}}; */
/* JDAW_Color menulist_inner_brdr = (JDAW_Color) {,}; */
/* JDAW_Color menulist_outer_brdr = {,{10, 10, 10, 220}}; */


	
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
    menu->layout->h.value.intval = h_max;
    layout_reset(menu->layout);
}
	
    

Menu *menu_create(Layout *layout, Window *window)
{
    Menu *menu = malloc(sizeof(Menu));
    menu->num_columns = 0;
    menu->layout = layout;
    menu->window = window;
    menu->title = NULL;
    menu->description = NULL;
    return menu;
}

MenuColumn *menu_column_add(Menu *menu, char *label)
{
    MenuColumn *col = malloc(sizeof(MenuColumn));
    col->num_sections = 0;
    col->label = label;
    col->menu = menu;
    col->layout = layout_add_child(menu->layout);
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

MenuSection *menu_section_add(MenuColumn *col, char *label)
{
    MenuSection *sctn = malloc(sizeof(MenuSection));
    sctn->num_items = 0;
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
    char *label,
    char *annotation,
    void (*onclick)(Textbox *tb, void *target),
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
    item->section = sctn;
    item->hovering = false;
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
    TTF_Font *font = ttf_get_font_at_size(win->std_font, MENU_TXT_SIZE);



    /* Create and initialize main tb */
    item->tb = textbox_create_from_str(
	label,
	tb_lt,
	font,
	win
	);
    item->tb->text->align = CENTER_LEFT;
    textbox_set_trunc(item->tb, false);
    textbox_set_text_color(item->tb, &menu_std_clr_txt);
    textbox_set_background_color(item->tb, &CLR_CLR);
    textbox_set_border_color(item->tb, &CLR_CLR);
    

    /* Create and initialize annotation tb */
    if (annotation) {
	
	item->annot_tb = textbox_create_from_str(
	    annotation,
	    annot_lt,
	    font,
	    win
	    );
	item->annot_tb->text->align = CENTER_RIGHT;
	textbox_set_trunc(item->annot_tb, false);
	textbox_set_text_color(item->annot_tb, &menu_std_clr_annot_txt);
	textbox_set_background_color(item->annot_tb, &CLR_CLR);
	textbox_set_border_color(item->annot_tb, &CLR_CLR);
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

    item->layout->y.value.intval = (h_logical + MENU_STD_ITEM_V_SPACING) * (sctn->num_items - 1);
    item->layout->h.value.intval = h_logical;
    item->layout->w.value.floatval = 1.0;
    item->layout->w.type = SCALE;
    MenuColumn *col = sctn->column;
    sctn->layout->w.value.floatval = 1.0;
    sctn->layout->w.type = SCALE;
    sctn->layout->h.value.intval += h_logical + MENU_STD_ITEM_V_SPACING;
    
    if (w_logical > col->layout->w.value.intval) {
	col->layout->w.value.intval = w_logical;
    }
    col->layout->h.value.intval += h_logical + MENU_STD_ITEM_V_SPACING;
    menu_column_rectify_sections(col);
    menu_rectify_columns(col->menu);

    txt_reset_display_value(item->tb->text);
    if (annotation) {
	txt_reset_display_value(item->annot_tb->text);
    }

    menu_reset_textboxes(col->menu);

    fprintf(stderr, "End create item\n");
    return item;
}

void menu_item_destroy(MenuItem *item)
{
    textbox_destroy(item->tb);
    free(item);
}

void menu_destroy(Menu *menu)
{
    if (menu->title) {
	free(menu->title);
    }
    if (menu->description) {
	free(menu->description);
    }
    for (int c=0; c<menu->num_columns; c++) {
	MenuColumn *column = menu->columns[c];
	for (int s=0; s < column->num_sections; s++) {
	    MenuSection *sctn = column->sections[s];
	    for (int i=0; i<sctn->num_items; i++) {
		MenuItem *item = sctn->items[i];
		menu_item_destroy(item);
	    }
	    free(sctn);
	}
	free(column);
    }
    free(menu);
}


MenuItem *hovering = NULL;
void triage_mouse_menu(Menu *menu, SDL_Point *mousep, bool click)
{
    if (hovering && !SDL_PointInRect(mousep, &hovering->layout->rect)) {
	hovering->hovering = false;
	/* textbox_set_background_color(hovering->tb, &menu_std_clr_tb_bckgrnd); */
	hovering = NULL;
    }
    if (!SDL_PointInRect(mousep, &menu->layout->rect)) {
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
		    hovering = item;
		    item->hovering = true;
		    if (click && item->onclick) {
			item->onclick(item->tb, item->target);
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
    layout_reset(menu->layout);
    menu_reset_textboxes(menu);
}

void menu_draw(Menu *menu)
{
    int itemh_pixels = 0;
    SDL_Renderer *rend = menu->window->rend;

    SDL_Rect border = menu->layout->rect;
    SDL_SetRenderDrawColor(rend, sdl_color_expand(menu_std_clr_outer_border));
    geom_draw_rounded_rect(rend, &border, menu_std_border_rad);
    border.x++;
    border.y++;
    border.w -= 2;
    border.h -= 2;
    SDL_SetRenderDrawColor(rend, sdl_color_expand(menu_std_clr_inner_border));
    geom_draw_rounded_rect(rend, &border, menu_std_border_rad - 1);
    
    SDL_SetRenderDrawColor(rend, sdl_color_expand(menu_std_clr_bckgrnd));
    geom_fill_rounded_rect(rend, &menu->layout->rect, menu_std_border_rad); 
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
		if (item->hovering) {
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
}


