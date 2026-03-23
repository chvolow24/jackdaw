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
    vu->horizontal = horizontal;
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
	textbox_container->w.type = SCALE;
	textbox_container->w.value = 1.0;
	textbox_container->h.type = SCALE;
	textbox_container->h.value = 0.5;

    } else {
	textbox_container->h.type = SCALE;
	textbox_container->h.value = 1.0;
	textbox_container->w.type = SCALE;
	textbox_container->w.value = 0.5;
    }
    layout_reset(vu->layout);
    float amp_max = db_to_amp(db_label_values[0] + 1.0f);
    vu->amp_max = amp_max;
    for (int i=0; i<NUM_DB_LABELS; i++) {
	float amp_raw = db_to_amp(db_label_values[i]);
	Layout *tb_lt = layout_add_child(textbox_container);
	if (horizontal) {
	    tb_lt->w.value = 40;
	    tb_lt->h.type = SCALE;
	    tb_lt->h.value = 1.0;

	} else {
	    tb_lt->w.type = SCALE;
	    tb_lt->w.value = 1.0;
	    tb_lt->h.type = SCALE;
	    tb_lt->h.value = 0.25;
	}
	    
	layout_reset(tb_lt);
	vu->db_labels[i] = textbox_create_from_str(
	    db_label_strs[i],
	    tb_lt,
	    main_win->std_font,
	    12,
	    main_win);
	if (horizontal) {
	    textbox_style(
		vu->db_labels[i],
		BOTTOM_CENTER,
		false,
		NULL,
		&colors.label_text_blue);
	    tb_lt->w.type = SCALE;
	    tb_lt->x.type = SCALE;
	    layout_size_to_fit_children_v(tb_lt, false, 0);
	    float prop = (amp_max - amp_raw) / amp_max;
	    tb_lt->x.type = SCALE;
	    tb_lt->x.value = (1.0 - prop) - tb_lt->w.value / 2.0f;
	    /* if (i==0) { */
	    /* 	layout_reset(tb_lt); */
	    /* 	textbox_container->h.value = (float)tb_lt->rect.h / container->rect.h; */
	    /* 	layout_reset(textbox_container); */
	    /* } */
	} else {
	    textbox_style(
		vu->db_labels[i],
		CENTER_RIGHT,
		false,
		NULL,
		&colors.label_text_blue);
	    tb_lt->h.type = SCALE;
	    tb_lt->y.type = SCALE;
	    layout_size_to_fit_children_v(tb_lt, true, 0);
	    float prop = (amp_max - amp_raw) / amp_max;
	    tb_lt->y.type = SCALE;
	    tb_lt->y.value = prop - tb_lt->h.value / 2.0f;
	}
	layout_reset(tb_lt);
	textbox_reset_full(vu->db_labels[i]);
    }

    if (horizontal) {
	inner_bar->x.value = 0;
	inner_bar->y.type = SCALE;
	inner_bar->y.value = textbox_container->h.value + 0.06;
	inner_bar->h.type = SCALE;
	inner_bar->h.value = 1.0 - inner_bar->y.value - 0.06;
	inner_bar->w.type = SCALE;
	inner_bar->w.value = 1.0;
    } else {
	inner_bar->x.type = SCALE;
	inner_bar->x.value = 0.6;
	inner_bar->w.type = SCALE;
	inner_bar->w.value = 0.4;
	inner_bar->h.type = SCALE;
	inner_bar->h.value = 1.0;
    }

    layout_reset(vu->layout);

    if (horizontal) {
	Layout *green_part = layout_add_child(inner_bar);
	green_part->h.type = SCALE;
	green_part->h.value = 1.0f;
	green_part->w.type = SCALE;
	green_part->w.value = 1.0 - ((amp_max - 1.0f) / amp_max +  0.35 / amp_max);
	green_part->x.type = STACK;
	green_part->x.value = 0;

	Layout *orange_part = layout_copy(green_part, inner_bar);
	orange_part->w.value = 0.35 / amp_max;
	Layout *red_part = layout_copy(orange_part, inner_bar);
	red_part->w.type = REVREL;
	red_part->w.value = 1.0f;

    } else {
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
    for (int i=0; i<NUM_DB_LABELS; i++) {
	textbox_draw(vu->db_labels[i]);
    }
    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(colors.grey));
    SDL_RenderDrawRect(main_win->rend, &vu->layout->children[0]->rect);
    Layout *bar_layout = vu->layout->children[0];


    if (vu->horizontal) {
	int zdb_x_rel = bar_layout->rect.w * (vu->amp_max - 1.0) / vu->amp_max;
	int zdb_x = bar_layout->rect.x + bar_layout->rect.w - zdb_x_rel;
	SDL_RenderDrawLine(main_win->rend, zdb_x, bar_layout->rect.y, zdb_x, bar_layout->rect.y + bar_layout->rect.h - 1);
    } else {
	int zdb_y_rel = bar_layout->rect.h * (vu->amp_max - 1.0) / vu->amp_max;
	int zdb_y = zdb_y_rel + bar_layout->rect.y;
	SDL_RenderDrawLine(main_win->rend, bar_layout->rect.x, zdb_y, bar_layout->rect.x + bar_layout->rect.w - 1, zdb_y);
    }
    int channels = vu->ef_R ? 2 : 1;
    for (int channel=0; channel<channels; channel++) {
	EnvelopeFollower *ef = channel == 0 ? vu->ef_L : vu->ef_R;
	float prop = (vu->amp_max - ef->prev_out) / vu->amp_max;
	if (prop < 0.0f) prop = 0.0f;
	/* fprintf(stderr, "PROP: %f\n", prop); */
	SDL_Rect bar_container = bar_layout->rect;
	if (channels > 1) {
	    if (vu->horizontal) {
		bar_container.h /= 2;
		if (channel == 1) {
		    bar_container.y += bar_container.h;
		}
	    } else {
		bar_container.w /= 2;
		if (channel == 1) {
		    bar_container.x += bar_container.w;
		}		
	    }
	}
	if (vu->horizontal) {
	    /* bar_container.x += prop * bar_container.w; */
	    bar_container.w = (1.0 - prop) * bar_container.w;
	} else {
	    bar_container.y += prop * bar_container.h;
	    bar_container.h -= prop * bar_container.h;
	}
	/* SDL_SetRenderDrawColor(main_win->rend, 255, 0, 0, 255); */
	/* SDL_RenderFillRect(main_win->rend, &bar_container); */

	SDL_Rect to_draw;
	int red_index = vu->horizontal ? 2 : 0;
	int orange_index = 1;
	int green_index = vu->horizontal ? 0 : 2;
	if (SDL_IntersectRect(&bar_container, &bar_layout->children[red_index]->rect, &to_draw)) {
	    SDL_SetRenderDrawColor(main_win->rend, 255, 0, 0, 210);
	    SDL_RenderFillRect(main_win->rend, &to_draw);
	}

	if (SDL_IntersectRect(&bar_container, &bar_layout->children[orange_index]->rect, &to_draw)) {
	    if (vu->horizontal) {
		int y = to_draw.y;
		int h = to_draw.h;
		int grad_w = bar_layout->children[orange_index]->rect.w;
		for (int i=1; i<=to_draw.w; i++) {
		    if (i <= grad_w / 2) {
			float prop_r = 2.0f * (float)i / grad_w;
			SDL_SetRenderDrawColor(main_win->rend, 255 * prop_r, 255, 0, 210);
		    } else {
			float prop_g = 1.0f - (2.0 * (float)(i - (float)grad_w / 2) / grad_w);
			SDL_SetRenderDrawColor(main_win->rend, 255, 255 * prop_g, 0, 210);
		    }
		    int x = to_draw.x + i;
		    SDL_RenderDrawLine(main_win->rend, x, y, x, y + h - 1);
		}
	    } else {
		int x = to_draw.x;
		int w = to_draw.w;
		int grad_h = bar_layout->children[orange_index]->rect.h;
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
	    }
	}

	if (SDL_IntersectRect(&bar_container, &bar_layout->children[green_index]->rect, &to_draw)) {
	    SDL_SetRenderDrawColor(main_win->rend, 0, 255, 0, 210);
	    SDL_RenderFillRect(main_win->rend, &to_draw);
	}
    }
    if (channels > 1) {
	SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(colors.grey));
	if (vu->horizontal) {
	    int y = bar_layout->rect.y + bar_layout->rect.h / 2;
	    SDL_RenderDrawLine(main_win->rend, bar_layout->rect.x, y, bar_layout->rect.x + bar_layout->rect.w - 1, y);

	} else {
	    int x = bar_layout->rect.x + bar_layout->rect.w / 2;
	    SDL_RenderDrawLine(main_win->rend, x, bar_layout->rect.y + 1, x, bar_layout->rect.y + bar_layout->rect.h - 1);

	}
    }
    /* layout_draw(main_win, vu->layout); */
}
