#include "color.h"
#include "components.h"
#include "endpoint.h"
#include "eq.h"
#include "geometry.h"
#include "input.h"
#include "input_mode.h"
#include "layout.h"
#include "menu.h"
#include "status.h"
#include "symbol.h"
#include "text.h"
#include "textbox.h"
#include "value.h"
#include "waveform.h"

extern Window *main_win;
extern Symbol *SYMBOL_TABLE[];
#define SLIDER_INNER_PAD 2
#define SLIDER_TICK_W 2

#define RADIO_BUTTON_LEFT_COL_W 10
#define SLIDER_MAX_LABEL_COUNTDOWN 80
#define SLIDER_NUDGE_PROP 0.01
#define BUTTON_COLOR_CHANGE_STD_DELAY 8

#define TEXTENTRY_V_PAD 4
#define TEXTENTRY_H_PAD 8


/* SDL_Color fslider_bckgrnd = {60, 60, 60, 255}; */
/* SDL_Color fslider_bar_container_bckgrnd =  {190, 190, 190, 255}; */
/* SDL_Color fslider_bar_color = {20, 20, 120, 255}; */
extern struct colors colors;

SDL_Color slider_bckgrnd = {60, 60, 60, 255};
SDL_Color slider_bar_container_bckgrnd =  {40, 40, 40, 248};
SDL_Color slider_bar_color = {12, 107, 249, 250};

SDL_Color textentry_background = (SDL_Color) {200, 200, 200, 255};
SDL_Color textentry_text_color = (SDL_Color) {0, 0, 0, 255};

SDL_Color tgl_bckgrnd = {110, 110, 110, 255};



/* Slider fslider_create(Layout *layout, SliderOrientation orientation, SliderType type, SliderStrFn *fn) */
Slider *slider_create(
    Layout *layout,
    Endpoint *ep,
    Value min,
    Value max,
    /* void *value, */
    /* ValType val_type, */
    enum slider_orientation orientation,
    enum slider_style style,
    LabelStrFn label_str_fn,
    /* SliderStrFn *create_label_fn, */
    /* ComponentFn action, */
    /* void *target, */
    Draggable *drag_context)

{
    Slider *s = calloc(1, sizeof(Slider));
    s->ep = ep;
    s->min = min;
    s->max = max;
    /* s->create_label = create_label_fn; */
    /* s->action = action; */
    /* s->target = target; */
    s->layout = layout;
    Layout *bar_container = layout_add_child(layout);
    layout_set_name(bar_container, "bar_container");
    Layout *label = layout_add_child(layout);
    label->rect.x = layout->rect.x + layout->rect.w;
    label->rect.y = layout->rect.y;
    layout_set_values_from_rect(label);
    s->label = label_create(0, label, label_str_fn, ep->val, ep->val_type, main_win);
    s->label->parent_obj_lt  = s->layout;
    s->drag_context = drag_context;
    /* if (create_label_fn) { */
    /* 	create_label_fn(s->label_str, SLIDER_LABEL_STRBUFLEN - 1, value, val_type); */
    /* } */
    /* amp_to_dbstr(s->label_str, SLIDER_LABEL_STRBUFLEN - 1, *value); */

    /* snprintf(s->label_str, SLIDER_LABEL_STRBUFLEN - 1, "%f", *value); */
    /* s->label = textbox_create_from_str(s->label_str, label, main_win->mono_font, 12, main_win); */

    /* textbox_size_to_fit(s->label, SLIDER_LABEL_H_PAD, SLIDER_LABEL_V_PAD); */
    /* textbox_set_pad(s->label, SLIDER_LABEL_H_PAD, SLIDER_LABEL_V_PAD); */
    /* textbox_set_border(s->label, &colors.black, 2); */
    /* textbox_set_trunc(s->label, false); */
    /* layout_reset(layout); */
    /* bar_container->x.value.intval = SLIDER_INNER_PAD; */
    /* bar_container->y.value.intval = SLIDER_INNER_PAD; */
    /* bar_container->w.value.intval = (layout->rect.w - (SLIDER_INNER_PAD * main_win->dpi_scale_factor * 2)) / main_win->dpi_scale_factor; */
    /* bar_container->h.value.intval = (layout->rect.h - (SLIDER_INNER_PAD * main_win->dpi_scale_factor * 2)) / main_win->dpi_scale_factor; */
    layout_pad(bar_container, SLIDER_INNER_PAD, SLIDER_INNER_PAD);
    Layout *bar = layout_add_child(bar_container);
    s->bar_layout = bar;
    layout_set_name(bar, "bar");
    s->bar_rect = &bar->rect;
    switch (orientation) {
    case SLIDER_HORIZONTAL:
	bar->y.value = 0.0f;
	bar->h.type = SCALE;
	bar->h.value = 1.0f;
	switch (style) {
	case SLIDER_FILL:
	    bar->w.type = SCALE;
	    s->val_dim = &bar->w.value;
	    break;
	case SLIDER_TICK:
	    bar->x.type = SCALE;
	    s->val_dim = &bar->x.value;
	    bar->w.type = ABS;
	    bar->w.value = SLIDER_TICK_W;
	    break;   
	}
	break;
    case SLIDER_VERTICAL:
	bar->x.value = 0;
	bar->w.type = SCALE;
	bar->w.value = 1.0f;
	switch (style) {
	case SLIDER_FILL:
	    bar->h.type = SCALE;
	    bar->y.type = SCALE;
	    s->val_dim = &bar->h.value;
	    break;
	case SLIDER_TICK:
	    bar->y.type = SCALE;
	    s->val_dim = &bar->y.value;
	    break;   
	}
    }

    /* if (ep->restrict_range) { */
    /* 	s->min = ep->min; */
    /* 	s->max = ep->max; */
    /* } else { */
    /* 	jdaw_val_set_min(&(s->min), ep->val_type); */
    /* 	jdaw_val_set_max(&(s->max), ep->val_type); */
    /* } */
    /* s->val_type = ep->val_type; */
    /* s->value = ep->value; */
    s->orientation = orientation;
    s->style = style;
    return s;
}

