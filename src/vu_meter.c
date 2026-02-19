#include <stdlib.h>
#include "SDL_rect.h"
#include "color.h"
#include "dsp_utils.h"
#include "vu_meter.h"

#define NUM_DB_LABELS 5

extern struct colors colors;

extern Window *main_win;

void vu_meter_destroy(VUMeter *vu)
{
    for (int i=0; i<NUM_DB_LABELS; i++) {
	textbox_destroy(vu->db_labels[i]);
    }
    free(vu);
}

VUMeter *vu_meter_create(Layout *container, bool horizontal, EnvelopeFollower *ef_L, EnvelopeFollower *ef_R)
{
    VUMeter *vu = calloc(1, sizeof(VUMeter));
    vu->layout = container;
    vu->ef_L = ef_L;
    vu->ef_R = ef_R;
    Layout *inner_bar = layout_add_child(container);
    Layout *textbox_container = layout_add_child(container);
    
    static const float db_label_values[NUM_DB_LABELS] = {
        0.0f, -3.0f, -6.0f, -10.0f, -20.0f
    };
    static const char *db_label_strs[NUM_DB_LABELS] = {
        "0 dB", "-3", "-6", "-10", "-20"
    };
    
    if (horizontal) {
	/* TODO: horizontal vu meter */
    } else {
	textbox_container->h.type = SCALE;
	textbox_container->h.value = 1.0;
	textbox_container->w.type = SCALE;
	textbox_container->w.value = 0.5;
	layout_reset(vu->layout);
	float amp_max = db_to_amp(db_label_values[0] + 1.0f);
	vu->amp_max = amp_max;
	for (int i=0; i<NUM_DB_LABELS; i++) {
	    float amp_raw = db_to_amp(db_label_values[i]);
	    Layout *tb_lt = layout_add_child(textbox_container);
	    tb_lt->w.type = SCALE;
	    tb_lt->w.value = 1.0;
	    tb_lt->h.type = SCALE;
	    tb_lt->h.value = 0.25;
	    
	    layout_reset(tb_lt);
	    vu->db_labels[i] = textbox_create_from_str(
		db_label_strs[i],
		tb_lt,
		main_win->std_font,
		12,
		main_win);
	    textbox_style(
		vu->db_labels[i],
		CENTER_RIGHT,
		false,
		NULL,
		&colors.label_text_blue);
	    tb_lt->h.type = SCALE;
	    tb_lt->y.type = SCALE;
	    layout_size_to_fit_children_v(tb_lt, true, 0);
	    /* layout_set_values_from_rect(tb_lt); */
	    float prop = (amp_max - amp_raw) / amp_max;
	    tb_lt->y.type = SCALE;
	    tb_lt->y.value = prop - tb_lt->h.value / 2.0f;
	    /* tb_lt->rect.y = textbox_container->rect.y + (float)textbox_container->rect.h * prop - (float)tb_lt->rect.h / 2.0f; */
	    

	    /* layout_set_values_from_rect(tb_lt); */
	    layout_reset(tb_lt);
	    /* layout_size_to_fit_children_v(tb_lt, true, 0); */
	    /* layout_reset(vu->layout); */
	    textbox_reset_full(vu->db_labels[i]);
	}

	inner_bar->x.type = SCALE;
	inner_bar->x.value = 0.6;
	inner_bar->w.type = SCALE;
	inner_bar->w.value = 0.4;
	inner_bar->h.type = SCALE;
	inner_bar->h.value = 1.0;

	layout_reset(vu->layout);
	Layout *red_part = layout_add_child(inner_bar);
	red_part->w.type = SCALE;
	red_part->w.value = 1.0f;
	red_part->h.type = SCALE;
	red_part->h.value = (amp_max - 1.0f) / amp_max;
	Layout *orange_part = layout_copy(red_part, inner_bar);
	orange_part->y.type = STACK;
	orange_part->y.value = 0.0f;
	orange_part->h.value = 0.35 / amp_max;

	Layout *green_part = layout_copy(orange_part, inner_bar);
	green_part->h.type = REVREL;
	green_part->h.value = 0;
	
    }
    return vu;
}

