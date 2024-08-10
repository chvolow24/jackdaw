#include "dir.h"
#include "geometry.h"
#include "modal.h"
#include "textbox.h"

#define MODAL_V_PADDING 5
#define MODAL_V_PADDING_TIGHT 0
#define MODAL_BOTTOM_PAD 12
#define MODAL_FONTSIZE_H1 36
#define MODAL_FONTSIZE_H2 24
#define MODAL_FONTSIZE_H3 18
#define MODAL_FONTSIZE_H4 16
#define MODAL_FONTSIZE_H5 14
#define MODAL_P_FONTSIZE 12

#define TEXTENTRY_V_PAD 6
#define TEXTENTRY_H_PAD 4

#define MODAL_STD_CORNER_RAD STD_CORNER_RAD

/* #define modal_color_background (menu_std_clr_bckgrnd) */
/* #define modal_color_border (menu_std_clr_inner_border) */
/* #define modal_textentry_text_color (menu_std_clr_txt) */


extern Window *main_win;

extern SDL_Color color_global_black;
extern SDL_Color color_global_clear;
extern SDL_Color color_global_white;
extern SDL_Color control_bar_bckgrnd;

extern SDL_Color menu_std_clr_inner_border;
extern SDL_Color menu_std_clr_outer_border;
extern SDL_Color menu_std_clr_bckgrnd;
extern SDL_Color menu_std_clr_txt;
extern SDL_Color menu_std_clr_annot_txt;
extern SDL_Color menu_std_clr_highlight;
extern SDL_Color menu_std_clr_sctn_div;


/* SDL_Color modal_color_background = (SDL_Color){255, 200, 100, 255}; */
/* SDL_Color modal_color_background = (SDL_Color){10, 40, 30, 255}; */
/* SDL_Color modal_color_background = (SDL_Color){110, 70, 40, 255}; */
/* SDL_Color modal_color_background = (SDL_Color){220, 200, 150, 255}; */



SDL_Color modal_color_background = (SDL_Color){220, 160, 0, 245};
SDL_Color modal_color_border = (SDL_Color){200, 200, 200, 255};
SDL_Color modal_color_border_outer = (SDL_Color){10, 10, 10, 255};
SDL_Color modal_color_border_selected = (SDL_Color) {10, 10, 155, 255};
SDL_Color modal_textentry_background = (SDL_Color) {200, 200, 200, 255};
SDL_Color modal_textentry_text_color = (SDL_Color) {10, 30, 100, 255};
SDL_Color modal_button_color = (SDL_Color) {200, 200, 200, 255};

Modal *modal_create(Layout *lt)
{
    Modal *modal = calloc(1, sizeof(Modal));
    /* Layout *padded = layout_add_child(lt); */
    /* padded->x.value.intval = MODAL_V_PADDING; */
    /* padded->y.value.intval = MODAL_V_PADDING; */
    /* padded->w.type = PAD; */
    /* padded->h.type = PAD; */
    modal->layout = lt;
    return modal;
}

static void textentry_destroy(TextEntry *te)
{
    textbox_destroy(te->tb);
    free(te);
}
static void modal_el_destroy(ModalEl *el)
{
    if (el->obj) {
	switch (el->type) {
	case MODAL_EL_MENU:
	    menu_destroy((Menu *)el->obj);
	    break;
	case MODAL_EL_TEXTENTRY:
	    textentry_destroy(((TextEntry *)el->obj));
	    break;
	case MODAL_EL_TEXTAREA:
	    txt_area_destroy((TextArea *)el->obj);
	    break;
	case MODAL_EL_TEXT:
	    textbox_destroy((Textbox *)el->obj);
	    break;
	case MODAL_EL_DIRNAV:
	    dirnav_destroy((DirNav *)el->obj);
	    break;
	case MODAL_EL_BUTTON:
	    button_destroy((Button *)el->obj);
	    /* free((Button *)el->obj); */
	    break;
	    
	}
    }
    free (el);
}
void modal_destroy(Modal *modal)
{
    for (uint8_t i=0; i<modal->num_els; i++) {
	modal_el_destroy(modal->els[i]);
    }
    layout_destroy(modal->layout);
    free(modal);

}

