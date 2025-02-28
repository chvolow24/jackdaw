/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

/*****************************************************************************************************************
    autocompletion.c

    * functions related to autocompletion display & navigation
 *****************************************************************************************************************/

#include "autocompletion.h"
#include "geometry.h"

#define AUTOCOMPLETE_LINE_SPACING 3

extern Window *main_win;
extern SDL_Color color_global_white;
extern SDL_Color color_global_clear;


/* TLinesItem *create_autocomplete_tline(void ***current_item, Layout *container, void *xarg, int (*filter)(void *item, void *xarg)) */
/* { */
   
/*     struct autocompletion_item **a_loc = (struct autocompletion_item **)current_item; */
/*     struct autocompletion_item *arr = *a_loc; */
/*     /\* fprintf(stderr, "(%s %p)\n", items->str, items->obj); *\/ */
/*     struct autocompletion_item item = arr[0]; */
    
/*     TLinesItem *line = calloc(1, sizeof(TLinesItem)); */
/*     line->obj = item.obj; */

/*     Layout *lt = layout_add_child(container); */
/*     lt->y.type = STACK; */
/*     lt->y.value = AUTOCOMPLETE_LINE_SPACING; */
/*     lt->h.value = 70; */
/*     lt->w.value = 500; */
    
/*     line->tb = textbox_create_from_str(item.str, lt, main_win->mono_bold_font, 16, main_win); */
/*     textbox_set_align(line->tb, CENTER_LEFT); */
/*     textbox_set_pad(line->tb, 4, 1); */
/*     textbox_size_to_fit(line->tb, 4, 1); */
/*     line->tb->layout->w.value = 1.0; */
/*     line->tb->layout->w.type = SCALE; */

/*     (*a_loc)++; */
/*     textbox_reset(line->tb); */
    
/*     return line; */
/* } */

static SDL_Color highlighted_line = {100, 60, 10, 255};

extern SDL_Color color_global_red;
static void create_autocomplete_tline(
    TextLines *tlines,
    TLinesItem *current_line,
    const void *src_item,
    void *x_arg)
{
    struct autocompletion_item *item = (struct autocompletion_item *)src_item;

    AutoCompletion *ac = (AutoCompletion *)x_arg;
    
    current_line->obj = item->obj;

    Layout *lt = layout_add_child(tlines->container);
    lt->y.type = STACK;
    lt->y.value = AUTOCOMPLETE_LINE_SPACING;
    lt->h.value = 70;
    lt->w.value = 500;

    ac->selection = -1;
    current_line->tb = textbox_create_from_str(item->str, lt, main_win->mono_bold_font, 16, main_win);
    
    if (current_line - tlines->items == ac->selection) {
        textbox_set_background_color(current_line->tb, &highlighted_line);
    } else {
        textbox_set_background_color(current_line->tb, &color_global_clear);
    }
    
    textbox_set_text_color(current_line->tb, &color_global_white);
    textbox_set_align(current_line->tb, CENTER_LEFT);
    textbox_set_pad(current_line->tb, 4, 1);
    textbox_size_to_fit(current_line->tb, 4, 1);
    current_line->tb->layout->w.value = 1.0;
    current_line->tb->layout->w.type = SCALE;
    layout_reset(lt);
    textbox_reset(current_line->tb);
    


    
    /* TODO: move tline creation out of autocompletion and into
       calling functions. Need caller-specific logic for addtl tbs */
    Layout *right_item = layout_add_child(lt);
    right_item->w.type = SCALE;
    right_item->w.value = 1.0;
    right_item->h.type = SCALE;
    right_item->h.value = 1.0;
    
    layout_reset(right_item);

    UserFn *fn = (UserFn *)item->obj;
    Textbox *keycmd = textline_add_addtl_tb(
	current_line,
	fn->annotation,
	right_item,
	main_win->mono_font,
	16,
	main_win);
    textbox_set_align(keycmd, CENTER_RIGHT);
    textbox_set_pad(keycmd, 10, 1);
    textbox_set_text_color(keycmd, &color_global_white);
    textbox_set_background_color(keycmd, &color_global_clear);
    textbox_reset(keycmd);
	
	

}