/* Calls endpoint_write */
/* void slider_set_value(Slider *s, Value val, ) */
/* { */
/*     endpoint_write(s->ep, val, true, true, true, false); */
/*     /\* jdaw_val_set_ptr(s->value, s->val_type, val); *\/ */
/*     slider_reset(s); */
/* } */

void slider_set_range(Slider *s, Value min, Value max)
{
/*     s->min = min; */
/*     s->max = max; */
}

void slider_add_point_of_interest(Slider *s, Value p)
{
    if (s->num_points_of_interest == SLIDER_MAX_POINTS_OF_INTEREST) {
	return;
    }
    s->points_of_interest[s->num_points_of_interest] = p;
    s->num_points_of_interest++;
}

static Value slider_val_from_coord(Slider *s, int coord_pix)
{
    double proportion;
    switch (s->orientation) {
    case SLIDER_VERTICAL:
	proportion = ((double)s->layout->rect.y + s->layout->rect.h - coord_pix) / s->layout->rect.h;
	break;
    case SLIDER_HORIZONTAL:
	proportion = ((double)coord_pix - s->layout->rect.x) / s->layout->rect.w;
	break;
    }
    Value range = jdaw_val_sub(s->max, s->min, s->ep->val_type);
    Value val_proportion = jdaw_val_scale(range, proportion, s->ep->val_type);
    Value ret = jdaw_val_add(val_proportion, s->min, s->ep->val_type);
    return ret;
}


void layout_write(FILE *f, Layout *lt, int indent);
Value slider_reset(Slider *s)
{
    Value range = jdaw_val_sub(s->max, s->min, s->ep->val_type);
    Value slider_val = endpoint_safe_read(s->ep, NULL);
    Value filled = jdaw_val_sub(slider_val, s->min, s->ep->val_type);
    double filled_prop = jdaw_val_div_double(filled, range, s->ep->val_type);
    /* fprintf(stderr, "Filled prop? %f\n", filled_prop); */
    switch (s->style) {
    case SLIDER_FILL:
	switch (s->orientation) {
	case SLIDER_HORIZONTAL:
	    *(s->val_dim) = filled_prop;
	    break;
	case SLIDER_VERTICAL:
	    s->bar_layout->y.value = 1.0 - filled_prop;
	    *(s->val_dim) = filled_prop;
	    break;
	}
	break;
    case SLIDER_TICK:
	switch (s->orientation) {
	case SLIDER_HORIZONTAL:
	    *(s->val_dim) = filled_prop;
	    break;
	case SLIDER_VERTICAL:
	    *(s->val_dim) = 1 - filled_prop;
	    break;
	}
    }
    for (int i=0; i<s->num_points_of_interest; i++) {
	Value left = jdaw_val_sub(s->points_of_interest[i], s->min, s->ep->val_type);
	/* fprintf(stderr, "POI %d MIN %d sub? %d\n", s->points_of_interest[i].int16_v, s->min.int16_v, left.int16_v); */
	double left_prop = jdaw_val_div_double(left, range, s->ep->val_type);
	/* fprintf(stderr, "%d: %f\n", i, left_prop); */
	switch (s->orientation) {
	case SLIDER_HORIZONTAL:
	    s->points_of_interest_draw_locs_pix[i] = s->layout->rect.x + s->layout->rect.w * left_prop;
	    break;
	case SLIDER_VERTICAL:
	    s->points_of_interest_draw_locs_pix[i] = s->layout->rect.y + s->layout->rect.h - s->layout->rect.h * left_prop;
	    break;
	}
    }
    layout_reset(s->layout);
    return slider_val;
}    


void slider_draw(Slider *s)
{
    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(slider_bckgrnd));
    SDL_RenderFillRect(main_win->rend, &s->layout->rect);

    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(slider_bar_container_bckgrnd));
    SDL_RenderFillRect(main_win->rend, &s->layout->children[0]->rect);

    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(colors.grey));
    if (s->orientation == SLIDER_VERTICAL) {
	for (int i=0; i<s->num_points_of_interest; i++) {
	    SDL_RenderDrawLine(main_win->rend, s->bar_layout->rect.x, s->points_of_interest_draw_locs_pix[i], s->bar_layout->rect.x + s->bar_layout->rect.w, s->points_of_interest_draw_locs_pix[i]);
	}
    } else {
	for (int i=0; i<s->num_points_of_interest; i++) {
	    SDL_RenderDrawLine(main_win->rend, s->points_of_interest_draw_locs_pix[i], s->bar_layout->rect.y, s->points_of_interest_draw_locs_pix[i], s->bar_layout->rect.y + s->bar_layout->rect.h);
	}
    }
    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(slider_bar_color));
    SDL_RenderFillRect(main_win->rend, s->bar_rect);

    label_draw(s->label);
}

void slider_destroy(Slider *s)
{
    /* textbox_destroy(s->label); */
    label_destroy(s->label);
    layout_destroy(s->layout);
    free(s);
}

/* void (SliderStrFn)(char *dst, size_t dstsize, void *value, ValType type); */
void slider_std_labelmaker(char *dst, size_t dstsize, void *value, ValType type)
{
    jdaw_valptr_to_str(dst, dstsize, value, type, 2);
}