static ModalEl *modal_add_el(Modal *modal)
{
    Layout *lt = layout_add_child(modal->layout);
    lt->y.type = STACK;
    lt->y.value.intval = MODAL_V_PADDING;
    lt->w.type = SCALE;
    lt->w.value.floatval = 1.0;

    lt->x.type = REL;
    lt->x.value.intval = MODAL_V_PADDING * 2;
    lt->w.type = PAD;
    layout_reset(lt);
    ModalEl *new_el = calloc(1, sizeof(ModalEl));
    new_el->layout = lt;
    modal->els[modal->num_els] = new_el;
    modal->num_els++;
    return new_el;
}

static ModalEl *modal_add_text(Modal *modal, Font *font, int font_size, SDL_Color *color, char *text, TextAlign align, bool truncate)
{
    ModalEl *el = modal_add_el(modal);
    el->type = MODAL_EL_TEXT;
    /* el->selectable = false; */
    Textbox *tb = textbox_create_from_str(text, el->layout, font, font_size, main_win);
    textbox_set_border(tb, &color_global_clear, 0);
    textbox_set_background_color(tb, &color_global_clear);
    textbox_set_text_color(tb, color);
    textbox_set_align(tb, align);
    /* Text *txt = txt_create_from_str(text, strlen(text), &el->layout->rect, font, font_size, *color, align, truncate, main_win); */
    el->obj = (void *)tb;
    return el;
}

static void layout_size_to_fit_text_v(ModalEl *el)
{
    if (el->type != MODAL_EL_TEXT) {
	return;
    }
    Textbox *tb = (Textbox *)el->obj;
    /* int saved_w = tb->layout->rect.w; */
    /* textbox_size_to_fit(tb, 5, 5); */
    /* tb->layout->rect.w = saved_w; */
    /* layout_set_values_from_rect(tb->layout); */
    textbox_size_to_fit(tb, MODAL_V_PADDING, MODAL_V_PADDING);
    /* layout_center_agnostic(tb->layout, true, false); */
    
}


void project_draw();
ModalEl *modal_add_header(Modal *modal, const char *text, SDL_Color *color, int level)
{
    ModalEl *el;
    int fontsize;
    TextAlign ta = CENTER;
    switch (level) {
    case 1:
	fontsize = MODAL_FONTSIZE_H1;
	break;
    case 2:
	fontsize = MODAL_FONTSIZE_H2;
	break;
    case 3:
	fontsize = MODAL_FONTSIZE_H3;
	break;
    case 4:
	fontsize = MODAL_FONTSIZE_H4;
	break;
    case 5:
	fontsize = MODAL_FONTSIZE_H5;
	ta = CENTER_LEFT;
	break;
    
    }
    el = modal_add_text(modal, main_win->bold_font, fontsize, color, (char *)text, ta, false);
    /* modal->selectable_indices[modal->num_selectable] = modal->num_els -1; */
    /* modal->num_selectable++; */
    layout_size_to_fit_text_v(el);
    if (level == 5) {
	el->layout->x.value.intval = 0;
    }
    layout_force_reset(modal->layout);
    /* layout_size_to_fit_children(el->layout, true, MODAL_V_PADDING); */
    /* modal_reset(modal); */
    return el;
}


ModalEl *modal_add_p(Modal *modal, const char *text, SDL_Color *color)
{
    ModalEl *el = modal_add_el(modal);

    el->type = MODAL_EL_TEXTAREA;
    TextArea *ta = txt_area_create(text, el->layout, main_win->std_font, MODAL_P_FONTSIZE, *color, main_win);
    /* fprintf(stdout, "AFTER creation, TA y and height: %d, %d\n", el->layout->rect.y,  el->layout->rect.h); */
    el->obj = (void *)ta;
    /* layout_force_reset(modal->layout); */
    /* fprintf(stdout, "AFTER force reset, TA y and height: %d, %d\n", el->layout->rect.y,  el->layout->rect.h); */
    /* modal_reset(modal); */
    /* fprintf(stdout, "AFTER modal reset, TA height: %d\n", el->layout->rect.h); */
    
    /* layout_size_to_fit_children(el->layout, true, MODAL_V_PADDING); */
    /* modal_reset(modal); */
    /* modal_reset(modal); */
    return el;
}

