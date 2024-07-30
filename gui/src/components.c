#include "color.h"
#include "components.h"
#include "geometry.h"
#include "input.h"
#include "layout.h"
#include "text.h"
#include "textbox.h"
#include "value.h"

extern Window *main_win;
#define SLIDER_INNER_PAD 2
#define SLIDER_TICK_W 2

#define SLIDER_LABEL_H_PAD 4
#define SLIDER_LABEL_V_PAD 2
#define RADIO_BUTTON_LEFT_COL_W 10

/* SDL_Color fslider_bckgrnd = {60, 60, 60, 255}; */
/* SDL_Color fslider_bar_container_bckgrnd =  {190, 190, 190, 255}; */
/* SDL_Color fslider_bar_color = {20, 20, 120, 255}; */

SDL_Color slider_bckgrnd = {60, 60, 60, 255};
SDL_Color slider_bar_container_bckgrnd =  {40, 40, 40, 248};
SDL_Color slider_bar_color = {12, 107, 249, 250};

SDL_Color tgl_bckgrnd = {110, 110, 110, 255};

extern SDL_Color color_global_black;
extern SDL_Color color_global_clear;

/* Slider fslider_create(Layout *layout, SliderOrientation orientation, SliderType type, SliderStrFn *fn) */
Slider *slider_create(
    Layout *layout,
    void *value,
    ValType val_type,
    enum slider_orientation orientation,
    enum slider_style style,
    SliderStrFn *fn)

{
    Slider *s = calloc(1, sizeof(Slider));
    s->create_label = fn;
    s->layout = layout;
    Layout *bar_container = layout_add_child(layout);
    layout_set_name(bar_container, "bar_container");
    Layout *label = layout_add_child(layout);
    label->rect.x = layout->rect.x + layout->rect.w;
    label->rect.y = layout->rect.y;
    layout_set_values_from_rect(label);
    if (fn) {
	fn(s->label_str, SLIDER_LABEL_STRBUFLEN - 1, value, val_type);
    }
    /* amp_to_dbstr(s->label_str, SLIDER_LABEL_STRBUFLEN - 1, *value); */

    /* snprintf(s->label_str, SLIDER_LABEL_STRBUFLEN - 1, "%f", *value); */
    s->label = textbox_create_from_str(s->label_str, label, main_win->std_font, 10, main_win);
    textbox_size_to_fit(s->label, SLIDER_LABEL_H_PAD, SLIDER_LABEL_V_PAD);
    textbox_set_border(s->label, &color_global_black, 2);
    layout_reset(layout);
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
	bar->y.value.intval = 0;
	bar->h.type = SCALE;
	bar->h.value.floatval = 1.0f;
	switch (style) {
	case SLIDER_FILL:
	    bar->w.type = SCALE;
	    s->val_dim = &bar->w.value;
	    break;
	case SLIDER_TICK:
	    bar->x.type = SCALE;
	    s->val_dim = &bar->x.value;
	    bar->w.type = ABS;
	    bar->w.value.intval = SLIDER_TICK_W;
	    break;   
	}
	break;
    case SLIDER_VERTICAL:
	bar->x.value.intval = 0;
	bar->w.type = SCALE;
	bar->w.value.floatval = 1.0f;
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

    jdaw_val_set_min(&(s->min), val_type);
    jdaw_val_set_max(&(s->max), val_type);
    s->val_type = val_type;
    s->value = value;
    s->orientation = orientation;
    s->style = style;
    return s;
}

void slider_set_range(Slider *s, Value min, Value max)
{
    s->min = min;
    s->max = max;
}

Value slider_val_from_coord(Slider *s, int coord_pix)
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
    /* fprintf(stdout, "Proportion: %f\n", proportion); */
    Value range = jdaw_val_sub(s->max, s->min, s->val_type);
    Value val_proportion = jdaw_val_scale(range, proportion, s->val_type);
    return jdaw_val_add(val_proportion, s->min, s->val_type);

}


void layout_write(FILE *f, Layout *lt, int indent);
void slider_reset(Slider *s)
{
    Value range = jdaw_val_sub(s->max, s->min, s->val_type);
    Value slider_val;
    /* Value slider_val = jdaw_val_from_ptr(&slider_val, s->val_type); */
    jdaw_val_set(&slider_val, s->val_type, s->value);
    Value filled = jdaw_val_sub(slider_val, s->min, s->val_type);
    double filled_prop = jdaw_val_div_double(filled, range, s->val_type);
    switch (s->style) {
    case SLIDER_FILL:
	switch (s->orientation) {
	case SLIDER_HORIZONTAL:
	    s->val_dim->floatval = filled_prop;
	    break;
	case SLIDER_VERTICAL:
	    s->bar_layout->y.value.floatval = 1.0 - filled_prop;
	    s->val_dim->floatval = filled_prop;
	    break;
	}
	break;
    case SLIDER_TICK:
	switch (s->orientation) {
	case SLIDER_HORIZONTAL:
	    s->val_dim->floatval = filled_prop;
	    break;
	case SLIDER_VERTICAL:
	    
	    /* layout_reset(s->layout->children[0]); */
	    s->val_dim->floatval = 1 - filled_prop;
	    break;
	}
    }
    if (s->editing) {
	s->create_label(s->label_str, SLIDER_LABEL_STRBUFLEN - 1, s->value, s->val_type);
	/* amp_to_dbstr(s->label_str, SLIDER_LABEL_STRBUFLEN - 1, *(s->value)); */
	/* snprintf(s->label_str, SLIDER_LABEL_STRBUFLEN - 1, "%f", *(s->value)); */
	textbox_reset_full(s->label);
    }
    /* layout_write(stdout, s->layout, 0); */
    layout_reset(s->layout);
    
    /* fprintf(stdout, "Bar rect: %d %d %d %d\n", s->bar_rect->x, s->bar_rect->y, s->bar_rect->w, s->bar_rect->h); */
}    