/* void slider_edit_made(Slider *slider) */
/* { */
/*     label_reset(slider->label); */
/* } */


bool slider_mouse_click(Slider *slider, Window *win)
{
    if (SDL_PointInRect(&main_win->mousep, &slider->layout->rect) && win->i_state & I_STATE_MOUSE_L) {
	Value newval;
	if (slider->ep && main_win->i_state & I_STATE_CMDCTRL && !(win->i_state & I_STATE_SHIFT)) {
	    newval = slider->ep->default_val;
	} else {
	    int dim = slider->orientation == SLIDER_VERTICAL ? main_win->mousep.y : main_win->mousep.x;
	    newval = slider_val_from_coord(slider, dim);
	}
	endpoint_start_continuous_change(slider->ep, false, (Value)0, slider->ep->owner_thread, newval);
	slider_reset(slider);

	/* slider_reset(slider); */
	/* slider_edit_made(slider); */
	slider->drag_context->component = (void *)slider;
	slider->drag_context->type = DRAG_SLIDER;
	return true;
    }
    return false;
}

bool slider_mouse_motion(Slider *slider, Window *win)
{
    Value newval;
    bool restrict_range = slider->ep->restrict_range;
    if (slider->ep && win->i_state & I_STATE_CMDCTRL && !(win->i_state & I_STATE_SHIFT)) {
	newval = slider->ep->default_val;
    } else {
	int dim, mindim, maxdim;
	switch (slider->orientation) {
	case SLIDER_VERTICAL:
	    dim = main_win->mousep.y;
	    mindim = slider->layout->rect.y;
	    maxdim = slider->layout->rect.y + slider->layout->rect.h;
	    break;
	case SLIDER_HORIZONTAL:
	    dim = main_win->mousep.x;
	    mindim = slider->layout->rect.x;
	    maxdim = slider->layout->rect.x + slider->layout->rect.w;
	    break;
	}
	/* int dim = slider->orientation == SLIDER_VERTICAL ? main_win->mousep.y : main_win->mousep.x; */
	/* int mindim = slider->orientation == SLIDER_ */
	if (slider->disallow_unsafe_mode || !(win->i_state & I_STATE_SHIFT && win->i_state & I_STATE_CMDCTRL)) {
	    if (dim < mindim) dim = mindim;
	    if (dim > maxdim) dim = maxdim;
	} else {
	    status_set_errstr("SLIDER UNSAFE MODE (release ctrl/shift to return to safety!)");
	    slider->ep->restrict_range = false;
	}

	newval = slider_val_from_coord(slider, dim);
    }
    endpoint_write(slider->ep, newval, true, true, true, false);
    slider->ep->restrict_range = restrict_range;
    /* jdaw_val_set_ptr(slider->value, slider->val_type, newval); */
    /* if (slider->action) */
    /* 	slider->action((void *)slider, slider->target); */
    /* track->vol = newval.float_v; */
    Value val = slider_reset(slider);
    label_reset(slider->label, val);
    /* slider_edit_made(slider); */
    /* proj->vol_changing = true; */
    return true;
}

void slider_nudge_right(Slider *slider)
{
    Value range = jdaw_val_sub(slider->max, slider->min, slider->ep->val_type);
    /* static const double slider_nudge_prop = SLIDER_NUDGE_PROP; */
    Value nudge_amt = jdaw_val_scale(range, SLIDER_NUDGE_PROP, slider->ep->val_type);
    if (jdaw_val_is_zero(nudge_amt, slider->ep->val_type)) {
	jdaw_val_set_default_incr(&nudge_amt, slider->ep->val_type);
    }
    Value val = endpoint_safe_read(slider->ep, NULL);
    val = jdaw_val_add(val, nudge_amt, slider->ep->val_type);
    if (jdaw_val_less_than(slider->max, val, slider->ep->val_type)) {
	val = slider->max;
    }
    endpoint_write(slider->ep, val, true, true, true, true);
}

void slider_nudge_left(Slider *slider)
{
    Value range = jdaw_val_sub(slider->max, slider->min, slider->ep->val_type);
    static const double slider_nudge_prop = SLIDER_NUDGE_PROP;
    Value nudge_amt = jdaw_val_scale(range, slider_nudge_prop, slider->ep->val_type);
    if (jdaw_val_is_zero(nudge_amt, slider->ep->val_type)) {
	jdaw_val_set_default_incr(&nudge_amt, slider->ep->val_type);
    }	
    Value val = endpoint_safe_read(slider->ep, NULL);
    val = jdaw_val_sub(val, nudge_amt, slider->ep->val_type);
    if (jdaw_val_less_than(slider->max, val, slider->ep->val_type)) {
	val = slider->max;
    }
    endpoint_write(slider->ep, val, true, true, true, true);
}

/* static void slider_scroll(Slider *slider, int scroll_value) */
/* { */
/*     /\* fprintf(stderr, "SCROLL VALUE: %d\n", scroll_value); *\/ */
/*     static const int MAX_SCROLL = 60; */
/*     Value range = jdaw_val_sub(slider->max, slider->min, slider->val_type); */
/*     double slider_nudge_prop = (double)scroll_value / MAX_SCROLL; */
/*     /\* fprintf(stderr, "OK scroll nudge prop %f\n", slider_nudge_prop); *\/ */
/*     Value nudge_amt = jdaw_val_scale(range, slider_nudge_prop, slider->val_type); */
/*     Value val = jdaw_val_from_ptr(slider->value, slider->val_type); */
/*     val = jdaw_val_sub(val, nudge_amt, slider->val_type); */
/*     if (jdaw_val_less_than(val, slider->min, slider->val_type)) { */
/* 	val = slider->min; */
/*     } else if (jdaw_val_less_than(slider->max, val, slider->val_type)) { */
/* 	val = slider->max; */
/*     } */
/*     jdaw_val_set_ptr(slider->value, slider->val_type, val); */
/*     slider_reset(slider); */
/*     if (slider->action) slider->action((void *)slider, slider->target); */
    