ModalEl *modal_add_dirnav(Modal *modal, const char *dirpath, int (*dir_to_tline_filter)(void *dp_v, void *dn_v))
{
    /* modal_add_header(modal, "- DirNav -", &color_global_black, 4); */
    /* modal_add_p(modal, "n (f) - next item\np (d) - previous item\n<ret> (<spc>) - drill down\nS-n (S-f) - escape DirNav next\nS-p (S-d) - escape DirNav previous", &color_global_black); */
    ModalEl *el = modal_add_el(modal);
    el->layout->y.value.intval = MODAL_V_PADDING_TIGHT;
    modal->selectable_indices[modal->num_selectable] = modal->num_els -1;
    modal->num_selectable++;
    /* el->layout->x.type = REL; */
    /* el->layout->x.value.intval = MODAL_V_PADDING * 2; */
    /* el->layout->w.type = PAD; */
    /* layout_reset(el->layout); */
    el->type = MODAL_EL_DIRNAV;
    DirNav *dn = dirnav_create(dirpath, el->layout, dir_to_tline_filter);
    el->obj = (void *)dn;
    return el;
}

ModalEl *modal_add_button(Modal *modal, char *text, ComponentFn action)
{
    ModalEl *el = modal_add_el(modal);
    el->type = MODAL_EL_BUTTON;
    modal->selectable_indices[modal->num_selectable] = modal->num_els - 1;
    modal->num_selectable++;
    el->layout->w.type = ABS;
    Button *button = button_create(el->layout, text, action, &color_global_black, &modal_button_color);
    layout_center_agnostic(el->layout, true, false);
    textbox_reset_full(button->tb);
    el->obj = (void *)button;
    return el;
}



ModalEl *modal_add_textentry(Modal *modal, char *init_val, int (*validation)(Text *txt, char input), int (*completion)(Text *))
{
    ModalEl *el = modal_add_el(modal);
    el->layout->y.value.intval = MODAL_V_PADDING_TIGHT;
    modal->selectable_indices[modal->num_selectable] = modal->num_els - 1;
    modal->num_selectable++;
    /* el->layout->x.type = REL; */
    /* el->layout->x.value.intval = MODAL_V_PADDING * 2; */
    /* el->layout->w.type = PAD; */
    /* layout_reset(el->layout); */
    el->type = MODAL_EL_TEXTENTRY;

    TextEntry *te = calloc(1, sizeof(TextEntry));
    te->tb = textbox_create_from_str(init_val, el->layout, main_win->bold_font, 12, main_win);
    
    textbox_set_text_color(te->tb, &modal_textentry_text_color);
    textbox_set_background_color(te->tb, &modal_textentry_background);
    textbox_set_align(te->tb, CENTER_LEFT);
    textbox_size_to_fit(te->tb, TEXTENTRY_H_PAD, TEXTENTRY_V_PAD);
    textbox_reset_full(te->tb);
    te->tb->text->validation = validation;
    te->tb->text->completion = completion;
    el->obj = (void *)te;
    return el;
    
}

static void modal_el_reset(ModalEl *el)
{
    /* fprintf(stdout, "Resetting el... \n"); */
    /* SDL_Delay(1000); */
    /* layout_force_reset(el->layout); */
    switch (el->type) {
    case MODAL_EL_MENU:
	menu_reset_layout((Menu *)el->obj);
	break;
    case MODAL_EL_TEXT:
	/* textbox_size_to_fit((Textbox *)el->obj, 0, MODAL_V_PADDING); */
	/* fprintf(stdout, "Reseting Textbox %s to lt %d %d %d %d\n", ((Textbox *)el->obj)->text->value_handle, ((Textbox *)el->obj)->layout->rect.x, ((Textbox *)el->obj)->layout->rect.y, ((Textbox *)el->obj)->layout->rect.w, ((Textbox *)el->obj)->layout->rect.h); */
	textbox_reset_full((Textbox *)el->obj);
	break;
    case MODAL_EL_TEXTENTRY:
    case MODAL_EL_TEXTAREA:
    case MODAL_EL_DIRNAV:
    case MODAL_EL_BUTTON:
	break;
    }
}

