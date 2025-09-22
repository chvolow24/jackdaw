/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/
#include <stdio.h>
#include "SDL_render.h"
#include "autocompletion.h"
#include "color.h"
#include "layout.h"
#include "menu.h"
#include "session.h"
#include "text.h"
#include "window.h"
#include "modal.h"

#define DEFAULT_WINDOW_FLAGS SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI
#define DEFAULT_RENDER_FLAGS SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC

#define ZOOM_GLOBAL_SCALE 4.0f

#ifndef LAYOUT_BUILD
#include "project.h"
extern Project *proj;
#endif


/* Create a window struct and initialize all members */
Window *window_create(int w, int h, const char *name) 
{

    Window *window = calloc(1, sizeof(Window));
    SDL_Window *win = SDL_CreateWindow(name, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, w, h, (Uint32)DEFAULT_WINDOW_FLAGS);
    if (!win) {
        fprintf(stderr, "Error creating window: %s", SDL_GetError());
        exit(1);
    }

    SDL_Renderer *rend = SDL_CreateRenderer(win, -1, (Uint32)DEFAULT_RENDER_FLAGS);
    if (!rend) {
        fprintf(stderr, "Error creating renderer: %s", SDL_GetError());
    }

    window->dpi_scale_factor = 0;
    int rw = 0, rh = 0, ww = 0, wh = 0;
    SDL_GetWindowSize(win, &ww, &wh);
    SDL_GetRendererOutputSize(rend, &rw, &rh);

    window->dpi_scale_factor = (double) rw / ww;
    
    if (window->dpi_scale_factor == 0 || window->dpi_scale_factor != (double) rh / wh) {
        fprintf(stderr, "Error setting scale factor: %s", SDL_GetError());
        exit(1);
    }
    window->win = win;
    window->rend = rend;
    window->w_pix = w * window->dpi_scale_factor;
    window->h_pix = h * window->dpi_scale_factor;

    window->zoom_scale_factor = 1.0f;
    /* window->canvas_src.x = 0; */
    /* window->canvas_src.y = 0; */
    window->canvas_src.w = window->w_pix;
    window->canvas_src.h = window->h_pix;

    /* Initialize canvas texture. All rendering will be done to this texture, then copied to the screen. */
    window->canvas = SDL_CreateTexture(rend, 0, SDL_TEXTUREACCESS_TARGET, window->w_pix, window->h_pix);
    /* window->canvas = SDL_CreateTexture(rend, 0, SDL_TEXTUREACCESS_TARGET, window->w_pix * window->dpi_scale_factor, window->h_pix * window->dpi_scale_factor); */
    if (!window->canvas) {
	fprintf(stderr, "Error: failed to create canvas texture. %s\n", SDL_GetError());
	exit(1);
    }

    /* window->i_state = 0; */
    
    /* window->std_font = NULL; */
    /* window->bold_font = NULL; */
    /* window->layout = NULL; */
    /* window->num_menus = 0; */
    /* window->num_modes = 0; */
    /* window->num_modals = 0; */
    /* window->active_tab_view = NULL; */
    /* window->active_page = NULL; */
    
    SDL_SetRenderDrawBlendMode(window->rend, SDL_BLENDMODE_BLEND);

    window->screenrecording = false;
    window->txt_editing = NULL;
    return window;
}


void window_check_monitor_dpi(Window *win)
{

    win->dpi_scale_factor = 0;
    int rw = 0, rh = 0, ww = 0, wh = 0;
    SDL_GetWindowSize(win->win, &ww, &wh);
    SDL_GetRendererOutputSize(win->rend, &rw, &rh);

    win->dpi_scale_factor = (double) rw / ww;
    
    if (win->dpi_scale_factor == 0 || win->dpi_scale_factor != (double) rh / wh) {
        fprintf(stderr, "Error setting scale factor: %s", SDL_GetError());
        exit(1);
    }
    ttf_reset_dpi_scale_factor(win->std_font);

}

