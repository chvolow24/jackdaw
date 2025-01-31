#include "animation.h"
#include "geometry.h"
#include "text.h"
#include "textbox.h"
#include "layout.h"

#define TXTBX_DEFAULT_RAD 0
#define TXTBX_DEFAULT_MAXLEN 255

SDL_Color textbox_default_bckgrnd_clr = (SDL_Color) {240, 240, 240, 255};
SDL_Color textbox_default_txt_clr = (SDL_Color) {0, 0, 0, 255};
SDL_Color textbox_default_border_clr = (SDL_Color) {100, 100, 100, 255};
extern SDL_Color color_global_clear;
extern SDL_Color color_global_black;
extern SDL_Color color_global_green;
extern SDL_Color color_global_grey;
#ifndef LAYOUT_BUILD
extern volatile bool CANCEL_THREADS;
#endif
int textbox_default_radius = 0;

/*txt_create_from_str(char *set_str, int max_len, SDL_Rect *container, TTF_Font *font, SDL_Color txt_clr, TextAlign align, bool truncate, SDL_Renderer *rend) -> Text **/
Textbox *textbox_create_from_str(
    const char *set_str,
    Layout *lt,
    Font *font,
    uint8_t text_size,
    /* TTF_Font *font, */
    Window *win
    )
{
    Textbox *tb = calloc(1, sizeof(Textbox));
    tb->layout = lt;
    tb->bckgrnd_clr = &textbox_default_bckgrnd_clr;
    tb->border_clr = &textbox_default_border_clr;
    tb->border_thickness = 0;
    tb->corner_radius = textbox_default_radius * win->dpi_scale_factor;
    tb->window = win;
    tb->text = txt_create_from_str(
	(char *)set_str,
	TXTBX_DEFAULT_MAXLEN,
	lt,
	font,
	text_size,
	textbox_default_txt_clr,
	CENTER,
	true,
	win
	);
    /* textbox_reset_full(tb); */
	
    return tb;
}

void textbox_size_to_fit(Textbox *tb, int h_pad, int v_pad)
{
    h_pad *= tb->window->dpi_scale_factor;
    v_pad *= tb->window->dpi_scale_factor;
    SDL_Rect *text_rect = &tb->text->text_lt->rect;
    SDL_Rect *layout_rect = &tb->layout->rect;
    bool save_trunc = tb->text->truncate;
    
    tb->text->truncate = false;
    txt_set_pad(tb->text, h_pad, v_pad);
    txt_reset_display_value(tb->text);
    layout_rect->w = text_rect->w + h_pad * 2;
    layout_rect->h = text_rect->h + v_pad * 2;
    
    text_rect->x = layout_rect->x + h_pad;
    /* fprintf(stderr, "Textbox rect x: %d, layout rect x: %d\n", text_rect->x, layout_rect->x); */
    text_rect->y = layout_rect->y + v_pad;
    tb->text->truncate = save_trunc;
    
    layout_set_wh_from_rect(tb->layout);
    layout_reset(tb->layout);
}

void textbox_size_to_fit_v(Textbox *tb, int v_pad)
{
    v_pad *= tb->window->dpi_scale_factor;
    SDL_Rect *text_rect = &tb->text->text_lt->rect;
    SDL_Rect *layout_rect = &tb->layout->rect;
    bool save_trunc = tb->text->truncate;
    
    tb->text->truncate = false;
    txt_set_pad(tb->text, 0, v_pad);
    txt_reset_display_value(tb->text);
    /* layout_rect->w = text_rect->w; */
    layout_rect->h = text_rect->h + v_pad * 2;
    /* fprintf(stderr, "Textbox rect x: %d, layout rect x: %d\n", text_rect->x, layout_rect->x); */
    text_rect->y = layout_rect->y + v_pad;
    tb->text->truncate = save_trunc;
    layout_set_wh_from_rect(tb->layout);
    layout_reset(tb->layout);

}