static void autocompletion_update_lines(AutoCompletion *ac, struct autocompletion_item *items, int num_items)
{
    if (ac->lines) {
	textlines_destroy(ac->lines);
	ac->lines = NULL;
    }

    Layout *lines_lt = layout_add_child(ac->inner_layout);

    lines_lt->y.type = STACK;
    lines_lt->y.value = 10.0;
    lines_lt->w.type = SCALE;
    lines_lt->w.value = 1.0;
    /* lines_lt->h.value = 500; */
    layout_force_reset(lines_lt);
    ac->selection = -1;

    ac->lines = textlines_create(
	(void *)items,
	sizeof(struct autocompletion_item),	
	num_items,
	create_autocomplete_tline,
	ac->tline_filter,
	/* create_autocomplete_tline, */
	lines_lt,
	&ac->selection);

    layout_size_to_fit_children_v(ac->inner_layout, true, 0);
    if (ac->inner_layout->rect.y + ac->inner_layout->rect.h > main_win->h_pix) {
	ac->inner_layout->rect.h = main_win->h_pix - ac->inner_layout->rect.y - 60;
    }

    layout_size_to_fit_children_v(ac->outer_layout, true, 20);


    /* /\* Add scroll pad *\/ */
    /* ac->lines->container->rect.h += ac->layout->rect.h; */
    /* layout_set_values_from_rect(ac->lines->container); */

}

static int autocompletion_te_afteredit(Text *self, void *xarg)
{
    AutoCompletion *ac = (AutoCompletion *)xarg;

    if (ac->update_records) {
	struct autocompletion_item *dst = NULL;
	int num_records = ac->update_records(ac, &dst);
	autocompletion_update_lines(ac, dst, num_records);
	if (dst) free(dst);
    }
    return 0;
}



void autocompletion_init(
    AutoCompletion *ac,
    Layout *layout,
    int update_records(AutoCompletion *self, struct autocompletion_item **dst_loc),
    TlinesFilter tline_filter)

{
    autocompletion_deinit(ac);
    ac->outer_layout = layout;
    ac->inner_layout = layout_add_child(ac->outer_layout);
    ac->update_records = update_records;
    ac->tline_filter = tline_filter;
    /* ac->items = items; */
    
    Layout *entry_lt = layout_add_child(ac->inner_layout);
    entry_lt->w.type = SCALE;
    entry_lt->w.value = 1.0;
    entry_lt->h.value = 20.0;
    snprintf(ac->entry_buf, AUTOCOMPLETE_ENTRY_BUFLEN, "Start typing to search...");
    ac->entry = textentry_create(
	entry_lt,
	ac->entry_buf,
	AUTOCOMPLETE_ENTRY_BUFLEN,
	main_win->mono_bold_font,
	16,
	NULL, NULL, NULL,
	main_win);
    ac->entry->tb->text->after_edit = autocompletion_te_afteredit;
    ac->entry->tb->text->after_edit_target = ac;


    layout_pad(ac->inner_layout, 20, 20);
    layout_force_reset(ac->outer_layout);

    


    /* ac->lines = textlines_create( */
    /*     (void **)&items, */
    /* 	num_items, */
    /* 	NULL, */
    /* 	create_autocomplete_tline, */
    /* 	lines_lt, */
    /* 	NULL); */
}

void autocompletion_deinit(AutoCompletion *ac)
{
    if (ac->lines) {
	textlines_destroy(ac->lines);
	ac->lines = NULL;
    }
    if (ac->entry) {
	textentry_destroy(ac->entry);
	ac->entry = NULL;
    }
    if (ac->outer_layout) {
	layout_destroy(ac->outer_layout);
	ac->outer_layout = NULL;
    }
}

