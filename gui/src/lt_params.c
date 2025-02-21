/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "draw.h"
#include "layout.h"
#include "lt_params.h"
#include "openfile.h"
#include "text.h"


extern Layout *param_lt;
extern LTParams *lt_params;
extern OpenFile *openfile;
//extern TTF_Font *open_sans;
extern Window *main_win;

void set_lt_params(Layout *lt)
{
    if (!lt_params) {
        lt_params = malloc(sizeof(LTParams));
        Layout *name_label_layout, *x_label_layout, *y_label_layout, *w_label_layout, *h_label_layout;
        Layout *name_val_layout, *x_typeval_layout, *y_typeval_layout, *w_typeval_layout, *h_typeval_layout;
        Layout *x_value_layout, *y_value_layout, *w_value_layout, *h_value_layout;

        name_label_layout = layout_get_child_by_name_recursive(param_lt, "name_label");
        x_label_layout = layout_get_child_by_name_recursive(param_lt, "x_label");
        y_label_layout = layout_get_child_by_name_recursive(param_lt, "y_label");
        w_label_layout = layout_get_child_by_name_recursive(param_lt, "w_label");
        h_label_layout = layout_get_child_by_name_recursive(param_lt, "h_label");

        name_val_layout = layout_get_child_by_name_recursive(param_lt, "name_value");

        x_typeval_layout = layout_get_child_by_name_recursive(param_lt, "x_type");
        y_typeval_layout = layout_get_child_by_name_recursive(param_lt, "y_type");
        w_typeval_layout = layout_get_child_by_name_recursive(param_lt, "w_type");
        h_typeval_layout = layout_get_child_by_name_recursive(param_lt, "h_type");

        x_value_layout = layout_get_child_by_name_recursive(param_lt, "x_value");
        y_value_layout = layout_get_child_by_name_recursive(param_lt, "y_value");
        w_value_layout = layout_get_child_by_name_recursive(param_lt, "w_value");
        h_value_layout = layout_get_child_by_name_recursive(param_lt, "h_value");


        // fprintf(stderr, "Rect addrs: %p, %p, %p, %p, %p, %p, %p, %p, %p, %p, %p, %p, %p, %p", name_label_rect, name_val_rect, x_label_rect, y_label_rect, w_label_rect, h_label_rect, x_typeval_rect, y_typeval_rect, w_typeval_rect, h_typeval_rect, x_value_rect, y_value_rect, w_value_rect, h_value_rect);
        // exit(0);
        SDL_Color txt_color = {255, 255, 255, 255};
	/* TTF_Font *open_sans_12 = ttf_get_font_at_size(main_win->std_font, 12); */
        lt_params->name_label = txt_create_from_str("Name: ", 7, name_label_layout, main_win->std_font, 12, txt_color, CENTER_LEFT, false, main_win);
        lt_params->x_type_label = txt_create_from_str("X Type: ", 9, x_label_layout, main_win->std_font, 12, txt_color, CENTER_LEFT, false, main_win);
        lt_params->y_type_label = txt_create_from_str("Y Type: ", 9, y_label_layout, main_win->std_font, 12, txt_color, CENTER_LEFT, false, main_win);
        lt_params->w_type_label = txt_create_from_str("W Type: ", 9, w_label_layout, main_win->std_font, 12, txt_color, CENTER_LEFT, false, main_win);
        lt_params->h_type_label = txt_create_from_str("H Type", 9, h_label_layout, main_win->std_font, 12, txt_color, CENTER_LEFT, false, main_win);
        // fprintf(stderr, "Done a bunch text from string\n");
        
        lt_params->name_value = txt_create_from_str(NULL, MAX_LT_NAMELEN - 1, name_val_layout, main_win->std_font, 12, txt_color, CENTER_LEFT, true, main_win);
        lt_params->x_type_value = txt_create_from_str(NULL, 5, x_typeval_layout, main_win->std_font, 12, txt_color, CENTER_LEFT, false, main_win);
        lt_params->y_type_value = txt_create_from_str(NULL, 5, y_typeval_layout, main_win->std_font, 12, txt_color, CENTER_LEFT, false, main_win);
        lt_params->w_type_value = txt_create_from_str(NULL, 5, w_typeval_layout, main_win->std_font, 12, txt_color, CENTER_LEFT, false, main_win);
        lt_params->h_type_value = txt_create_from_str(NULL, 5, h_typeval_layout, main_win->std_font, 12, txt_color, CENTER_LEFT, false, main_win);

        lt_params->x_value = txt_create_from_str(NULL, 10, x_value_layout, main_win->std_font, 12, txt_color, CENTER_LEFT, false, main_win);
        lt_params->y_value = txt_create_from_str(NULL, 10, y_value_layout, main_win->std_font, 12, txt_color, CENTER_LEFT, false, main_win);
        lt_params->w_value = txt_create_from_str(NULL, 10, w_value_layout, main_win->std_font, 12, txt_color, CENTER_LEFT, false, main_win);
        lt_params->h_value = txt_create_from_str(NULL, 10, h_value_layout, main_win->std_font, 12, txt_color, CENTER_LEFT, false, main_win);

        lt_params->x_value_str = malloc(10);
        lt_params->y_value_str = malloc(10);
        lt_params->w_value_str = malloc(10);
        lt_params->h_value_str = malloc(10);
    }
    txt_set_value_handle(lt_params->name_value, lt->name);
    txt_set_value_handle(lt_params->x_type_value, (char *) layout_get_dimtype_str(lt->x.type));
    txt_set_value_handle(lt_params->y_type_value, (char *) layout_get_dimtype_str(lt->y.type));
    txt_set_value_handle(lt_params->w_type_value, (char *) layout_get_dimtype_str(lt->w.type));
    txt_set_value_handle(lt_params->h_type_value, (char *) layout_get_dimtype_str(lt->h.type));


    layout_get_dimval_str(&(lt->x), lt_params->x_value_str, 10);
    layout_get_dimval_str(&(lt->y), lt_params->y_value_str, 10);
    layout_get_dimval_str(&(lt->w), lt_params->w_value_str, 10);
    layout_get_dimval_str(&(lt->h), lt_params->h_value_str, 10);


    txt_set_value_handle(lt_params->x_value, lt_params->x_value_str);
    txt_set_value_handle(lt_params->y_value, lt_params->y_value_str);
    txt_set_value_handle(lt_params->w_value, lt_params->w_value_str);
    txt_set_value_handle(lt_params->h_value, lt_params->h_value_str);
    // return lt_params;
}

