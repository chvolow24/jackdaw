/*****************************************************************************************************************
  Jackdaw | a stripped-down, keyboard-focused Digital Audio Workstation | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023 Charlie Volow
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:
  
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
  
  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.

*****************************************************************************************************************/

#include <stdio.h>
#include <stdbool.h>
#include "SDL.h"
#include "draw.h"
#include "layout.h"
#include "lt_params.h"
#include "openfile.h"
#include "text.h"
#include "text.h"
#include "window.h"
#include "parse_xml.h"
#include "layout_xml.h"

#define OPEN_SANS_PATH "../assets/ttf/OpenSans-Regular.ttf"

#define CLICK_EDGE_DIST_TOLERANCE 10
#define MAX_LTS 255

#define SCROLL_SCALAR 8

Layout *main_lt;
Layout *clicked_lt;
Layout *param_lt = NULL;
Layout *openfile_lt;
LTParams *lt_params = NULL;
OpenFile *openfile = NULL;
Window *main_win;
bool show_layout_params = false;
bool show_openfile = false;


//TTF_Font *open_sans;


/********* Screenshot ********/

int screenshot_index = 0;

/* Takes a bmp screenshot and saves to the 'images' subdirectory, with index i included in filename. */
void screenshot(int i, SDL_Renderer* rend)
{
  char filename[30];
  sprintf(filename, "gifframes/screenshot%3d.bmp", i);
  printf("\nSaved %s", filename);
  SDL_Surface *sshot = SDL_CreateRGBSurface(0, main_win->w, main_win->h, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000);
  SDL_RenderReadPixels(rend, NULL, SDL_PIXELFORMAT_ARGB8888, sshot->pixels, sshot->pitch);
  SDL_SaveBMP(sshot, filename);
  SDL_FreeSurface(sshot);
}

/******************************/



void init_SDL()
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "\nError initializing SDL: %s\n", SDL_GetError());
        exit(1);
    }
    SDL_SetHint(SDL_HINT_MOUSE_TOUCH_EVENTS, "1");


}

void init_SDL_ttf()
{
    if (TTF_Init() != 0) {
        fprintf(stderr, "\nError: TTF_Init failed: %s", TTF_GetError());
        exit(1);
    }

}

void get_mousep(Window *win, SDL_Point *mousep) {
    SDL_GetMouseState(&(mousep->x), &(mousep->y));
    mousep->x *= win->dpi_scale_factor;
    mousep->y *= win->dpi_scale_factor;
}

int point_dist_from_rect(SDL_Point p, SDL_Rect r, Edge *edge_arg)
{

    Edge edge;
    if (edge_arg) {
        edge = *edge_arg;
    }
    int dist = 0;

    int left_x = r.x;
    int right_x = r.x + r.w;
    int top_y = r.y;
    int bottom_y = r.y + r.h;

    int x_outside;
    if (p.x > right_x) {
        x_outside = p.x - right_x;
    } else if (p.x < left_x) {
        x_outside = left_x - p.x;
    } else {
        x_outside = 0;
    }

    int y_outside;
    if (p.y > bottom_y) {
        y_outside = p.y - bottom_y;
    } else if (top_y > p.y) {
        y_outside = top_y - p.y;
    } else {
        y_outside = 0;
    }

    dist = abs(p.y - top_y) + x_outside;
    edge = TOP;

    int tmp_dist;
    if ((tmp_dist = abs(p.y - bottom_y) + x_outside) < dist) {
        dist = tmp_dist;
        edge = BOTTOM;
    }
    if ((tmp_dist = abs(p.x - left_x) + y_outside) < dist) {
        dist = tmp_dist;
        edge = LEFT;
    }
    if ((tmp_dist = abs(p.x - right_x) + y_outside) < dist) {
        dist = tmp_dist;
        edge = RIGHT;
    }
    *edge_arg = edge;
    return dist;

}

