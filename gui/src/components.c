#include "color.h"
#include "components.h"
#include "layout.h"
#include "text.h"
#include "textbox.h"
#include "value.h"

extern Window *main_win;
#define SLIDER_INNER_PAD 2
#define SLIDER_TICK_W 2

#define SLIDER_LABEL_H_PAD 4
#define SLIDER_LABEL_V_PAD 2

/* SDL_Color fslider_bckgrnd = {60, 60, 60, 255}; */
/* SDL_Color fslider_bar_container_bckgrnd =  {190, 190, 190, 255}; */
/* SDL_Color fslider_bar_color = {20, 20, 120, 255}; */

SDL_Color slider_bckgrnd = {60, 60, 60, 255};
SDL_Color slider_bar_container_bckgrnd =  {40, 40, 40, 248};
SDL_Color slider_bar_color = {12, 107, 249, 250};

extern SDL_Color color_global_black;

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
    fn(s->label_str, SLIDER_LABEL_STRBUFLEN - 1, value, val_type);
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
	    s->val_dim = &bar->h.value;
	    break;
	case SLIDER_TICK:
	    bar->x.type = SCALE;
	    s->val_dim = &bar->x.value;
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
	proportion = ((double)coord_pix - s->layout->rect.y) / s->layout->rect.h;
	break;
    case SLIDER_HORIZONTAL:
	proportion = ((double)coord_pix - s->layout->rect.x) / s->layout->rect.w;
	break;
    }
    Value range = jdaw_val_sub(s->max, s->min, s->val_type);
    Value val_proportion = jdaw_val_scale(range, proportion, s->val_type);
    return jdaw_val_add(val_proportion, s->min, s->val_type);

}

void layout_write(FILE *f, Layout *lt, int indent);
void slider_reset(Slider *s)
{
    Value range = jdaw_val_sub(s->max, s->min, s->val_type);
    Value slider_val;
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
	    s->val_dim->floatval = 1 - filled_prop;
	}
	break;
    case SLIDER_TICK:
	switch (s->orientation) {
	case SLIDER_HORIZONTAL:
	    s->val_dim->floatval = filled_prop;
	    break;
	case SLIDER_VERTICAL:
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
