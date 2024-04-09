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
#include "SDL.h"
#include "SDL_render.h"
#include "color.h"
#include "layout.h"
#include "menu.h"
#include "text.h"
#include "window.h"

#define DEFAULT_WINDOW_FLAGS SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI
#define DEFAULT_RENDER_FLAGS SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC

#define ZOOM_GLOBAL_SCALE 4.0f

/* Create a window struct and initialize all members */
Window *window_create(int w, int h, const char *name) 
{

    Window *window = malloc(sizeof(Window));
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
    window->w = w * window->dpi_scale_factor;
    window->h = h * window->dpi_scale_factor;

    window->zoom_scale_factor = 1.0f;
    window->canvas_src.x = 0;
    window->canvas_src.y = 0;
    window->canvas_src.w = window->w;
    window->canvas_src.h = window->h;

    /* Initialize canvas texture. All rendering will be done to this texture, then copied to the screen. */
    window->canvas = SDL_CreateTexture(rend, 0, SDL_TEXTUREACCESS_TARGET, window->w * window->dpi_scale_factor, window->h * window->dpi_scale_factor);
    if (!window->canvas) {
	fprintf(stderr, "Error: failed to create canvas texture. %s\n", SDL_GetError());
	exit(1);
    }
    
    window->std_font = NULL;
    window->layout = NULL;
    window->num_menus = 0;
    SDL_SetRenderDrawBlendMode(window->rend, SDL_BLENDMODE_BLEND);

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

    /* ttf_destroy_font(win->std_font); */
    fprintf(stdout, "Ok, font destroyed\n");
    /* window_assign_std_font(win, win->std_font->path); */
    fprintf(stdout, "Ok, font assigned\n");
    ttf_reset_dpi_scale_factor(win->std_font);

}

void window_assign_std_font(Window *win, const char *font_path)
{
    win->std_font = ttf_init_font(font_path, win);
    if (!win->std_font) {
	fprintf(stderr, "Error: Unable to initialize font at %s.\n", font_path);
    }
}

/* Reset the w and h members of a window struct (usually in response to a resize event) */
void window_auto_resize(Window *win)
{
    SDL_GetWindowSize(win->win, &(win->w), &(win->h));
    win->w *= win->dpi_scale_factor;
    win->h *= win->dpi_scale_factor;
    win->canvas_src.w = win->w;
    win->canvas_src.h = win->h;
    if (win->canvas) {
	SDL_DestroyTexture(win->canvas);
	win->canvas = SDL_CreateTexture(win->rend, 0, SDL_TEXTUREACCESS_TARGET, win->w * win->dpi_scale_factor, win->h * win->dpi_scale_factor);
	if (!win->canvas) {
	    fprintf(stderr, "Error: failed to create canvas texture. %s\n", SDL_GetError());
	    //exit(1);
	}
    }
}

void window_resize(Window *win, int w, int h)
{

    SDL_SetWindowSize(win->win, w, h);
    win->w = w * win->dpi_scale_factor;
    win->h = h * win->dpi_scale_factor;
    win->canvas_src.w = win->w;
    win->canvas_src.h = win->h;
    if (win->canvas) {
	SDL_DestroyTexture(win->canvas);
	win->canvas = SDL_CreateTexture(win->rend, 0, SDL_TEXTUREACCESS_TARGET, win->w, win->h);
	if (!win->canvas) {
	    fprintf(stderr, "Error: failed to create canvas texture. %s\n", SDL_GetError());
	    // exit(1);
	}
    }
    if (win->layout) {
	layout_reset_from_window(win->layout, win);
    }
    
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
    new_w = (double) win->w / win->zoom_scale_factor;
    new_h = (double) win->h / win->zoom_scale_factor;
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
    SDL_SetRenderTarget(win->rend, NULL);
    SDL_RenderCopy(win->rend, win->canvas, &win->canvas_src, NULL);
    SDL_RenderPresent(win->rend);
}

void window_set_layout(Window *win, Layout *layout)
{
    win->layout = layout;
}

Layout *layout_create_from_window(Window *win);

/* Create a window, intialize font and layout */
/* Window *window_init(int w, int h, const char *name, const char *font_path) */
/* { */
/*     Window *win = window_create(w, h, name); */
/*     window_assign_std_font(win, font_path); */
/*     fprintf(stderr, "Assigned standard font\n"); */
/*     window_set_layout(win, layout_create_from_window(win)); */
/*     return win; */
/* } */

void layout_destroy(Layout *lt);
void window_destroy(Window *win)
{
    if (win->std_font) {
	ttf_destroy_font(win->std_font);
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

void window_pop_menu(Window *win)
{
    if (win->num_menus > 0) {
	menu_destroy(win->menus[win->num_menus - 1]);
	win->num_menus--;
	
    }
}

void window_add_menu(Window *win, Menu *menu)
{
    if (win->num_menus < MAX_WINDOW_MENUS) {
	win->menus[win->num_menus] = menu;
	win->num_menus++;
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

void window_draw_menus(Window *win)
{

    for (uint8_t i=0; i<win->num_menus; i++) {
	menu_draw(win->menus[i]);
    }
}