void modal_reset(Modal *modal)
{
    layout_force_reset(modal->layout);
    fprintf(stdout, "modal reset!\n");
    for (uint8_t i=0; i<modal->num_els; i++) {
	modal_el_reset(modal->els[i]);
	/* layout_force_reset(modal->layout); */
    }
    layout_size_to_fit_children(modal->layout, true, MODAL_V_PADDING);
    modal->layout->h.value.intval += MODAL_BOTTOM_PAD;
    layout_center_agnostic(modal->layout, true, true);
    layout_reset(modal->layout);

}

static void modal_el_draw(ModalEl *el)
{
    switch (el->type) {
    case MODAL_EL_MENU:
	menu_draw((Menu *)el->obj);
	break;
    case MODAL_EL_TEXTENTRY:
	textbox_draw(((TextEntry *)el->obj)->tb);
	break;
    case MODAL_EL_TEXTAREA:
	/* layout_draw(main_win, el->layout); */
	txt_area_draw((TextArea *)el->obj);
	break;
    case MODAL_EL_TEXT:
	textbox_draw((Textbox *)el->obj);
	/* layout_draw(main_win, el->layout); */
	break;
    case MODAL_EL_DIRNAV:
	dirnav_draw((DirNav *)el->obj);
	break;
    case MODAL_EL_BUTTON:
	button_draw((Button *)el->obj);
	/* textbox_draw(((Button *)el->obj)->tb); */
	break;
    }
}



/* void layout_fprint(FILE *f, Layout *lt); */
void layout_write(FILE *f, Layout *lt, int indent);
void modal_draw(Modal *modal)
{
   
    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(modal_color_background));
    geom_fill_rounded_rect(main_win->rend, &modal->layout->rect, MODAL_STD_CORNER_RAD);

    SDL_Rect border = modal->layout->rect;
    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(menu_std_clr_outer_border));
    geom_draw_rounded_rect(main_win->rend, &border, MODAL_STD_CORNER_RAD);
    border.x++;
    border.y++;
    border.w -= 2;
    border.h -= 2;
    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(menu_std_clr_inner_border));
    geom_draw_rounded_rect(main_win->rend, &border, MODAL_STD_CORNER_RAD - 1);
    /* SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(modal_color_border_outer)); */
    /* geom_draw_rounded_rect(main_win->rend, &modal->layout->rect, MODAL_STD_CORNER_RAD); */
    for (uint8_t i=0; i<modal->num_els; i++) {
	/* fprintf(stdout, "drawing el\n"); */
	modal_el_draw(modal->els[i]);
    }
    if (modal->num_selectable > 0) {
	SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(modal_color_border_selected));
	geom_draw_rect_thick(main_win->rend, &modal->els[modal->selectable_indices[modal->selected_i]]->layout->rect, 2, main_win->dpi_scale_factor);
	/* SDL_Rect r = modal->els[modal->selectable_indices[modal->selected_i]]->layout->rect; */
	/* fprintf(stdout, "R: %d %d %d %d\n", r.x, r.y, r.w, r.h); */
    }
    /* layout_write(stdout, modal->layout, 0); */
}

void modal_next(Modal *modal)
{
    int num_selectable = modal->num_selectable;
    ModalEl *el = modal->els[modal->selectable_indices[modal->selected_i]];
    if (el->type == MODAL_EL_DIRNAV) {
	dirnav_next((DirNav *)el->obj);
    } else if (modal->selected_i < num_selectable - 1) modal->selected_i++;

}

void modal_next_escape(Modal *modal);
void modal_move_onto(Modal *modal)
{
    ModalEl *el = modal->els[modal->selectable_indices[modal->selected_i]];
    fprintf(stdout, "El: %p\n", el);
    switch (el->type) {
    case MODAL_EL_TEXTENTRY:
	txt_edit((((TextEntry *)el->obj))->tb->text, project_draw);
	modal_next_escape(modal);
	break;
    default:
	break;
    }
}

void modal_previous(Modal *modal)
{
    ModalEl *el = modal->els[modal->selectable_indices[modal->selected_i]];
    if (el->type == MODAL_EL_DIRNAV) {
	dirnav_previous((DirNav *)el->obj);
	return;
    } else if (modal->selected_i > 0) {
	modal->selected_i--;
	modal_move_onto(modal);
    }
    /* fprintf(stdout, "Prev, selected i: %d\n", modal->selected_i); */
}