void window_assign_font(Window *win, const char *font_path, FontType type)
{
    Font **font_to_init;

    int style = TTF_STYLE_NORMAL;
    switch (type) {
    case FONT_REG:
	font_to_init = &win->std_font;
	break;
    case FONT_BOLD:
	font_to_init = &win->bold_font;
	style = TTF_STYLE_BOLD;
	break;
    case FONT_MONO:
        font_to_init = &win->mono_font;
	break;
    case FONT_MONO_BOLD:
	font_to_init = &win->mono_bold_font;
	break;
    case FONT_SYMBOLIC:
	font_to_init = &win->symbolic_font;
	break;
    case FONT_MATHEMATICAL:
	font_to_init = &win->mathematical_font;
	break;
    case FONT_MUSIC:
	font_to_init = &win->music_font;
	break;
	
    }
    *font_to_init = ttf_init_font(font_path, win, style);
    if (!(*font_to_init)) {
	fprintf(stderr, "Failed to initialize font at %s\n", font_path);
	exit(1);
    }
	    
}

/* Reset the w and h members of a window struct (usually in response to a resize event) */
void window_auto_resize(Window *win)
{
    SDL_GetWindowSize(win->win, &(win->w_pix), &(win->h_pix));
    /* win->layout->w.value.intval = win->w; */
    /* win->layout->h.value.intval = win->h; */
    /* layout_reset(win->layout); */
    win->w_pix *= win->dpi_scale_factor;
    win->h_pix *= win->dpi_scale_factor;
    win->canvas_src.w = win->w_pix;
    win->canvas_src.h = win->h_pix;
    if (win->canvas) {
	SDL_DestroyTexture(win->canvas);
	win->canvas = SDL_CreateTexture(win->rend, 0, SDL_TEXTUREACCESS_TARGET, win->w_pix * win->dpi_scale_factor, win->h_pix * win->dpi_scale_factor);
	if (!win->canvas) {
	    fprintf(stderr, "Error: failed to create canvas texture. %s\n", SDL_GetError());
	    //exit(1);
	}
    }
    layout_reset_from_window(win->layout, win);

}

void window_resize(Window *win, int w, int h)
{

    SDL_SetWindowSize(win->win, w, h);
    win->w_pix = w * win->dpi_scale_factor;
    win->h_pix = h * win->dpi_scale_factor;
    win->canvas_src.w = win->w_pix;
    win->canvas_src.h = win->h_pix;
    if (win->canvas) {
	SDL_DestroyTexture(win->canvas);
	win->canvas = SDL_CreateTexture(win->rend, 0, SDL_TEXTUREACCESS_TARGET, win->w_pix, win->h_pix);
	if (!win->canvas) {
	    fprintf(stderr, "Error: failed to create canvas texture. %s\n", SDL_GetError());
	    // exit(1);
	}
    }
    if (win->layout) {
	layout_reset_from_window(win->layout, win);
    }
}

void window_resize_passive(Window *win, int w, int h)
{
    win->w_pix = w * win->dpi_scale_factor;
    win->h_pix = h * win->dpi_scale_factor;
    win->canvas_src.w = win->w_pix;
    win->canvas_src.h = win->h_pix;
    if (win->canvas) {
	SDL_DestroyTexture(win->canvas);
	win->canvas = SDL_CreateTexture(win->rend, 0, SDL_TEXTUREACCESS_TARGET, win->w_pix, win->h_pix);
	if (!win->canvas) {
	    fprintf(stderr, "Error: failed to create canvas texture. %s\n", SDL_GetError());
	    // exit(1);
	}
    }
    if (win->layout) {
	layout_reset_from_window(win->layout, win);
    }
    #ifndef LAYOUT_BUILD
    if (win->active_tabview) {
	tabview_reset(win->active_tabview, win->active_tabview->leftmost_index);
    }
    #endif

 
}

void window_set_mouse_point(Window *win, int logical_x, int logical_y)
{
    win->mousep_screen.x = logical_x * win->dpi_scale_factor;
    win->mousep_screen.y = logical_y * win->dpi_scale_factor;
    win->mousep.x = win->mousep_screen.x;
    win->mousep.y = win->mousep_screen.y;

    
    if (win->zoom_scale_factor != 1.0f && win->zoom_scale_factor != 0) {
	double new_x = (double) win->mousep.x / win->zoom_scale_factor;
	double new_y = (double) win->mousep.y / win->zoom_scale_factor;
	win->mousep.x = (int) round(new_x);
	win->mousep.y = (int) round(new_y);
	win->mousep.x += win->canvas_src.x;
	win->mousep.y += win->canvas_src.y;
    }
}