void vu_meter_draw(VUMeter *vu)
{
    /* layout_force_reset(vu->layout); */
    /* layout_draw(main_win, vu->layout); */
    /* layout_write_debug(stderr, vu->layout, 0, 20); */
    for (int i=0; i<NUM_DB_LABELS; i++) {
	textbox_draw(vu->db_labels[i]);
	/* SDL_SetRenderDrawColor(main_win->rend, 255, 0, 0, 255); */
	/* SDL_RenderDrawRect(main_win->rend, &vu->db_labels[i]->layout->rect); */
    }
    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(colors.grey));
    /* SDL_RenderDrawRect(main_win->rend, &vu->layout->rect); */
    SDL_RenderDrawRect(main_win->rend, &vu->layout->children[0]->rect);
    Layout *bar_layout = vu->layout->children[0];
    int zdb_y_rel = bar_layout->rect.h * (vu->amp_max - 1.0) / vu->amp_max;
    int zdb_y = zdb_y_rel + bar_layout->rect.y;
    SDL_RenderDrawLine(main_win->rend, bar_layout->rect.x, zdb_y, bar_layout->rect.x + bar_layout->rect.w - 1, zdb_y);
    int channels = vu->ef_R ? 2 : 1;
    for (int channel=0; channel<channels; channel++) {
	EnvelopeFollower *ef = channel == 0 ? vu->ef_L : vu->ef_R;
	float prop = (vu->amp_max - ef->prev_out) / vu->amp_max;
	if (prop < 0.0f) prop = 0.0f;
	/* fprintf(stderr, "PROP: %f\n", prop); */
	SDL_Rect bar_container = bar_layout->rect;
	if (channels > 1) {
	    bar_container.w /= 2;
	    if (channel == 1) {
		bar_container.x += bar_container.w;
	    }
	}
	bar_container.y += prop * bar_container.h;
	bar_container.h -= prop * bar_container.h;
	/* SDL_SetRenderDrawColor(main_win->rend, 255, 0, 0, 255); */
	/* SDL_RenderFillRect(main_win->rend, &bar_container); */

	SDL_Rect to_draw;
    
	if (SDL_IntersectRect(&bar_container, &bar_layout->children[0]->rect, &to_draw)) {
	    SDL_SetRenderDrawColor(main_win->rend, 255, 0, 0, 210);
	    SDL_RenderFillRect(main_win->rend, &to_draw);
	}

	if (SDL_IntersectRect(&bar_container, &bar_layout->children[1]->rect, &to_draw)) {
	    int x = to_draw.x;
	    int w = to_draw.w;
	    int grad_h = bar_layout->children[1]->rect.h;
	    for (int i=1; i<=to_draw.h; i++) {
		if (i <= grad_h / 2) {
		    float prop_r = 2.0f * (float)i / grad_h;
		    SDL_SetRenderDrawColor(main_win->rend, 255 * prop_r, 255, 0, 210);
		} else {
		    float prop_g = 1.0f - (2.0 * (float)(i - (float)grad_h / 2) / grad_h);
		    SDL_SetRenderDrawColor(main_win->rend, 255, 255 * prop_g, 0, 210);
		}
		int y = to_draw.y + to_draw.h - i;
		SDL_RenderDrawLine(main_win->rend, x, y, x + w - 1, y);
	    }
	    /* SDL_SetRenderDrawColor(main_win->rend, 255, 255, 0, 255); */
	    /* SDL_RenderFillRect(main_win->rend, &to_draw); */

	}

	if (SDL_IntersectRect(&bar_container, &bar_layout->children[2]->rect, &to_draw)) {
	    SDL_SetRenderDrawColor(main_win->rend, 0, 255, 0, 210);
	    SDL_RenderFillRect(main_win->rend, &to_draw);
	}
    }
    if (channels > 1) {
	SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(colors.grey));
	int x = bar_layout->rect.x + bar_layout->rect.w / 2;
	SDL_RenderDrawLine(main_win->rend, x, bar_layout->rect.y + 1, x, bar_layout->rect.y + bar_layout->rect.h - 1);
    }
}
