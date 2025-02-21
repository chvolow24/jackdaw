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

extern Window *main_win;

#define AUTOCOMPLETE_LINE_SPACING 3

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

void breakfn();

extern SDL_Color color_global_red;
static void create_autocomplete_tline(
    TextLines *tlines,
    TLinesItem *current_line,
    const void *src_item,
    void *x_arg)
{
    /* struct autocompletion_item **a_loc = (struct autocompletion_item **)current_item; */
    /* struct autocompletion_item *arr = *a_loc; */
    /* /\* fprintf(stderr, "(%s %p)\n", items->str, items->obj); *\/ */
    /* struct autocompletion_item item = arr[0]; */
    struct autocompletion_item *item = (struct autocompletion_item *)src_item;
    

    AutoCompletion *ac = (AutoCompletion *)x_arg;
    
    current_line->obj = item->obj;

    Layout *lt = layout_add_child(tlines->container);
    lt->y.type = STACK;
    lt->y.value = AUTOCOMPLETE_LINE_SPACING;
    lt->h.value = 70;
    lt->w.value = 500;

    ac->selection = 4;
    current_line->tb = textbox_create_from_str(item->str, lt, main_win->mono_bold_font, 16, main_win);
    if (current_line - tlines->items == ac->selection) {
	static SDL_Color c = {255, 0, 0, 255};
	breakfn();
	textbox_set_background_color(current_line->tb, &c);
	
    }
    textbox_set_align(current_line->tb, CENTER_LEFT);
    textbox_set_pad(current_line->tb, 4, 1);
    textbox_size_to_fit(current_line->tb, 4, 1);
    current_line->tb->layout->w.value = 1.0;
    current_line->tb->layout->w.type = SCALE;


    textbox_reset(current_line->tb);
}


static void autocompletion_update_lines(AutoCompletion *ac, struct autocompletion_item *items, int num_items)
{
    if (ac->lines) {
	textlines_destroy(ac->lines);
	ac->lines = NULL;
    }

    Layout *lines_lt = layout_add_child(ac->layout);

    lines_lt->y.type = STACK;
    lines_lt->w.type = SCALE;
    lines_lt->w.value = 1.0;
    lines_lt->h.value = 500;
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
}

static int autocompletion_te_afteredit(Text *self, void *xarg)
{
    AutoCompletion *ac = (AutoCompletion *)xarg;

    if (ac->update_records) {
	struct autocompletion_item *dst = NULL;
	int num_records = ac->update_records(ac, &dst);
	/* fprintf(stderr, "\nNUM RECORDS: %d\n", num_records); */
	/* for (int i=0; i<num_records; i++) { */
	/*     fprintf(stderr, "%d: %s\n", i, dst[i].str); */
	/* } */

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
    /* AutoCompletion *ac = calloc(1, sizeof(AutoCompletion)); */
    
    ac->layout = layout;
    ac->update_records = update_records;
    ac->tline_filter = tline_filter;
    /* ac->items = items; */
    
    Layout *entry_lt = layout_add_child(layout);
    entry_lt->w.type = SCALE;
    entry_lt->w.value = 1.0;
    entry_lt->h.value = 20.0;
    /* sprintf(ac->entry_buf, "example"); */
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
    


    /* ac->lines = textlines_create( */
    /*     (void **)&items, */
    /* 	num_items, */
    /* 	NULL, */
    /* 	create_autocomplete_tline, */
    /* 	lines_lt, */
    /* 	NULL); */
}

void autocompletion_draw(AutoCompletion *ac)
{
    layout_force_reset(ac->layout);
    textentry_draw(ac->entry);
    if (ac->lines) {
	textlines_draw(ac->lines);
    }
}