void textbox_size_to_fit_h(Textbox *tb, int h_pad)
{
    h_pad *= tb->window->dpi_scale_factor;
    SDL_Rect *text_rect = &tb->text->text_lt->rect;
    SDL_Rect *layout_rect = &tb->layout->rect;
    bool save_trunc = tb->text->truncate;
    
    tb->text->truncate = false;
    txt_set_pad(tb->text, 0, h_pad);
    txt_reset_display_value(tb->text);
    /* layout_rect->h = text_rect->h; */
    layout_rect->w = text_rect->w + h_pad * 2;
    /* fprintf(stderr, "Textbox rect x: %d, layout rect x: %d\n", text_rect->x, layout_rect->x); */
    text_rect->y = layout_rect->y + h_pad;
    tb->text->truncate = save_trunc;
    layout_set_wh_from_rect(tb->layout);
    layout_reset(tb->layout);

}
/* Pad a textbox on all sides. Modifies the dimensions of the layout */
void textbox_pad(Textbox *tb, int pad)
{
    SDL_Rect *lt_rect = &tb->layout->rect;
    SDL_Rect *txt_rect = &tb->text->text_lt->rect;
    lt_rect->w = txt_rect->w + (2 * pad);
    lt_rect->h = txt_rect->h + (2 * pad);
    layout_set_values_from_rect(tb->layout);
}

void textbox_set_fixed_w(Textbox *tb, int fixed_w)
{
    fixed_w *= tb->window->dpi_scale_factor;
    tb->layout->rect.w = fixed_w;
    layout_set_values_from_rect(tb->layout);
    txt_reset_display_value(tb->text);
}

void textbox_destroy(Textbox *tb)
{
    if (tb->text) {
	txt_destroy(tb->text);
    }
    if (tb->layout) {
	layout_destroy(tb->layout);
    }
    free(tb);
}

void textbox_draw(Textbox *tb)
{
    int rad = tb->corner_radius * tb->window->dpi_scale_factor;
    /* TODO: consider whether this is bad and if rend needs to be on textbox as well */
    SDL_Renderer *rend = tb->text->win->rend;
    SDL_Color *bckgrnd = tb->bckgrnd_clr;
    SDL_Color *txtclr = &tb->text->color;
    SDL_Color *brdrclr = tb->border_clr;
    SDL_Rect tb_rect = tb->layout->rect;

    
    if (bckgrnd)
	SDL_SetRenderDrawColor(rend, bckgrnd->r, bckgrnd->g, bckgrnd->b, bckgrnd->a);
    if (rad > 0) {
	int halfdim = tb_rect.w < tb_rect.h ? tb_rect.w >> 1 : tb_rect.h >>1;
	rad = halfdim < rad ? halfdim : rad;
	if (bckgrnd)
	    geom_fill_rounded_rect(rend, &tb_rect, rad);
	SDL_SetRenderDrawColor(rend, brdrclr->r, brdrclr->g, brdrclr->b, brdrclr->a);
	for (int i=0; i<tb->border_thickness; i++) {	    
	    geom_draw_rounded_rect(rend, &tb_rect, rad);
	    tb_rect.x += 1;
	    tb_rect.y += 1;
	    tb_rect.w -= 2;
	    tb_rect.h -= 2;
	    rad -= 1;
	}
	    
    } else {
	if (bckgrnd)
	    SDL_RenderFillRect(rend, &tb_rect);
	if (brdrclr) {
	    SDL_SetRenderDrawColor(rend, brdrclr->r, brdrclr->g, brdrclr->b, brdrclr->a);
	    for (int i=0; i<tb->border_thickness; i++) {
		SDL_RenderDrawRect(rend, &tb_rect);
		tb_rect.x += 1;
		tb_rect.y += 1;
		tb_rect.w -= 2;
		tb_rect.h -= 2;
	    }
	}
    }


    /* Styled chunky buttons -- save for later, maybe */

    /* if (tb->style == BUTTON_CLASSIC || tb->style == BUTTON_DARK) { */
    /* 	int dpi_scale = tb->window->dpi_scale_factor; */
    /* 	tb_rect = tb->layout->rect; */
    /* 	if (tb->style == BUTTON_CLASSIC) */
    /* 	    SDL_SetRenderDrawColor(rend, 240, 240, 240, 255); */
    /* 	else */
    /* 	    SDL_SetRenderDrawColor(rend, 120, 120, 120, 255); */
    /* 	SDL_Rect draw_rect = {tb_rect.x, tb_rect.y, dpi_scale, tb_rect.h - dpi_scale}; */
    /* 	SDL_RenderFillRect(rend, &draw_rect); */
    /* 	draw_rect.w = tb_rect.w - dpi_scale; */
    /* 	draw_rect.h = dpi_scale; */
    /* 	SDL_RenderFillRect(rend, &draw_rect); */
    /* 	SDL_SetRenderDrawColor(rend, 0, 0, 0, 255); */
    /* 	/\* draw_rect.x += 2; *\/ */
    /* 	draw_rect.y += tb_rect.h - dpi_scale; */
    /* 	/\* draw_rect.w -= dpi_scale; *\/ */
    /* 	SDL_RenderFillRect(rend, &draw_rect); */
    /* 	draw_rect.x += tb_rect.w - dpi_scale; */
    /* 	draw_rect.y = tb_rect.y; */
    /* 	draw_rect.h = tb_rect.h; */
    /* 	draw_rect.w = dpi_scale; */
    /* 	SDL_RenderFillRect(rend, &draw_rect); */
    /* } */


    
    SDL_SetRenderDrawColor(rend, txtclr->r, txtclr->g, txtclr->b, txtclr->a);
    txt_draw(tb->text);
}

