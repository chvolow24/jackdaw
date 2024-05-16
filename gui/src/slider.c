#include "color.h"
#include "slider.h"
#include "layout.h"
#include "text.h"
#include "textbox.h"

extern Window *main_win;
#define SLIDER_INNER_PAD 2
#define SLIDER_TICK_W 2

#define SLIDER_LABEL_H_PAD 4
#define SLIDER_LABEL_V_PAD 2

/* SDL_Color fslider_bckgrnd = {60, 60, 60, 255}; */
/* SDL_Color fslider_bar_container_bckgrnd =  {190, 190, 190, 255}; */
/* SDL_Color fslider_bar_color = {20, 20, 120, 255}; */

SDL_Color fslider_bckgrnd = {60, 60, 60, 255};
SDL_Color fslider_bar_container_bckgrnd =  {40, 40, 40, 248};
SDL_Color fslider_bar_color = {12, 107, 249, 250};

extern SDL_Color color_global_black;

FSlider *fslider_create(float *value, Layout *layout, SliderOrientation orientation, SliderType type, SliderStrFn *fn)
{
    FSlider *fs = calloc(1, sizeof(FSlider));
    fs->create_label = fn;
    fs->layout = layout;
    Layout *bar_container = layout_add_child(layout);
    layout_set_name(bar_container, "bar_container");
    Layout *label = layout_add_child(layout);
    label->rect.x = layout->rect.x + layout->rect.w;
    label->rect.y = layout->rect.y;
    layout_set_values_from_rect(label);
    fn(fs->label_str, SLIDER_LABEL_STRBUFLEN - 1, *value);
    /* amp_to_dbstr(fs->label_str, SLIDER_LABEL_STRBUFLEN - 1, *value); */

    /* snprintf(fs->label_str, SLIDER_LABEL_STRBUFLEN - 1, "%f", *value); */
    fs->label = textbox_create_from_str(fs->label_str, label, main_win->std_font, 10, main_win);
    textbox_size_to_fit(fs->label, SLIDER_LABEL_H_PAD, SLIDER_LABEL_V_PAD);
    textbox_set_border(fs->label, &color_global_black, 2);
    layout_reset(layout);
    /* bar_container->x.value.intval = SLIDER_INNER_PAD; */
    /* bar_container->y.value.intval = SLIDER_INNER_PAD; */
    /* bar_container->w.value.intval = (layout->rect.w - (SLIDER_INNER_PAD * main_win->dpi_scale_factor * 2)) / main_win->dpi_scale_factor; */
    /* bar_container->h.value.intval = (layout->rect.h - (SLIDER_INNER_PAD * main_win->dpi_scale_factor * 2)) / main_win->dpi_scale_factor; */
    layout_pad(bar_container, SLIDER_INNER_PAD, SLIDER_INNER_PAD);
    Layout *bar = layout_add_child(bar_container);
    layout_set_name(bar, "bar");
    fs->bar_rect = &bar->rect;
    switch (orientation) {
    case SLIDER_HORIZONTAL:
	bar->y.value.intval = 0;
	bar->h.type = SCALE;
	bar->h.value.floatval = 1.0f;
	switch (type) {
	case SLIDER_FILL:
	    bar->w.type = SCALE;
	    fs->val_dim = &bar->w.value;
	    break;
	case SLIDER_TICK:
	    bar->x.type = SCALE;
	    fs->val_dim = &bar->x.value;
	    bar->w.type = ABS;
	    bar->w.value.intval = SLIDER_TICK_W;
	    break;   
	}
	break;
    case SLIDER_VERTICAL:
	bar->x.value.intval = 0;
	bar->w.type = SCALE;
	bar->w.value.floatval = 1.0f;
	switch (type) {
	case SLIDER_FILL:
	    bar->h.type = SCALE;
	    fs->val_dim = &bar->h.value;
	    break;
	case SLIDER_TICK:
	    bar->x.type = SCALE;
	    fs->val_dim = &bar->x.value;
	    break;   
	}
    }

    fs->min = 0;
    fs->max = 1;
    fs->value = value;
    fs->orientation = orientation;
    fs->type = type;
    return fs;
}

void fslider_set_range(FSlider *fs, float min, float max)
{
    fs->min = min;
    fs->max = max;

}



void layout_write(FILE *f, Layout *lt, int indent);
void fslider_reset(FSlider *fs)
{
    float range = fs->max - fs->min;
    switch (fs->type) {
    case SLIDER_FILL:
	switch (fs->orientation) {
	case SLIDER_HORIZONTAL:
	    fs->val_dim->floatval = *(fs->value) / range;
	    break;
	case SLIDER_VERTICAL:
	    fs->val_dim->floatval = 1 - *(fs->value) / range;
	}
	break;
    case SLIDER_TICK:
	switch (fs->orientation) {
	case SLIDER_HORIZONTAL:
	    fs->val_dim->floatval = *(fs->value) / range;
	    break;
	case SLIDER_VERTICAL:
	    fs->val_dim->floatval = 1 - *(fs->value) / range;
	    break;
	}
    }
    if (fs->editing) {
	fs->create_label(fs->label_str, SLIDER_LABEL_STRBUFLEN - 1, *(fs->value));
	/* amp_to_dbstr(fs->label_str, SLIDER_LABEL_STRBUFLEN - 1, *(fs->value)); */
	/* snprintf(fs->label_str, SLIDER_LABEL_STRBUFLEN - 1, "%f", *(fs->value)); */
	textbox_reset_full(fs->label);
    }
    /* layout_write(stdout, fs->layout, 0); */
    layout_reset(fs->layout);
    
    /* fprintf(stdout, "Bar rect: %d %d %d %d\n", fs->bar_rect->x, fs->bar_rect->y, fs->bar_rect->w, fs->bar_rect->h); */
}    


void fslider_draw(FSlider *fs)
{
    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(fslider_bckgrnd));
    SDL_RenderFillRect(main_win->rend, &fs->layout->rect);


    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(fslider_bar_container_bckgrnd));
    SDL_RenderFillRect(main_win->rend, &fs->layout->children[0]->rect);
    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(fslider_bar_color));
    SDL_RenderFillRect(main_win->rend, fs->bar_rect);

    /* fprintf(stdout, "DRAWING textbox label w disp val: %s\n", fs->label->text->display_value); */
    /* fprintf(stdout, "Str? %s\n", fs->label_str); */
    if (fs->editing) {
	textbox_draw(fs->label);
    }

}