void autocompletion_draw(AutoCompletion *ac)
{
    SDL_SetRenderDrawColor(main_win->rend, 30, 30, 30, 255);
    geom_fill_rounded_rect(main_win->rend, &ac->outer_layout->rect, STD_CORNER_RAD);
    /* SDL_RenderFillRect(main_win->rend, &ac->outer_layout->rect); */

    SDL_RenderSetClipRect(main_win->rend, &ac->inner_layout->rect);
    if (ac->lines) {
	textlines_draw(ac->lines);
    }
    SDL_RenderSetClipRect(main_win->rend, NULL);

    textentry_draw(ac->entry);
}

void autocompletion_reset_selection(AutoCompletion *ac, int new_sel)
{
    int old_sel = ac->selection;
    if (new_sel >= ac->lines->num_items) {
	new_sel = ac->lines->num_items - 1;
    } else if (new_sel < -1) {
	new_sel = -1;
    }
    if (new_sel != old_sel) {
	ac->selection = new_sel;
	if (old_sel >=0) {
	    Textbox *old = ac->lines->items[old_sel].tb;
	    textbox_set_background_color(old, &color_global_clear);            
	}
	if (new_sel >=0) {
	    Textbox *new = ac->lines->items[new_sel].tb;
	    textbox_set_background_color(new, &highlighted_line);
	}
    }

}
void autocompletion_escape()
{
    main_win->ac_active = false;
    window_pop_mode(main_win);
    txt_stop_editing(main_win->txt_editing);
}

void autocompletion_select(AutoCompletion *ac)
{
    autocompletion_escape();
    if (ac->selection >= 0) {
	TLinesItem item = ac->lines->items[ac->selection];
	UserFn *fn = item.obj;
	fn->do_fn(fn);
    }
    /* ac->select(ac); */
}
bool autocompletion_triage_mouse_motion(AutoCompletion *ac)
{
    if (!SDL_PointInRect(&main_win->mousep, &ac->inner_layout->rect)) return false;
    if (!ac->lines) return false;
    
    if (SDL_PointInRect(&main_win->mousep, &ac->lines->container->rect)) {
	int y_diff = main_win->mousep.y - ac->lines->container->rect.y;
	int line_h = ac->lines->items[0].tb->layout->rect.h;
	int line_spacing = AUTOCOMPLETE_LINE_SPACING * main_win->dpi_scale_factor;
	int item = y_diff / (line_h + line_spacing);
	if (item >= 0 && item < ac->lines->num_items) {
	    autocompletion_reset_selection(ac, item);
	}
	/* for (int i=item; i<ac->lines->num_items; i++) { */
	/*     TLinesItem *item = ac->lines->items + i; */
	/*     if (SDL_PointInRect(&main_win->mousep, &item->tb->layout->rect)) { */
	/* 	autocompletion_reset_selection(ac, i); */
	/* 	break; */
	/*     } */
	/* } */
    }
    return true;
}


bool autocompletion_triage_mouse_click(AutoCompletion *ac)
{
    if (!SDL_PointInRect(&main_win->mousep, &ac->inner_layout->rect)) {
	autocompletion_escape();
    }
    if (SDL_PointInRect(&main_win->mousep, &ac->lines->container->rect)) {
	int y_diff = main_win->mousep.y - ac->lines->container->rect.y;
	int line_h = ac->lines->items[0].tb->layout->rect.h;
	int line_spacing = AUTOCOMPLETE_LINE_SPACING * main_win->dpi_scale_factor;
	int item = y_diff / (line_h + line_spacing);
	if (item >= 0 && item < ac->lines->num_items) {
	    autocompletion_reset_selection(ac, item);
	    autocompletion_select(ac);
	}
    }

    return true;
}


Layout *autocompletion_scroll(int y, bool dynamic)
{
    AutoCompletion *ac = &main_win->ac;
    SDL_Rect padded = ac->lines->container->rect;
    /* Add scroll pad */
    padded.h += ac->inner_layout->rect.h;

    if (SDL_PointInRect(&main_win->mousep, &padded)) {
	layout_scroll(ac->lines->container, 0, y, dynamic);
	return ac->lines->container;
    }
    return NULL;;
}