static int scheduled_color_change(void *data)
{
    Textbox *tb = (Textbox *)data;
    for (int i=0; i<tb->color_change_timer; i++) {
	#ifndef LAYOUT_BUILD
	if (CANCEL_THREADS) return 0;
	#endif
	SDL_Delay(1);
	
    }
    /* SDL_Delay(tb->color_change_timer); */
    if (tb->color_change_target_text) {
	textbox_set_text_color(tb, tb->color_change_new_color);
    } else {
	textbox_set_background_color(tb, tb->color_change_new_color);
    }
    if (tb->color_change_callback)
	tb->color_change_callback((void *)tb, tb->color_change_callback_target);
    return 0;
}

void textbox_schedule_color_change(
    Textbox *tb,
    int timer,
    SDL_Color *new_color,
    bool change_text_color,
    ComponentFn color_change_callback,
    void *color_change_callback_target)
{
    /* tb->color_change_timer = timer; */
    /* tb->color_change_target_text = change_text_color; */
    /* tb->color_change_new_color = new_color; */
    /* tb->color_change_callback = color_change_callback; */
    /* tb->color_change_callback_target = color_change_callback_target; */

    /* SDL_CreateThread(scheduled_color_change, "scheduled_tb_color_change", tb); */
}


void textbox_set_trunc(Textbox *tb, bool trunc)
{
    tb->text->truncate = trunc;
    txt_reset_display_value(tb->text);
}

void textbox_set_text_color(Textbox *tb, SDL_Color *clr)
{
    /* txt_set_color also resets the text drawables */
    txt_set_color(tb->text, clr);
}

void textbox_set_background_color(Textbox *tb, SDL_Color *clr)
{
    tb->bckgrnd_clr = clr;
}

void textbox_set_border_color(Textbox *tb, SDL_Color *clr)
{
    tb->border_clr = clr;
}

void textbox_set_border(Textbox *tb, SDL_Color *color, int thickness)
{
    if (color) {
	tb->border_clr = color;
    }
    tb->border_thickness = thickness;
}