/* } */
/* static int timed_hide_slider_label(void *data) */
/* { */
/*     Slider *fs = (Slider *)data; */
/*     if (fs->editing) { */
/* 	SDL_Delay(STICK_DELAY_MS); */
/* 	fs->editing = false; */
/*     } */
/*     return 0; */
/* } */

/* static void hide_slider_label(Slider *fs) */
/* { */
/*     SDL_CreateThread(timed_hide_slider_label, "hide_slider_label", fs); */
/* } */


/* static void stop_update_track_vol_pan() */
/* { */
/*     Timeline *tl = ACTIVE_TL; */
/*     Track *trk = NULL; */
/*     for (int i=0; i<tl->num_tracks; i++) { */
/* 	trk = tl->tracks[i]; */
/* 	hide_slider_label(trk->vol_ctrl); */
/* 	hide_slider_label(trk->pan_ctrl); */
/*         /\* trk->vol_ctrl->editing = false; *\/ */
/* 	/\* trk->pan_ctrl->editing = false; *\/ */
/*     } */
/*     proj->vol_changing = false; */
/*     proj->pan_changing = false; */
/* } */


/* Button */

Button *button_create(
    Layout *lt,
    char *text,
    ComponentFn action,
    void *target,
    Font *font,
    int text_size,
    SDL_Color *text_color,
    SDL_Color *background_color)
{
    Button *button = calloc(1, sizeof(Button));
    button->action = action;
    button->target = target;
    button->tb = textbox_create_from_str(text, lt, font, text_size, main_win);
    button->tb->corner_radius = BUTTON_CORNER_RADIUS;
    textbox_set_trunc(button->tb, false);
    textbox_set_border(button->tb, text_color, 1);
    textbox_set_style(button->tb, BUTTON_CLASSIC);
    textbox_set_text_color(button->tb, text_color);
    textbox_set_background_color(button->tb, background_color);
    textbox_set_align(button->tb, CENTER);
    textbox_size_to_fit(button->tb, 6, 2);
    textbox_reset_full(button->tb);
    return button;
}

SymbolButton *symbol_button_create(
    Layout *lt,
    Symbol *symbol,
    ComponentFn action,
    void *target,
    SDL_Color *background_color)
{
    SymbolButton *button = calloc(1, sizeof(SymbolButton));
    button->action = action;
    button->target = target;
    button->symbol = symbol;
    button->layout = lt;
    button->background_color = background_color;
    return button;
}

void button_draw(Button *button)
{
    textbox_draw(button->tb);
}

void symbol_button_draw(SymbolButton *sbutton)
{
    if (sbutton->background_color) {
	symbol_draw_w_bckgrnd(sbutton->symbol, &sbutton->layout->rect, sbutton->background_color);
    } else {
	symbol_draw(sbutton->symbol, &sbutton->layout->rect);
    }
}

void button_destroy(Button *button)
{
    /* Unbind the UserFn */
    if (button->bound_userfn) {
	button->bound_userfn->bound_button = NULL;
    }
    if (button->animation) {
	session_dequeue_animation(button->animation);
    }
    textbox_destroy(button->tb);
    free(button);
}


/* bckgrnd_color is optional; the existing textbox color will be used otherwise */
void button_bind_userfn(
    Button *button,
    char *fn_id,
    InputMode im,
    SDL_Color *pressed_color,
    SDL_Color *bckgrnd_color)
{
    UserFn *userfn = input_get_fn_by_id(fn_id, im);
    if (!userfn) {
	fprintf(stderr, "Error: no function \"%s\" found in mode \"%s\".\n", fn_id, input_mode_str(im));
	return;
    }
    userfn->bound_button = button;
    button->bound_userfn = userfn;
    if (pressed_color) button->pressed_color = pressed_color;
    if (bckgrnd_color)
	button->return_color = bckgrnd_color;
    else
	button->return_color = button->tb->bckgrnd_clr;
    
}

void symbol_button_destroy(SymbolButton *sbutton)
{
    free(sbutton);
}

extern void project_active_tl_redraw();
static void button_end_animation(void *arg1, void *arg2)
{
    Button *b = (Button *)arg1;
    SDL_Color *c = (SDL_Color *)arg2;
    textbox_set_background_color(b->tb, c);
    project_active_tl_redraw();
    b->animation = NULL;
    
}
void button_press_color_change(
    Button *button,
    SDL_Color *temp_color,
    SDL_Color *return_color)
{
    textbox_set_background_color(button->tb, temp_color);
    button->animation = session_queue_animation(NULL, button_end_animation, (void *)button, (void *)return_color, BUTTON_COLOR_CHANGE_STD_DELAY);
}