void window_zoom(Window *win, float zoom_by)
{
    fprintf(stderr, "WARNING: window_zoom doesn't work well and should not be used until fixed.\n");
    win->zoom_scale_factor += zoom_by * win->zoom_scale_factor * ZOOM_GLOBAL_SCALE;
    if (win->zoom_scale_factor < 1.0f) {
	win->zoom_scale_factor = 1.0f;
    }
    double new_w, new_h;
    new_w = (double) win->w_pix / win->zoom_scale_factor;
    new_h = (double) win->h_pix / win->zoom_scale_factor;
    win->canvas_src = (SDL_Rect){
	round(win->mousep_screen.x - new_w / 2.0f),
	round(win->mousep_screen.y - new_h / 2.0f),
        /* diff_w / 2 - win->mousep.x / 2, */
	/* diff_h / 2 - win->mousep.y / 2, */
	round(new_w),
	round(new_h)
    };
    if (win->canvas_src.x < 0) {
	win->canvas_src.x = 0;
    }
    if (win->canvas_src.y < 0) {
	win->canvas_src.y = 0;
    }
}

void window_defer_draw(Window *win, void (*draw_op)(void *), void *obj)
{
    if (win->num_deferred_draw_ops < WINDOW_MAX_DEFERRED_DRAW_OPS) {
	win->deferred_draw_ops[win->num_deferred_draw_ops] = draw_op;
	win->deferred_draw_objs[win->num_deferred_draw_ops] = obj;
	win->num_deferred_draw_ops++;
    }	
}

void window_start_draw(Window *win, SDL_Color *bckgrnd_color)
{
    SDL_SetRenderTarget(win->rend, win->canvas);
    if (bckgrnd_color) {
	SDL_SetRenderDrawColor(win->rend, sdl_colorp_expand(bckgrnd_color));
	SDL_RenderClear(win->rend);
    }
}

void window_end_draw(Window *win)
{
    /* Do deferred draw operations */
    for (uint8_t i=0; i<win->num_deferred_draw_ops; i++) {
	win->deferred_draw_ops[i](win->deferred_draw_objs[i]);
    }
    /* Reset for next frame */
    win->num_deferred_draw_ops = 0;
    
    SDL_SetRenderTarget(win->rend, NULL);
    SDL_RenderCopy(win->rend, win->canvas, &win->canvas_src, NULL);
    SDL_RenderPresent(win->rend);
}
void window_set_layout(Window *win, Layout *layout)
{
    layout->x.value = 0.0f;
    layout->y.value = 0.0f;
    layout->w.value = win->w_pix / win->dpi_scale_factor;
    layout->h.value = win->h_pix / win->dpi_scale_factor;
    layout_reset(layout);
    win->layout = layout;
}

Layout *layout_create_from_window(Window *win);



void layout_destroy(Layout *lt);
void window_destroy(Window *win)
{
    /* if (win->ac_active) { */
    #ifndef LAYOUT_BUILD
    autocompletion_deinit(&win->ac);
    #endif
    /* } */
    if (win->std_font) {
	ttf_destroy_font(win->std_font);
    }
    if (win->bold_font) {
	ttf_destroy_font(win->bold_font);
    }
    if (win->mono_font) {
	ttf_destroy_font(win->mono_font);
    }
    if (win->mono_bold_font) {
	ttf_destroy_font(win->mono_bold_font);
    }
    if (win->symbolic_font) {
	ttf_destroy_font(win->symbolic_font);
    }
    if (win->mathematical_font) {
	ttf_destroy_font(win->mathematical_font);
    }
    if (win->rend) {
	SDL_DestroyRenderer(win->rend);
    }
    if (win->canvas) {
	SDL_DestroyTexture(win->canvas);
    }
    if (win->layout) {
	layout_destroy(win->layout);
    }
    free(win);    
}