void slider_draw(Slider *s)
{
    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(slider_bckgrnd));
    SDL_RenderFillRect(main_win->rend, &s->layout->rect);


    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(slider_bar_container_bckgrnd));
    SDL_RenderFillRect(main_win->rend, &s->layout->children[0]->rect);
    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(slider_bar_color));
    SDL_RenderFillRect(main_win->rend, s->bar_rect);

    /* fprintf(stdout, "DRAWING textbox label w disp val: %s\n", s->label->text->display_value); */
    /* fprintf(stdout, "Str? %s\n", s->label_str); */
    if (s->editing) {
	textbox_draw(s->label);
    }
}

void slider_destroy(Slider *s)
{
    textbox_destroy(s->label);
    layout_destroy(s->layout);
    free(s);
}


/* Button */


Button *button_create(Layout *lt, char *text, void *(*action)(void *arg), SDL_Color *text_color, SDL_Color *background_color)
{
    Button *button = calloc(1, sizeof(Button));
    button->action = action;
    button->tb = textbox_create_from_str(text, lt, main_win->bold_font, 12, main_win);
    button->tb->corner_radius = BUTTON_CORNER_RADIUS;
    textbox_set_border(button->tb, text_color, 1);
    textbox_set_background_color(button->tb, background_color);
    textbox_size_to_fit(button->tb, 16, 2);
    return button;
}

void button_draw(Button *button)
{
    textbox_draw(button->tb);
}

void button_destroy(Button *button)
{
    textbox_destroy(button->tb);
    free(button);
}


/* Toggle */

Toggle *toggle_create(Layout *lt, bool *value)
{
    Layout *inner = layout_add_child(lt);
    inner->w.type = SCALE;
    inner->h.type = SCALE;
    inner->w.value.floatval = 0.90;
    inner->h.value.floatval = 0.90;

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
    *(toggle->value) = !(*toggle->value);
    return toggle->value;
}

/* Radio Button */
RadioButton *radio_button_create(
    Layout *lt,
    int text_size,
    SDL_Color *text_color,
    void *target,
    void (*external_action)(int selected_i, void *target),
    const char **item_names,
    uint8_t num_items
    )
{
    RadioButton *rb = calloc(1, sizeof(RadioButton));
    rb->external_action = external_action;
    rb->target = target;
    rb->num_items = num_items;
    rb->layout = lt;
    /* Layout manipulation */
    for (uint8_t i=0; i<num_items; i++) {
	Layout *item_lt, *l, *r;
	if (i==0) {
	    item_lt = layout_add_child(lt);
	    item_lt->h.value.intval = RADIO_BUTTON_ITEM_H;
	    item_lt->w.type = SCALE;
	    item_lt->w.value.floatval = 1.0f;
	    item_lt->y.type = STACK;
	    l = layout_add_child(item_lt);
	    r = layout_add_child(item_lt);
	    l->h.type = SCALE;
	    l->h.value.floatval = 1.0f;
	    r->h.type = SCALE;
	    r->h.value.floatval = 1.0f;
	    l->w.value.intval = RADIO_BUTTON_LEFT_W;
	    r->x.type = STACK;
	    r->w.type = COMPLEMENT;
	} else {
	    item_lt = layout_copy(lt->children[0], lt);
	    l = item_lt->children[0];
	    r = item_lt->children[1];
	}
	layout_force_reset(item_lt);
	Textbox *label = textbox_create_from_str((char *)item_names[i], r, main_win->std_font, 14, main_win);
	textbox_set_align(label, CENTER_LEFT);
	textbox_set_pad(label, 4, 0);
	textbox_set_background_color(label, &color_global_clear);
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
	if (i==rb->selected_item) {
	    r -= RADIO_BUTTON_RAD_PAD;
	    orig_x += RADIO_BUTTON_RAD_PAD;
	    orig_y += RADIO_BUTTON_RAD_PAD;
	    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(slider_bar_color));
	    geom_fill_circle(main_win->rend, orig_x, orig_y, r);
	}
    }
}


/* Mouse functions */

bool slider_mouse_motion(Slider *slider, Window *win)
{
    if (SDL_PointInRect(&main_win->mousep, &slider->layout->rect) && win->i_state & I_STATE_MOUSE_L) {
	int dim = slider->orientation == SLIDER_VERTICAL ? main_win->mousep.y : main_win->mousep.x;
	Value newval = slider_val_from_coord(slider, dim);
	jdaw_val_set_ptr(slider->value, slider->val_type, newval);
	/* track->vol = newval.float_v; */
	slider_reset(slider);
	/* proj->vol_changing = true; */
	return true;
    }
    return false;
}

bool radio_click(RadioButton *rb, Window *Win)
{
    if (SDL_PointInRect(&main_win->mousep, &rb->layout->rect)) {
	for (uint8_t i = 0; i<rb->num_items; i++) {
	    if (SDL_PointInRect(&main_win->mousep, &(rb->layout->children[i]->rect))) {
		rb->selected_item = i;
		if (rb->external_action) {
		    rb->external_action(i, rb->target);
		}
		return true;
	    }
	}
    }
    return false;
}