static void set_lt_dim_from_param_str(Layout *lt, RectMem rm)
{
    switch (rm) {
    case X: 
	switch (lt->x.type) {
	case ABS:
	case REL:
	case STACK:
	case REVREL:
	case PAD:
	    lt->x.value = (float)atoi(lt_params->x_value_str);
	    break;
	case SCALE:
	    lt->x.value = atof(lt_params->x_value_str);
	    break;
	case COMPLEMENT:
	    break;
	}
	break;
    case Y:
	switch (lt->y.type) {
	case ABS:
	case REL:
	case STACK:
	case PAD:
	case REVREL:
	    lt->y.value = (float)atoi(lt_params->y_value_str);
	    break;
	case SCALE:
	    lt->y.value = atof(lt_params->y_value_str);
	    break;
	case COMPLEMENT:
	    break;
	}
	break;
    case W:
	switch (lt->w.type) {
	case ABS:
	case REL:
	case STACK:
	case PAD:
	case REVREL:
	    lt->w.value = (float)atoi(lt_params->w_value_str);
	    break;
	case SCALE:
	    lt->w.value = atof(lt_params->w_value_str);
	    break;
	case COMPLEMENT:
	    break;
	}
	break;
    case H:
	switch (lt->h.type) {
	case ABS:
	case REL:
	case STACK:
	case PAD:
	case REVREL:
	    lt->h.value = (float)atoi(lt_params->h_value_str);
	    break;
	case SCALE:
	    lt->h.value = atof(lt_params->h_value_str);
	    break;
	case COMPLEMENT:
	    break;
	}
	break;
    }
    layout_reset(lt);
}
void edit_lt_loop(Layout *lt)
{
    set_lt_params(lt);
    bool exit = false;
    bool shiftdown = false;
    while (!exit) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT 
                || (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_CLOSE) 
                || e.type == SDL_MOUSEBUTTONDOWN) {
                exit = true;
                /* Push the event back to the main event stack, so it can be handled in main.c */
                SDL_PushEvent(&e);
            } else if (e.type == SDL_KEYDOWN) {
                switch (e.key.keysym.scancode) {
		case SDL_SCANCODE_RETURN:
		case SDL_SCANCODE_L:
		    exit = true;
		    break;
		case SDL_SCANCODE_N:
		    txt_edit(lt_params->name_value, layout_draw_main);
		    if (lt->namelabel) {
			txt_reset_display_value(lt->namelabel);
		    }
		    // set_lt_params(lt);
		    break;
		case SDL_SCANCODE_X: {
		    if (shiftdown) {
			txt_edit(lt_params->x_value, layout_draw_main);
			set_lt_dim_from_param_str(lt, X);
		    } else {
			layout_toggle_dimension(lt, &(lt->x), X, &(lt->rect), &(lt->parent->rect));
			set_lt_params(lt);
			layout_reset(lt);
		    }
		    break;
		}
		case SDL_SCANCODE_Y:
		    if (shiftdown) {
			txt_edit(lt_params->y_value, layout_draw_main);
			set_lt_dim_from_param_str(lt, Y);
		    } else {
			layout_toggle_dimension(lt, &(lt->y), Y, &(lt->rect), &(lt->parent->rect));
			set_lt_params(lt);
			layout_reset(lt);
		    }
		    break;
		case SDL_SCANCODE_W:
		    if (shiftdown) {
			txt_edit(lt_params->w_value, layout_draw_main);
			set_lt_dim_from_param_str(lt, W);
		    } else {
			layout_toggle_dimension(lt, &(lt->w), W, &(lt->rect), &(lt->parent->rect));
			set_lt_params(lt);
			layout_reset(lt);
		    }
		    break;
		case SDL_SCANCODE_H:
		    if (shiftdown) {
			txt_edit(lt_params->h_value, layout_draw_main);
			set_lt_dim_from_param_str(lt, H);
		    } else {
			layout_toggle_dimension(lt, &(lt->h), H, &(lt->rect), &(lt->parent->rect));
			set_lt_params(lt);
			layout_reset(lt);
		    }
		    break;
		case SDL_SCANCODE_LSHIFT:
		case SDL_SCANCODE_RSHIFT:
		    shiftdown = true;
		    break;
		default:
		    break;
                } 
            } else if (e.type == SDL_KEYUP) {
                if (e.key.keysym.scancode == SDL_SCANCODE_LSHIFT || e.key.keysym.scancode == SDL_SCANCODE_RSHIFT) {
                    shiftdown = false;
                }
            }
            layout_draw_main();
            SDL_Delay(1);
        }
        
    }
    fprintf(stderr, "\t->exit edit lt loop\n");


}
// typedef struct lt_params {
    
//     Textbox *name_label;
//     Textbox *x_type_label;
//     Textbox *y_type_label;
//     Textbox *w_type_label;
//     Textbox *h_type_label;

//     Textbox *name_value;
//     Textbox *x_type_value;
//     Textbox *y_type_value;
//     Textbox *w_type_value;
//     Textbox *h_type_value;


// } LTParams;