void get_clicked_layout(SDL_Point p, Layout *top_level, int *distance, Layout **ret, Edge *edge, Corner *corner)
{
    if (top_level->type == PRGRM_INTERNAL || top_level->type == ITERATION) {
        return;
    }
    if (*corner != NONECRNR) {
        return;
    }
    Edge edge_internal;

    SDL_Rect top_rect = top_level->rect;

    if (abs(top_rect.x + top_rect.w - p.x) + abs(top_rect.y + top_rect.h - p.y) < CLICK_EDGE_DIST_TOLERANCE * 2) {
        *distance = 500;
        *ret = top_level;
        *edge = NONEEDGE;
        *corner = BTTMRT;
        return;
    }
    int tmp_distance = point_dist_from_rect(p, top_level->rect, &edge_internal);
    if  (tmp_distance < *distance) {
        *distance = tmp_distance;
        if (*ret) {
            (*ret)->selected = false;
        }
        *ret = top_level;
        *edge = edge_internal;
        *corner = NONECRNR;
    }
    for (uint8_t i=0; i<top_level->num_children; i++) {
        get_clicked_layout(p, top_level->children[i], distance, ret, edge, corner);
    }
}

void set_window_size_to_lt()
{
    if (main_lt->w.type != SCALE && main_lt->h.type != SCALE) {
        resize_window(main_win, main_lt->w.value.intval, main_lt->h.value.intval);
    } else {
        main_lt->rect.w = main_win->w;
        main_lt->rect.h = main_win->h;
    }
    main_lt->x.type = ABS;
    main_lt->y.type = ABS;
    main_lt->x.value.intval = 0;
    main_lt->y.value.intval = 0;
}

static void reset_clicked_lt(Layout *to)
{
    if (clicked_lt != to) {
	if (clicked_lt) {
	    clicked_lt->selected = false;
	}
	clicked_lt = to;
	clicked_lt->selected = true;
    }
}

