#include "dir.h"
#include "modal.h"

#define MODAL_V_PADDING 5
#define MODAL_FONTSIZE_H1 36
#define MODAL_FONTSIZE_H2 24
#define MODAL_FONTSIZE_H3 18
#define MODAL_FONTSIZE_H4 16
#define MODAL_FONTSIZE_H5 14
#define MODAL_P_FONTSIZE 12


extern Window *main_win;

extern SDL_Color color_global_black;
extern SDL_Color color_global_clear;
extern SDL_Color color_global_white;

SDL_Color modal_color_background = (SDL_Color){255, 200, 100, 255};

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

static ModalEl *modal_add_el(Modal *modal)
{
    Layout *lt = layout_add_child(modal->layout);
    lt->y.type = STACK;
    lt->y.value.intval = MODAL_V_PADDING;
    lt->w.type = SCALE;
    lt->w.value.floatval = 1.0;
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
    Textbox *tb = textbox_create_from_str(text, el->layout, font, font_size, main_win);
    textbox_set_border(tb, &color_global_clear, 0);
    textbox_set_background_color(tb, &color_global_clear);
    textbox_set_text_color(tb, color);
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
    layout_center_agnostic(tb->layout, true, false);
    
}


void project_draw();
ModalEl *modal_add_header(Modal *modal, const char *text, SDL_Color *color, int level)
{
    ModalEl *el;
    int fontsize;
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
	break;
    
    }
    el = modal_add_text(modal, main_win->bold_font, fontsize, color, (char *)text, CENTER, false);
    layout_size_to_fit_text_v(el);
    layout_force_reset(modal->layout);
    /* layout_size_to_fit_children(el->layout, true, MODAL_V_PADDING); */
    /* modal_reset(modal); */
    return el;
}


ModalEl *modal_add_p(Modal *modal, const char *text, SDL_Color *color)
{
    ModalEl *el = modal_add_el(modal);
    el->layout->x.type = REL;
    el->layout->x.value.intval = MODAL_V_PADDING * 2;
    el->layout->w.type = PAD;
    layout_reset(el->layout);
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

ModalEl *modal_add_dirnav(Modal *modal, const char *dirpath, bool show_dirs, bool show_files)
{
    ModalEl *el = modal_add_el(modal);
    el->layout->x.type = REL;
    el->layout->x.value.intval = MODAL_V_PADDING * 2;
    el->layout->w.type = PAD;
    layout_reset(el->layout);
    el->type = MODAL_EL_DIRNAV;
    DirNav *dn = dirnav_create(dirpath, el->layout, show_dirs, show_files);
    el->obj = (void *)dn;
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
    case MODAL_EL_TEXTENTRY:
	/* textbox_reset_full((Textbox *)el->obj); */
	break;
    case MODAL_EL_TEXTAREA:
	/* txt_area_create_lines((TextArea *)el->obj); */
	break;
    case MODAL_EL_TEXT:
	/* textbox_size_to_fit((Textbox *)el->obj, 0, MODAL_V_PADDING); */
	/* fprintf(stdout, "Reseting Textbox %s to lt %d %d %d %d\n", ((Textbox *)el->obj)->text->value_handle, ((Textbox *)el->obj)->layout->rect.x, ((Textbox *)el->obj)->layout->rect.y, ((Textbox *)el->obj)->layout->rect.w, ((Textbox *)el->obj)->layout->rect.h); */
	textbox_reset_full((Textbox *)el->obj);
	/* fprintf(stdout, "Reset Textbox %s to lt %d %d %d %d\n", ((Textbox *)el->obj)->text->value_handle, ((Textbox *)el->obj)->layout->rect.x, ((Textbox *)el->obj)->layout->rect.y, ((Textbox *)el->obj)->layout->rect.w, ((Textbox *)el->obj)->layout->rect.h); */
	/* fprintf(stdout, "Reset Textbox %s to lt y: %d %d %d %d\n", ((Textbox *)el->obj)->text->value_handle, ((Textbox *)el->obj)->layout->rect.y); */
	/* layout_force_reset(el->layout); */
    case MODAL_EL_DIRNAV:
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

}

static void modal_el_draw(ModalEl *el)
{
    switch (el->type) {
    case MODAL_EL_MENU:
	menu_draw((Menu *)el->obj);
	break;
    case MODAL_EL_TEXTENTRY:
	textbox_draw((Textbox *)el->obj);
	break;
    case MODAL_EL_TEXTAREA:
	layout_draw(main_win, el->layout);
	txt_area_draw((TextArea *)el->obj);
	break;
    case MODAL_EL_TEXT:
	textbox_draw((Textbox *)el->obj);
	break;
    case MODAL_EL_DIRNAV:
	dirnav_draw((DirNav *)el->obj);
    }
}



/* void layout_fprint(FILE *f, Layout *lt); */
void layout_write(FILE *f, Layout *lt, int indent);
void modal_draw(Modal *modal)
{
    /* layout_reset(modal->layout); */
    /* modal_reset(modal); */
    /* layout_force_reset(modal->layout); */
    /* layout_force_reset(modal->layout); */
    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(modal_color_background));
    SDL_RenderFillRect(main_win->rend, &modal->layout->rect);
    layout_draw(main_win, modal->layout);
    for (uint8_t i=0; i<modal->num_els; i++) {
	/* fprintf(stdout, "drawing el\n"); */
	modal_el_draw(modal->els[i]);
    }
    /* layout_write(stdout, modal->layout, 0); */
}