TextEntry *textentry_create(
    Layout *lt,
    char *value_handle,
    int buf_len,
    Font *font,
    uint8_t text_size,
    int (*validation)(Text *txt, char input),
    int (*completion)(Text *txt, void *target),
    void *completion_target,
    Window *win) {


    TextEntry *te = calloc(1, sizeof(TextEntry));
    te->tb = textbox_create_from_str(value_handle, lt, font, text_size, win);
    /* textbox_set_text_color(te->tb, &textentry_text_color); */
    /* textbox_set_background_color(te->tb, &textentry_background); */
    /* textbox_set_border(te->tb, &colors.black, 1); */
    /* textbox_size_to_fit_v(te->tb, TEXTENTRY_V_PAD); */
    textbox_set_align(te->tb, CENTER_LEFT);
    textbox_set_pad(te->tb, 8, 0);
    /* textbox_set_(te->tb, TEXTENTRY_H_PAD, TEXTENTRY_V_PAD); */
    /* textbox_size_to_fit(te->tb, TEXTENTRY_H_PAD, TEXTENTRY_V_PAD); */
    textbox_reset_full(te->tb);
    te->tb->text->validation = validation;
    te->tb->text->completion = completion;
    te->tb->text->max_len = buf_len;
    te->tb->text->completion_target = completion_target;
    return te;
}

void textentry_destroy(TextEntry *te)
{
    textbox_destroy(te->tb);
    free(te);
}

void textentry_draw(TextEntry *te)
{
    textbox_draw(te->tb);    
}

void textentry_reset(TextEntry *te)
{
    textbox_reset_full(te->tb);
}

extern void (*project_draw)(void);
void textentry_edit(TextEntry *te)
{
    txt_edit(te->tb->text, project_draw);
}

void textentry_complete_edit(TextEntry *te)
{
    txt_stop_editing(te->tb->text);
}

/* Toggle */

Toggle *toggle_create_from_endpoint(Layout *lt, Endpoint *ep)
{
    Layout *inner = layout_add_child(lt);
    inner->w.type = SCALE;
    inner->h.type = SCALE;
    inner->w.value = 0.90;
    inner->h.value = 0.90;
    layout_force_reset(lt);
    layout_center_agnostic(inner, true, true);
    /* layout_center_agnostic(outer, true, true); */
    Toggle *tgl = calloc(1, sizeof(Toggle));
    tgl->endpoint = ep;
    tgl->value = ep->val;
    tgl->layout = lt;
    return tgl;

}

Toggle *toggle_create(Layout *lt, bool *value, ComponentFn action, void *target)
{
    Layout *inner = layout_add_child(lt);
    inner->w.type = SCALE;
    inner->h.type = SCALE;
    inner->w.value = 0.90;
    inner->h.value = 0.90;

    /* Layout *outer = layout_add_child(lt); */
    /* outer->w.type = SCALE; */
    /* outer->h.type = SCALE; */
    /* outer->w.value.floatval = 0.9; */
    /* outer->h.value.floatval = 0.9; */
    layout_force_reset(lt);
    layout_center_agnostic(inner, true, true);
    /* layout_center_agnostic(outer, true, true); */
    Toggle *tgl = calloc(1, sizeof(Toggle));
    tgl->value = value;
    tgl->layout = lt;
    tgl->action = action;
    tgl->target = target;
    return tgl;
}

void toggle_destroy(Toggle *tgl)
{
    layout_destroy(tgl->layout);
    free(tgl);
}

void toggle_draw(Toggle *tgl)
{
    /* SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(tgl_bckgrnd)); */
    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(slider_bckgrnd));
    SDL_RenderFillRect(main_win->rend, &tgl->layout->rect);
    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(slider_bar_container_bckgrnd));
    SDL_RenderFillRect(main_win->rend, &tgl->layout->children[0]->rect);
    if (*(tgl->value)) {
	SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(slider_bar_color));
	SDL_RenderFillRect(main_win->rend, &(tgl->layout->children[0]->rect));
    }
}


bool toggle_toggle(Toggle *toggle)
{
    if (toggle->endpoint) {
	bool current = endpoint_safe_read(toggle->endpoint, NULL).bool_v;
	endpoint_write(toggle->endpoint, (Value){.bool_v = !current}, true, true, true, true);
	return !current;
    } else {
	*(toggle->value) = !(*toggle->value);
	if (toggle->action) {
	    toggle->action((void *)toggle, toggle->target);
	}

	return toggle->value;
    }
}



/* Radio Button */
RadioButton *radio_button_create(
    Layout *lt,
    int text_size,
    SDL_Color *text_color,
    Endpoint *ep,
    /* void *target, */
    /* ComponentFn action, */
    const char **item_names,
    uint8_t num_items
    )
{
    RadioButton *rb = calloc(1, sizeof(RadioButton));
    rb->ep = ep;
    /* rb->action = action; */
    /* rb->target = target; */
    rb->num_items = num_items;
    rb->layout = lt;
    /* Layout manipulation */
    for (uint8_t i=0; i<num_items; i++) {
	Layout *item_lt, *l, *r;
	if (i==0) {
	    item_lt = layout_add_child(lt);
	    item_lt->h.value = RADIO_BUTTON_ITEM_H;
	    item_lt->w.type = SCALE;
	    item_lt->w.value = 1.0f;
	    item_lt->y.type = STACK;
	    l = layout_add_child(item_lt);
	    r = layout_add_child(item_lt);
	    l->h.type = SCALE;
	    l->h.value = 1.0f;
	    r->h.type = SCALE;
	    r->h.value = 1.0f;
	    l->w.value = RADIO_BUTTON_LEFT_W;
	    r->x.type = STACK;
	    r->w.type = COMPLEMENT;
	} else {
	    item_lt = layout_copy(lt->children[0], lt);
	    l = item_lt->children[0];
	    r = item_lt->children[1];
	}
	layout_force_reset(item_lt);
	Textbox *label = textbox_create_from_str((char *)item_names[i], r, main_win->mono_bold_font, 14, main_win);
	textbox_set_align(label, CENTER_LEFT);
	textbox_set_pad(label, 4, 0);
	textbox_set_text_color(label, text_color);
	textbox_set_background_color(label, NULL);
	textbox_reset_full(label);
	rb->items[i] = label;
    }
    /* Layout *left_col = layout_add_child(lt); */
    /* left_col->h.type = SCALE; */
    /* left_col->h.value.floatval = 1.0f; */
    /* left_col->w.value.intval = RADIO_BUTTON_LEFT_COL_W; */

    /* Layout *item_container = layout_add_child(lt); */
    /* item_container->h.type = SCALE; */
    /* item_container->h.value.floatval = 1.0f; */
    /* item_container->x.type = STACK; */
    /* item_container->w.type = COMPLEMENT; */

    /* layout_force_reset(lt); */
    
    return rb;
}