int main(int argc, char** argv) 
{
    if (argc > 2) {
        fprintf(stderr, "Usage: layout [file to open]\n");
        exit(1);
    }
    init_SDL();
    init_SDL_ttf();

    main_win = create_window(1200, 900, "Layout editor");
    assign_std_font(main_win, OPEN_SANS_PATH);

    //  open_sans = open_font("../assets/ttf/OpenSans-Regular.ttf", 12, main_win);

    if (argc == 2) {
        FILE *f = fopen(argv[1], "r");
        if (!f) {
            fprintf(stderr, "Unable to open layout file at %s\n", argv[1]);
            exit(1);
        }
        main_lt = read_layout_from_xml(argv[1]);
        main_lt->type = NORMAL;
        set_window_size_to_lt();
    } else {
        main_lt = create_layout_from_window(main_win);
    }

    SDL_StopTextInput();
    bool quit = false;
    bool mousedown = false;
    bool cmdctrldown = false;
    bool shiftdown  = false;
    bool fingerdown = false;

    bool screen_record = false;

    SDL_Point mousep;
    clicked_lt = NULL;
    bool layout_clicked = false;
    bool layout_corner_clicked = false;
    Edge ed = NONEEDGE;
    Corner crnr = NONECRNR;

    Layout *scrolling = NULL;
    
    while (!quit) {
        get_mousep(main_win, &mousep);
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                quit = true;
            } else if (e.type == SDL_WINDOWEVENT) {
                if (e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                    auto_resize_window(main_win);
                    reset_layout_from_window(main_lt, main_win);
                    reset_layout(main_lt);

                }
            } else if (e.type == SDL_MOUSEBUTTONDOWN) {
                crnr = NONECRNR;
                mousedown = true;
                int dist = 5000;
                get_clicked_layout(mousep, main_lt, &dist, &(clicked_lt), &ed, &crnr);
                if (crnr != NONECRNR) {
                    layout_clicked = true;
                    layout_corner_clicked = true;
                    clicked_lt->selected = true;
                } else if (dist < CLICK_EDGE_DIST_TOLERANCE) {
                    layout_clicked = true;
                    clicked_lt->selected = true;
                } else {
                    // fprintf(stderr, "Falsifying layout clicked\n");
                    clicked_lt->selected = false;
                    layout_clicked = false;
                }
            } else if (e.type == SDL_MOUSEMOTION) {
                if (mousedown == true) {
                    if (layout_clicked && !layout_corner_clicked) {
                        if (cmdctrldown) {
                            switch (ed) {
                                case TOP:
                                    set_edge(clicked_lt, ed, mousep.y, shiftdown);
                                    break;
                                case RIGHT:
                                    set_edge(clicked_lt, ed, mousep.x, shiftdown);
                                    break;
                                case BOTTOM:
                                    set_edge(clicked_lt, ed, mousep.y, shiftdown);
                                    break;
                                case LEFT:
                                    set_edge(clicked_lt, ed, mousep.x, shiftdown);
                                    break;
                                case NONEEDGE:
                                    break;
                            }
                        } else {
                            move_position(clicked_lt, e.motion.xrel * main_win->dpi_scale_factor, e.motion.yrel * main_win->dpi_scale_factor, shiftdown);
                        }
                        // set_position(clicked_lt, mousep.x, mousep.y);
                        // translate_layout(clicked_lt, e.motion.xrel * main_win->dpi_scale_factor, e.motion.yrel * main_win->dpi_scale_factor, shiftdown);
                        // reset_layout(clicked_lt);
                        if (show_layout_params) {
                            set_lt_params(clicked_lt);
                        }
                    } else if (layout_corner_clicked) {
                        set_corner(clicked_lt, crnr, mousep.x, mousep.y, shiftdown);
                        // resize_layout(clicked_lt, e.motion.xrel * main_win->dpi_scale_factor, e.motion.yrel * main_win->dpi_scale_factor, shiftdown);
                        // reset_layout(clicked_lt);
                        if (show_layout_params) {
                            set_lt_params(clicked_lt);
                        }
                    }
                }
            } else if (e.type == SDL_MOUSEBUTTONUP) {
                mousedown = false;
                // layout_clicked = false;
                layout_corner_clicked = false;
                // clicked_lt->selected = false;

		/* TODO: Get this to work. cf https://discourse.libsdl.org/t/trackpad-events/49461 */
	    } else if (e.type == SDL_FINGERUP) {
		fingerdown = false;
	    } else if (e.type == SDL_FINGERDOWN) {

		fingerdown = true;
	    } else if (e.type == SDL_FINGERMOTION) {



		
            } else if (e.type == SDL_MOUSEWHEEL) {
	        scrolling = handle_scroll(main_lt, &mousep, e.wheel.preciseX * SCROLL_SCALAR * 1, e.wheel.preciseY * SCROLL_SCALAR * -1);
            } else if (e.type == SDL_KEYDOWN) {
                switch (e.key.keysym.scancode) {
                    case SDL_SCANCODE_LGUI:
                    case SDL_SCANCODE_RGUI:
                    case SDL_SCANCODE_LCTRL:
                    case SDL_SCANCODE_RCTRL:
                        cmdctrldown = true;
                        break;
                    case SDL_SCANCODE_LSHIFT:
                    case SDL_SCANCODE_RSHIFT:
                        shiftdown = true;
                        break;
                    case SDL_SCANCODE_S:
                        if (clicked_lt && layout_clicked && cmdctrldown) {
                            write_layout_to_file(clicked_lt);
                        } else if (cmdctrldown) {
                            write_layout_to_file(main_lt);
                        }
                        break;
                    case SDL_SCANCODE_C:
                        if (cmdctrldown) {
                            if (layout_clicked && clicked_lt) {
                                copy_layout(clicked_lt, clicked_lt->parent);
                            }
                        } else if (shiftdown) {
                            Layout *new_child;
                            if (layout_clicked) {
                                new_child = add_child(clicked_lt);
                                set_default_dims(new_child);
                                reset_layout(new_child);
                            } else {
                                new_child = add_child(main_lt);
                            }
                            set_default_dims(new_child);
                            reset_layout(new_child);
                        }
                        break;
		    case SDL_SCANCODE_Q:
			if (layout_clicked && clicked_lt) {
			    if (shiftdown) {
				add_complementary_child(clicked_lt, W);
			    } else {
			        add_complementary_child(clicked_lt, H);
			    }
			    reset_layout(clicked_lt);
			}
			break;
                    case SDL_SCANCODE_I:
                        if (clicked_lt && layout_clicked) {
                            if (shiftdown) {
                                remove_iteration_from_layout(clicked_lt);
                            } else {
				if (cmdctrldown) {
				    
				    add_iteration_to_layout(clicked_lt, HORIZONTAL, true);
				} else {
				    add_iteration_to_layout(clicked_lt, VERTICAL, true);
				}
                            }
                        }
                        break;
                    case SDL_SCANCODE_L:
                        if (show_layout_params) {
                            show_layout_params = false;
                        } else {
                            if (!param_lt) {
                                param_lt = read_layout_from_xml("template_lts/param_lt.xml");
                            }
                            reparent(param_lt, main_lt);
                            show_layout_params = true;
                            if (clicked_lt && layout_clicked) {
                                edit_lt_loop(clicked_lt);
                            } else {
                                edit_lt_loop(main_lt);
                            }
                            show_layout_params = false;
                        }
                        break;
                    case SDL_SCANCODE_O:
                        if (show_openfile) {
                            show_openfile = false;
                        } else {
                            show_openfile = true;
                            if (!openfile_lt) {
                                openfile_lt = read_layout_from_xml("template_lts/openfile.xml");
                                reparent(openfile_lt, main_lt);
                            }
                            if (clicked_lt && layout_clicked) {
                                fprintf(stderr, "A layout is clicked! \"%s\" at %p", clicked_lt->name, clicked_lt);
                                openfile_loop(clicked_lt);
                            } else {
                                show_openfile = true;
                                fprintf(stderr, "NO layout is clicked! \"%s\" at %p", main_lt->name, clicked_lt);
                                openfile_loop(main_lt);

                                // main_lt = openfile_loop(main_lt);
                                // set_window_size_to_lt();
                                // reset_layout_from_window(main_lt, main_win);
                            }
                        }
                        break;
                    case SDL_SCANCODE_P:
			reset_clicked_lt(get_parent(clicked_lt));
                        /* if (layout_clicked && clicked_lt && clicked_lt->parent) { */
                        /*     clicked_lt->selected = false; */
                        /*     clicked_lt = clicked_lt->parent; */
                        /*     clicked_lt->selected = true; */
                        /* } */
                        break;
                    case SDL_SCANCODE_LEFTBRACKET:
			reset_clicked_lt(iterate_siblings(clicked_lt, -1));
			break;
		    case SDL_SCANCODE_RIGHTBRACKET:
			reset_clicked_lt(iterate_siblings(clicked_lt, 1));
                        break;
		    case SDL_SCANCODE_BACKSLASH:
			reset_clicked_lt(get_first_child(clicked_lt));
			break;
		    case SDL_SCANCODE_DELETE:
		    case SDL_SCANCODE_BACKSPACE:
			if (clicked_lt) {
			    delete_layout(clicked_lt);
			    reset_layout(main_lt);
			}
			break;
		    case SDL_SCANCODE_0:
		        screen_record = screen_record ? false : true;
		        break;
                    default:
                        break;
                }
            } else if (e.type == SDL_KEYUP) {
                switch (e.key.keysym.scancode) {
                    case SDL_SCANCODE_LGUI:
                    case SDL_SCANCODE_RGUI:
                    case SDL_SCANCODE_LCTRL:
                    case SDL_SCANCODE_RCTRL:
                        cmdctrldown = false;
                        break;
                    case SDL_SCANCODE_LSHIFT:
                    case SDL_SCANCODE_RSHIFT:
                        shiftdown = false;
                    default:
                        break;
                }
            }
        }
	if (scrolling && !fingerdown) {
	    if (scroll_step(scrolling) == 0) {
		scrolling = NULL;
	    }
	}
        draw_main();
        SDL_Delay(1);

	if (screen_record) {
	    screenshot_index++;
	    screenshot(screenshot_index, main_win->rend);
	}
    }

}