void textbox_set_align(Textbox *tb, TextAlign align)
{
    tb->text->align = align;
    /* textbox_reset_full(tb); */
}


void textbox_reset_full(Textbox *tb)
{
    txt_reset_display_value(tb->text);
}


void textbox_reset(Textbox *tb)
{
    /* txt_reset_display_value(tb->text); */
    txt_reset_drawable(tb->text);
}

void textbox_set_pad(Textbox *tb, int h_pad, int v_pad)
{
    txt_set_pad(tb->text, h_pad, v_pad);
    /* textbox_reset_full(tb); */
}

void textbox_set_value_handle(Textbox *tb, const char *new_value)
{
    txt_set_value_handle(tb->text, (char *) new_value);
}


/* Chunky buttons -- save for later, maybe */
void textbox_set_style(Textbox *tb, enum textbox_style style)
{
    /* tb->border_thickness = 0; */
    /* tb->border_clr = NULL; */
    /* tb->corner_radius = 0; */

    /* switch (style) { */
    /* case BUTTON_CLASSIC: */
    /* 	tb->style = BUTTON_CLASSIC; */
    /* 	break; */
    /* case BUTTON_DARK: */
    /* 	tb->style = BUTTON_DARK; */
    /* default: */
    /* 	break; */
    /* } */
}

/* void textbox_set_style(Textbox *tb, enum textbox_style style) */
/* { */
/*     switch (style) { */
/*     case BLANK: */
/* 	textbox_set_background_color(tb, &color_global_clear); */
/* 	textbox_set_border(tb, &color_global_clear, 0); */
/* 	break; */
/*     case NUMBOX: */
/* 	textbox_set_background_color(tb, &color_global_black); */
/* 	textbox_set_text_color(tb, &color_global_green); */
/* 	textbox_set_border(tb, &color_global_grey, 2); */
/* 	tb->text->font = tb->text->win->mono_bold_font; */
/* 	break; */
/*     } */

/* } */

/* #include "dir.h" */

TextLines *textlines_create(
    void **items,
    uint16_t num_items,
    int (*filter)(void *item, void *x_arg),
    TLinesItem *(*create_item)(void ***curent_item, Layout *container, void *x_arg, int (*filter)(void *item, void *x_arg)),
    Layout *container, void *x_arg)
{
    uint16_t num_items_after_filter = 0;
    for (uint16_t i=0; i<num_items; i++) {
	if (filter(items[i], x_arg)) num_items_after_filter++;
    }

    TextLines *tlines = calloc(1, sizeof(TextLines));
    tlines->items = calloc(num_items_after_filter, sizeof(TLinesItem *));
    tlines->num_items = num_items_after_filter;
    tlines->container = container;

    uint16_t line_i = 0;
    for (uint16_t i=0; i<num_items; i++) {
	TLinesItem *item = create_item(&items, container, x_arg, filter);
	if (item) {
	    tlines->items[line_i] = item;
	    /* fprintf(stdout, "Item %d = %s\n", i, ((DirPath *)tlines->items[line_i]->obj)->path); */
	    line_i++;
	    /* i++; */
	}
    }
    layout_force_reset(tlines->container);
    layout_size_to_fit_children(tlines->container, true, 0);
    return tlines;
}

static void textlines_el_destroy(TLinesItem *el)
{
    if (el->tb) {
	textbox_destroy(el->tb);
    }
    free(el);
}

void textlines_destroy(TextLines *tlines)
{
    for (uint16_t i=0; i<tlines->num_items; i++) {
	TLinesItem *item = NULL;
	if ((item = tlines->items[i])) {
	    textlines_el_destroy(item);
	}
    }
    free(tlines->items);
    free(tlines);
}

void textlines_draw(TextLines *tlines)
{
    for (uint16_t i=0; i<tlines->num_items; i++) {
	textbox_draw(tlines->items[i]->tb);
    }
    layout_destroy(tlines->container);
}