void radio_button_reset_from_endpoint(RadioButton *rb)
{
    /* if (!rb->target) return; */
    Value val = endpoint_safe_read(rb->ep, NULL);
    rb->selected_item = val.int_v;
    if (rb->selected_item > rb->num_items) {
	rb->selected_item = 0;
	fprintf(stderr, "Error: unable to set radio button from target valu");
    }
}


void radio_button_draw(RadioButton *rb)
{
    for (uint8_t i=0; i<rb->num_items; i++) {
	textbox_draw(rb->items[i]);
	Layout *circle_container = rb->layout->children[i]->children[0];
	int r = (circle_container->rect.w >> 1) - RADIO_BUTTON_RAD_PAD;
	int orig_x = circle_container->rect.x + RADIO_BUTTON_RAD_PAD;
	int orig_y = circle_container->rect.y + RADIO_BUTTON_RAD_PAD;
	SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(slider_bar_container_bckgrnd));
	geom_fill_circle(main_win->rend, orig_x, orig_y, r);
	SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(colors.grey));
	geom_draw_circle(main_win->rend, orig_x, orig_y, r);
	if (i==rb->selected_item) {
	    r -= RADIO_BUTTON_RAD_PAD;
	    orig_x += RADIO_BUTTON_RAD_PAD;
	    orig_y += RADIO_BUTTON_RAD_PAD;
	    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(slider_bar_color));
	    geom_fill_circle(main_win->rend, orig_x, orig_y, r);
	}
    }
}

void radio_cycle_back(RadioButton *rb)
{
    if (rb->selected_item > 0)
	rb->selected_item--;
    else
	rb->selected_item = rb->num_items - 1;

    if (rb->ep)
	endpoint_write(rb->ep, (Value){.int_v = rb->selected_item}, true, true, true, true);
    /* if (rb->action) rb->action((void *)rb, rb->target); */
}

void radio_cycle(RadioButton *rb)
{
    /* if (rb->selected_item == rb->num_items - 1) return; */
    rb->selected_item++;
    rb->selected_item %= rb->num_items;

    if (rb->ep)
	endpoint_write(rb->ep, (Value){.int_v = rb->selected_item}, true, true, true, true);
    /* if (rb->action) rb->action((void *)rb, rb->target); */
}

bool radio_click(RadioButton *rb, Window *Win)
{
    if (SDL_PointInRect(&main_win->mousep, &rb->layout->rect)) {
	for (uint8_t i = 0; i<rb->num_items; i++) {
	    if (SDL_PointInRect(&main_win->mousep, &(rb->layout->children[i]->rect))) {
		rb->selected_item = i;
		if (rb->ep)
		    endpoint_write(rb->ep, (Value){.int_v = rb->selected_item}, true, true, true, true);
		/* if (rb->action) { */
		/*     rb->action((void *)rb, rb->target); */
		/* } */
		return true;
	    }
	}
    }
    return false;
}


void radio_destroy(RadioButton *rb)
{
    for (uint8_t i=0; i<rb->num_items; i++) {
	textbox_destroy(rb->items[i]);
    }
    layout_destroy(rb->layout);
    if (rb->dynamic_text) free(rb->dynamic_text);
    free(rb);

}


/* Symbol Radio button */

SymbolRadio *symbol_radio_create(
    Layout *lt,
    Symbol **symbols,
    uint8_t num_items,
    Endpoint *ep,
    bool align_horizontal,
    int padding,
    SDL_Color *sel_color,
    SDL_Color *unsel_color)
{
    SymbolRadio *sr = calloc(1, sizeof(SymbolRadio));
    sr->ep = ep;
    sr->num_items = num_items;
    sr->layout = lt;
    sr->ep = ep;
    sr->sel_color = sel_color;
    sr->unsel_color = unsel_color;
    memcpy(sr->symbols, symbols, num_items * sizeof(Symbol *));

    for (int i=0; i<num_items; i++) {
	Layout *item_lt = layout_add_child(lt);
	if (align_horizontal) {
	    item_lt->x.type = STACK;
	    item_lt->y.type = REL;
	    item_lt->x.value = padding;
	} else {
	    item_lt->x.type = REL;
	    item_lt->y.type = STACK;
	    item_lt->y.value = padding;
	}
	item_lt->w.value = symbols[i]->x_dim_pix / main_win->dpi_scale_factor;
	item_lt->h.value = symbols[i]->y_dim_pix / main_win->dpi_scale_factor;	
    }
    return sr;

}

void symbol_radio_reset_from_endpoint(SymbolRadio *sr)
{
    Value val = endpoint_safe_read(sr->ep, NULL);
    sr->selected_item = val.int_v;
}