#ifndef LAYOUT_BUILD
void window_pop_menu(Window *win)
{
    if (win->num_menus > 0) {
	menu_destroy(win->menus[win->num_menus - 1]);
	win->num_menus--;
    }
    if (win->num_menus == 0 && win->modes[win->num_modes - 1] == MODE_MENU_NAV)  {
	window_pop_mode(win);
    }
}
#endif

/* extern Window *main_win; */
void window_add_menu(Window *win, Menu *menu)
{
    if (win->num_menus < MAX_WINDOW_MENUS) {
	win->menus[win->num_menus] = menu;
	win->num_menus++;
	if (win->modes[win->num_modes - 1] != MODE_MENU_NAV) {
	    window_push_mode(win, MODE_MENU_NAV);
	}
	menu->sel_col = 0;
	menu->columns[0]->sel_sctn = 0;
	menu->columns[0]->sections[0]->sel_item = 0;
	menu->columns[0]->sections[0]->items[0]->selected = true;
	
	
    } else {
	fprintf(stderr, "Error: window already has maximum number of menus (%d)\n", win->num_menus);
    }
}

Menu *window_top_menu(Window *win)
{
    if (win->num_menus > 0) {
	return win->menus[win->num_menus - 1];
    } else {
	return NULL;
    }
}


void window_push_mode(Window *win, InputMode im)
{
    /* if (im == TEXT_EDIT) */
    /* 	SDL_StartTextInput(); */

    /* fprintf(stderr, "\nWINDOW PUSH MODE %d (%s)\n\n", im, input_mode_str(im)); */
    /* if (im == MIDI_QWERTY) breakfn(); */
    if (win->num_modes < WINDOW_MAX_MODES) {
	win->modes[win->num_modes] = im;
	win->num_modes++;
    } else {
	fprintf(stderr, "Error: window already has maximum number of modes\n");
    }
}

InputMode window_pop_mode(Window *win)
{
    /* if (win->modes[win->num_modes] == TEXT_EDIT) */
    /* 	SDL_StopTextInput(); */
    if (win->num_modes > 0) {
	win->num_modes--;
	return win->modes[win->num_modes];
    }
    return MODE_GLOBAL;
}

#ifndef LAYOUT_BUILD
#include "page.h"
void window_push_modal(Window *win, Modal *modal)
{
    Session *session = session_get();
    while (win->num_modals > 0) {
	window_pop_modal(win);
    }
    
    #ifndef LAYOUT_BUILD
    Timeline *tl = ACTIVE_TL;
    tl->needs_redraw = true;
    #endif
    
    if (win->active_tabview) {
	tabview_close(win->active_tabview);
    }
    if (win->active_page) {
	page_close(win->active_page);
    }
    #ifndef LAYOUT_BUILD
    if (win->ac_active) {
	autocompletion_escape();
    }
    #endif
    while (win->num_menus > 0) {
	window_pop_menu(win);
    }
    if (win->modes[win->num_modes - 1] == MODE_MENU_NAV) {
	window_pop_mode(win);
    }
    if (win->num_modals < WINDOW_MAX_MODALS) {
	win->modals[win->num_modals] = modal;
	win->num_modals++;
	/* layout_center_agnostic(modal->layout, true, true); */
	if (win->modes[win->num_modes - 1] != MODE_MODAL) {
	    window_push_mode(win, MODE_MODAL);
	}
    } else {
	fprintf(stderr, "Error: window already has maximum number of modals\n");
    }
}

void window_pop_modal(Window *win)
{
    if (win->num_modals == 0) {
	return;
    }
    if (win->txt_editing) {
	txt_stop_editing(win->txt_editing);
    }
    
    modal_destroy(win->modals[win->num_modals - 1]);
    /* } */
    win->num_modals--;
    if (win->num_menus > 0) {
	window_pop_menu(win);
    }

    if (win->num_modals == 0) {
	window_pop_mode(win);
    }
}


void window_draw_menus(Window *win)
{
    for (uint8_t i=0; i<win->num_menus; i++) {
	menu_draw(win->menus[i]);
    }
}

void window_draw_modals(Window *win)
{
    for (uint8_t i=0; i<win->num_modals; i++) {
	modal_draw(win->modals[i]);
    }
}

#endif