void modal_next_escape(Modal *modal)
{
    fprintf(stdout, "Next escape\n");
    int num_selectable = modal->num_selectable;
    if (modal->selected_i < num_selectable - 1) {
	modal->selected_i++;
	modal_move_onto(modal);
    }
}

void modal_previous_escape(Modal *modal)
{
    if (modal->selected_i > 0) {
	modal->selected_i--;
        /* fprintf(stdout, "Prev, selected i: %d\n", modal->selected_i); */
	modal_move_onto(modal);
    }
}

/* void modal_next_escape(Modal *modal); */
void modal_select(Modal *modal)
{
    ModalEl *current_el = modal->els[modal->selectable_indices[modal->selected_i]];
    switch (current_el->type) {
    case MODAL_EL_DIRNAV:
	dirnav_select(current_el->obj);
	break;
    case MODAL_EL_TEXTENTRY:
	txt_edit(((TextEntry *)current_el->obj)->tb->text, project_draw);
	modal_next_escape(modal);
	break;
    case MODAL_EL_BUTTON:
	((Button *)(current_el->obj))->action(modal, NULL);
    default:
	break;
    }
}

void modal_submit_form(Modal *modal)
{
    fprintf(stdout, "modal submit form\n");
    if (modal->submit_form) {
	fprintf(stdout, "has fn\n");
	modal->submit_form((void *)modal, NULL);
    }
}

/* static void modal_el_triage_mouse(ModalEl *el, SDL_Point *mousep, bool click) */
/* { */
    
/* } */

bool modal_triage_mouse(Modal *modal, SDL_Point *mousep, bool click)
{
    if (!SDL_PointInRect(mousep, &modal->layout->rect)) {
	if (click) {
	    window_pop_modal(main_win);
	}
	return false;
    }
    ModalEl *el;
    uint8_t i;
    bool found = false;
    for (i=0; i<modal->num_els; i++) {
	el = modal->els[i];
	if (SDL_PointInRect(mousep, &el->layout->rect)) {
	    found = true;
	    break;
	}
    }
    if (found) {
	/* Already selected */
	if (click) {
	    /* fprintf(stdout, "Selected i: %d\n", modal->selected_i); */
	    if (el == modal->els[modal->selectable_indices[modal->selected_i]]) {
		/* fprintf(stdout, "El alreadyx selected\n"); */
		switch (el->type) {
		case MODAL_EL_BUTTON:
		    ((Button *)(el->obj))->action(modal, NULL);
		    break;
		case MODAL_EL_DIRNAV:
		    dirnav_triage_click(el->obj, mousep);
		    break;
		default:
		    break;
		
		}
	    } else {
		for (uint8_t j=0; j<modal->num_selectable; j++) {
		    /* fprintf(stdout, "Selecting new el\n"); */
		    if (modal->selectable_indices[j] == i) {
			modal->selected_i = j;
			modal_select(modal);
			break;
		    }
		}
	    }
	} else if (el->type == MODAL_EL_DIRNAV) {
	    DirNav *dn = (DirNav *)(el->obj);
	    /* fprintf(stdout, "motion in dirnav!\n"); */
	    for (uint16_t i=0; i<dn->lines->num_items; i++) {
		TLinesItem *item = dn->lines->items[i];
		if (SDL_PointInRect(mousep, &item->tb->layout->rect)) {
		    dirnav_select_item(dn, i);
		    /* dn->current_line = i; */
		}
	    }
	}
    }
    return true;
}


void modal_triage_wheel(Modal *modal, SDL_Point *mousep, int x, int y)
{
    ModalEl *el;
    for (uint8_t i=0; i<modal->num_els; i++) {
	el = modal->els[i];
	if (el->type == MODAL_EL_DIRNAV) {
	    /* fprintf(stdout, "Found dirnav\n"); */
	    if (SDL_PointInRect(mousep, &el->layout->rect)) {
		int *scroll_offset = &((DirNav *)el->obj)->lines->container->scroll_offset_v;
		*scroll_offset += y;
		if (*scroll_offset > 0) *scroll_offset = 0;
		layout_reset(el->layout);		
	    }
	    break;
	}
    }
}