void symbol_radio_draw(SymbolRadio *sr)
{
    for (uint8_t i=0; i<sr->num_items; i++) {
	Layout *lt = sr->layout->children[i];

	if (i==sr->selected_item)
	    symbol_draw_w_bckgrnd(sr->symbols[i], &lt->rect, sr->sel_color);
	else
	    symbol_draw_w_bckgrnd(sr->symbols[i], &lt->rect, sr->unsel_color);
    }
}

void symbol_radio_cycle_back(SymbolRadio *sr)
{
    if (sr->selected_item > 0)
	sr->selected_item--;
    else
	sr->selected_item = sr->num_items - 1;
    endpoint_write(sr->ep, (Value){.int_v = sr->selected_item}, true, true, true, true);
}

void symbol_radio_cycle(SymbolRadio *sr)
{
    /* if (sr->selected_item == sr->num_items - 1) return; */
    sr->selected_item++;
    sr->selected_item %= sr->num_items;
    endpoint_write(sr->ep, (Value){.int_v = sr->selected_item}, true, true, true, true);
}

bool symbol_radio_click(SymbolRadio *sr, Window *Win)
{
    if (SDL_PointInRect(&main_win->mousep, &sr->layout->rect)) {
	for (uint8_t i = 0; i<sr->num_items; i++) {
	    if (SDL_PointInRect(&main_win->mousep, &(sr->layout->children[i]->rect))) {
		sr->selected_item = i;
		endpoint_write(sr->ep, (Value){.int_v = sr->selected_item}, true, true, true, true);
		return true;
	    }
	}
    }
    return false;
}

void symbol_radio_destroy(SymbolRadio *sr)
{
    /* layout_destroy(sr->layout); */
    free(sr);
}





/* Waveform */

Waveform *waveform_create(
    Layout *lt,
    void **channels,
    ValType type,
    uint8_t num_channels,
    uint32_t len,
    SDL_Color *background_color,
    SDL_Color *plot_color)
{
    Waveform *w = calloc(1, sizeof(Waveform));
    w->layout = lt;
    w->channels = calloc(num_channels, sizeof(void *));
    for (uint8_t i=0; i<num_channels; i++) {
	w->channels[i] = channels[i];
    }
    w->type = type;
    w->num_channels = num_channels;
    w->len = len;
    w->background_color = background_color;
    w->plot_color = plot_color;
    return w;
}

void waveform_destroy(Waveform *w)
{
    free(w->channels);
    layout_destroy(w->layout);
    free(w);
}

void waveform_draw(Waveform *w)
{
    SDL_SetRenderDrawColor(main_win->rend, sdl_colorp_expand(w->background_color));
    SDL_RenderFillRect(main_win->rend, &w->layout->rect);
    SDL_SetRenderDrawColor(main_win->rend, sdl_colorp_expand(w->plot_color));
    waveform_draw_all_channels_generic(w->channels, w->type, w->num_channels, w->len, &w->layout->rect, 0, main_win->w_pix);
}

/* Canvas */

Canvas *canvas_create(Layout *lt, void (*draw_fn)(void *, void *), void *draw_arg1, void *draw_arg2)
{
    Canvas *c = calloc(1, sizeof(Canvas));
    c->layout = lt;
    c->draw_fn = draw_fn;
    c->draw_arg1 = draw_arg1;
    c->draw_arg2 = draw_arg2;
    return c;
}

void canvas_draw(Canvas *c)
{
    c->draw_fn(c->draw_arg1, c->draw_arg2);
}

void canvas_destroy(Canvas *c)
{
    free(c);
}


Dropdown *dropdown_create(
    Layout *lt,
    const char *header,
    char **item_names,
    char **item_annotations,
    void **item_args,
    uint8_t num_items,
    int *reset_from,
    /* bool free_args_on_destroy, */
    int (*selection_fn)(Dropdown *self, void *arg))
{
    Dropdown *d = calloc(1, sizeof(Dropdown));
    d->layout = lt;
    if (!item_names) {
	fprintf(stderr, "Error: dropdown_create: item_names cannot be null\n");
	free(d);
	return NULL;
    }
    d->reset_from = reset_from;
    d->item_names = calloc(num_items, sizeof(char *));
    memcpy(d->item_names, item_names, num_items * sizeof(char *));
    if (item_annotations) {
	d->item_annotations = calloc(num_items, sizeof(char *));
	memcpy(d->item_annotations, item_annotations, num_items * sizeof(char *));
    }
    if (item_args) {
	d->item_args = calloc(num_items, sizeof(void *));
	memcpy(d->item_args, item_args, num_items * sizeof(void *));
    }
    d->num_items = num_items;
    /* d->free_args_on_destroy = free_args_on_destroy; */
    d->selection_fn = selection_fn;
    d->tb = textbox_create_from_str(
	item_names[0],
	lt,
	main_win->std_font,
	12,
	main_win);
    textbox_set_trunc(d->tb, false);
    textbox_set_style(d->tb, BUTTON_DARK);
    textbox_reset_full(d->tb);
    return d;
}

void dropdown_reset(Dropdown *d)
{
    if (d->reset_from)
	d->selected_item = *d->reset_from;
    textbox_set_value_handle(d->tb, d->item_names[d->selected_item]);
}

void dropdown_draw(Dropdown *d)
{
    textbox_draw(d->tb);
}

void dropdown_destroy(Dropdown *d)
{
    free(d->item_names);
    if (d->item_annotations) free(d->item_annotations);
    if (d->item_args) free(d->item_args);
    free(d);
}

struct dropdown_menu_item_arg {
    Dropdown *d;
    void *inner_arg;
    int index;
};

static void dropdown_item_onclick(void *arg_v)
{
    struct dropdown_menu_item_arg *arg = arg_v;
    Dropdown *d = arg->d;
    int ret = 0;
    if (d->selection_fn) {
        ret = d->selection_fn(d, arg->inner_arg);
    }
    if (ret < 0) {
	fprintf(stderr, "Dropdown: invalid\n");
    } else { /* Allow textbox reset to happen in reset fn */
	
	/* d->selected_item = arg->index; */
	/* textbox_set_value_handle(d->tb, d->item_names[arg->index]); */
	/* textbox_reset_full(d->tb); */
    }
    window_pop_menu(main_win);
}


void dropdown_create_menu(Dropdown *d)
{
    Menu *menu = menu_create_at_point(d->layout->rect.x, d->layout->rect.y);
    MenuColumn *c = menu_column_add(menu, "");
    MenuSection *s = menu_section_add(c, "");
    for (int i=0; i<d->num_items; i++) {
	struct dropdown_menu_item_arg *arg = malloc(sizeof(struct dropdown_menu_item_arg));
	arg->d = d;
	arg->inner_arg = d->item_args ? d->item_args[i] : NULL;
	arg->index = i;
	MenuItem *item = menu_item_add(
	    s,
	    d->item_names[i],
	    d->item_annotations ? d->item_annotations[i] : NULL,
	    dropdown_item_onclick,
	    arg);	
	item->free_target_on_destroy = true;
	/* item->free_target_on_destroy = d->free_args_on_destroy; */
    }
    if (d->header) menu_add_header(menu, d->header, d->description);
    window_add_menu(main_win, menu);
    
    /* MenuSubcat *sc = menu_add_subcat(menu, ""); */
}

bool dropdown_click(Dropdown *d, Window *win)
{
    if (SDL_PointInRect(&win->mousep, &d->layout->rect)) {
	dropdown_create_menu(d);
	return true;
    } else {
	return false;
    }
}

/* Status light */

StatusLight *status_light_create(Layout *lt, void *value, size_t val_size)
{
    StatusLight *sl = calloc(1, sizeof(StatusLight));
    sl->layout = lt;
    sl->value = value;
    sl->val_size = val_size;
    Layout *inner = layout_add_child(lt);
    inner->rect.w = SYMBOL_TABLE[SYMBOL_STATUS_ON]->x_dim_pix;
    inner->rect.h = inner->rect.w;
    layout_set_values_from_rect(inner);
    layout_center_agnostic(inner, true, true);
    return sl;
    
}

void status_light_draw(StatusLight *sl)
{
    char zero[sl->val_size];
    memset(zero, '\0', sl->val_size * sizeof(char));
    if (memcmp(sl->value, zero, sl->val_size) == 0) {
	symbol_draw(SYMBOL_TABLE[SYMBOL_STATUS_OFF], &sl->layout->children[0]->rect);
    } else {
	symbol_draw(SYMBOL_TABLE[SYMBOL_STATUS_ON], &sl->layout->children[0]->rect);
    }
}

void status_light_destroy(StatusLight *sl)
{
    free(sl);
}


/* Mouse functions */

typedef struct click_segment ClickSegment;
bool click_track_mouse_motion(ClickSegment *s, Window *win);
void tabview_tab_drag(TabView *tv, Window *win);
bool draggable_mouse_motion(Draggable *draggable, Window *win)
{
    switch (draggable->type) {
    case DRAG_SLIDER:
	return slider_mouse_motion((Slider *)draggable->component, win);
	break;
    case DRAG_CLICK_SEG_BOUND:
	return click_track_mouse_motion((ClickSegment *)draggable->component, win);
    case DRAG_EQ_FILTER_NODE:
	eq_mouse_motion((EQFilterCtrl *)draggable->component, win);
	return true;
    case DRAG_TABVIEW_TAB:
	tabview_tab_drag((TabView *)draggable->component, win);
	return true;
    }
    return false;
}


bool toggle_click(Toggle *toggle, Window *win)
{
    if (SDL_PointInRect(&main_win->mousep, &toggle->layout->rect)) {
	toggle_toggle(toggle);
	/* if (toggle->action) { */
	/*     toggle->action((void *)toggle, toggle->target); */
	/* } */
	return true;
    }
    return false;
}

bool button_click(Button *button, Window *win)
{
    if (SDL_PointInRect(&main_win->mousep, &button->tb->layout->rect)) {
	if (button->action) {
	    button->action((void *)button, button->target);
	}
	if (button->bound_userfn) {
	    button_press_color_change(
		button,
		button->pressed_color,
		button->return_color);

	}
	return true;
    }
    return false;
}

bool symbol_button_click(SymbolButton *sbutton, Window *win)
{
    if (SDL_PointInRect(&main_win->mousep, &sbutton->layout->rect)) {
	if (sbutton->action)
	    sbutton->action((void *)sbutton, sbutton->target);
	return true;
    }
    return false;
}


void draggable_handle_scroll(Draggable *d, int x, int y)
{

/*     switch (d->type) { */
/*     case DRAG_SLIDER: { */
/* 	Slider *s = (Slider *)d->component; */
/* 	if (abs(x) > abs(y) && s->orientation == SLIDER_HORIZONTAL) { */
/* 	    slider_scroll(s, x); */
/* 	} else { */
/* 	    if (abs(x) > abs(y)) */
/* 		slider_scroll(s, x); */
/* 	    else */
/* 		slider_scroll(s, -1 * y); */
/* 	} */
/*     } */
/* 	break; */
/*     default: */
/* 	break; */
/*     } */
}
