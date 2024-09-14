#include "SDL_render.h"
#include "color.h" 
#include "geometry.h"
#include "symbols.h"

#define SYMBOL_STD_DIM 16
#define SYMBOL_DEFAULT_PAD 4
#define SYMBOL_DEFAULT_CORNER_R 4
#define SYMBOL_DEFAULT_THICKNESS 3

Symbol *SYMBOL_TABLE[4];

static void x_draw_fn(void *self)
{
    Symbol *s = (Symbol *)self;
    SDL_SetRenderDrawColor(s->window->rend, 255, 255, 255, 255);
    SDL_Rect r = {0, 0, s->x_dim_pix, s->y_dim_pix};
    int corner_r = SYMBOL_DEFAULT_CORNER_R * s->window->dpi_scale_factor;
    geom_draw_rounded_rect(s->window->rend, &r, corner_r);
    /* geom_draw_circle(s->window->rend, 0, 0, s->y_dim_pix / 2); */
    int pad = SYMBOL_DEFAULT_PAD * s->window->dpi_scale_factor;
    int thickness = SYMBOL_DEFAULT_THICKNESS * s->window->dpi_scale_factor;
    int btm_y = s->y_dim_pix;
    double right_x = s->x_dim_pix;
    double x = 0;
    double x_inc = (double)(s->x_dim_pix - thickness) / s->x_dim_pix;
    SDL_SetRenderDrawColor(s->window->rend, 255, 255, 255, 255);
    for (int y=0; y<=s->y_dim_pix / 2; y++) {
	if (y>=pad) {
	    SDL_RenderDrawLine(s->window->rend, x, y, x + thickness, y);
	    SDL_RenderDrawLine(s->window->rend, x, btm_y, x + thickness, btm_y);
	    SDL_RenderDrawLine(s->window->rend, right_x - thickness, y, right_x, y);
	    SDL_RenderDrawLine(s->window->rend, right_x - thickness, btm_y, right_x, btm_y);
	}
	x += x_inc;
	right_x -= x_inc;
	btm_y--;	
    }
}

static void minimize_draw_fn(void *self)
{
    Symbol *s = (Symbol *)self;
    SDL_SetRenderDrawColor(s->window->rend, 255, 255, 255, 255);
    SDL_Rect r = {0, 0, s->x_dim_pix, s->y_dim_pix};
    int corner_r = SYMBOL_DEFAULT_CORNER_R * s->window->dpi_scale_factor;
    geom_draw_rounded_rect(s->window->rend, &r, corner_r);

    int pad = SYMBOL_DEFAULT_PAD * s->window->dpi_scale_factor;
    int thickness = SYMBOL_DEFAULT_THICKNESS * s->window->dpi_scale_factor;
    int x = pad;
    int y = s->y_dim_pix / 2 - thickness / 2;
    
    int right_x = s->y_dim_pix - pad;

    for (int i=0; i<thickness; i++) {
	SDL_RenderDrawLine(s->window->rend, x, y, right_x, y);
	y++;
    }
}

static void dropdown_draw_fn(void *self)
{
    Symbol *s = (Symbol *)self;
    SDL_SetRenderDrawColor(s->window->rend, 255, 255, 255, 255);
    SDL_Rect r = {0, 0, s->x_dim_pix, s->y_dim_pix};
    int corner_r = SYMBOL_DEFAULT_CORNER_R * s->window->dpi_scale_factor;
    geom_draw_rounded_rect(s->window->rend, &r, corner_r);

    int pad = SYMBOL_DEFAULT_PAD * s->window->dpi_scale_factor;
    int thickness = SYMBOL_DEFAULT_THICKNESS * 0.5 * s->window->dpi_scale_factor;
    double x = pad;
    int y = pad * 1.5;
    double x_inc = ((double)s->x_dim_pix / 2 - pad) / (s->y_dim_pix - 2 * y);
    double right_x = s->x_dim_pix - pad;
    double half_w = (double)s->x_dim_pix / 2;
    while (x < half_w) {
	double loc_thickness = half_w - x;
	if (loc_thickness > thickness) loc_thickness = thickness;
	SDL_RenderDrawLine(s->window->rend, x, y, x + loc_thickness, y);
	SDL_RenderDrawLine(s->window->rend, right_x - loc_thickness, y, right_x, y);
	/* SDL_RenderDrawLine( */
	y++;
	x += x_inc;
	right_x -= x_inc;
    }
}

static void dropup_draw_fn(void *self)
{
    Symbol *s = (Symbol *)self;
    SDL_SetRenderDrawColor(s->window->rend, 255, 255, 255, 255);
    SDL_Rect r = {0, 0, s->x_dim_pix, s->y_dim_pix};
    int corner_r = SYMBOL_DEFAULT_CORNER_R * s->window->dpi_scale_factor;
    geom_draw_rounded_rect(s->window->rend, &r, corner_r);

    int pad = SYMBOL_DEFAULT_PAD * s->window->dpi_scale_factor;
    int thickness = SYMBOL_DEFAULT_THICKNESS * 0.5 * s->window->dpi_scale_factor;
    double x = pad;
    int y = s->y_dim_pix - pad * 1.5;
    double x_inc = ((double)s->x_dim_pix / 2 - pad) / (s->y_dim_pix - 2 * (pad * 1.5));
    double right_x = s->x_dim_pix - pad;
    double half_w = (double)s->x_dim_pix / 2;
    while (x < half_w) {
	double loc_thickness = half_w - x;
	if (loc_thickness > thickness) loc_thickness = thickness;
	SDL_RenderDrawLine(s->window->rend, x, y, x + loc_thickness, y);
	SDL_RenderDrawLine(s->window->rend, right_x - loc_thickness, y, right_x, y);
	/* SDL_RenderDrawLine( */
	y--;
	x += x_inc;
	right_x -= x_inc;
    }
}

void init_symbol_table(Window *win)
{
    int std_dim = SYMBOL_STD_DIM * win->dpi_scale_factor;
    SYMBOL_TABLE[0] = symbol_create(
	win,
	std_dim,
	std_dim,
	x_draw_fn);
    SYMBOL_TABLE[1] = symbol_create(
	win,
	std_dim,
	std_dim,
	minimize_draw_fn);
    SYMBOL_TABLE[2] = symbol_create(
	win,
	std_dim,
	std_dim,
	dropdown_draw_fn);
    SYMBOL_TABLE[3] = symbol_create(
	win,
	std_dim,
	std_dim,
	dropup_draw_fn);
}


Symbol *symbol_create(
    Window *win,
    int x_dim_pix,
    int y_dim_pix,
    void (*draw_fn)(void *self))
{
    Symbol *s = calloc(1, sizeof(Symbol));
    s->window = win;
    s->x_dim_pix = x_dim_pix;
    s->y_dim_pix = y_dim_pix;
    s->draw_fn = draw_fn;
    return s;
}

void symbol_draw(Symbol *s, SDL_Rect *dst)
{
    if (!s->texture || s->redraw) {
	if (s->texture) {
	    SDL_DestroyTexture(s->texture);
	}
	SDL_Texture *saved_targ = SDL_GetRenderTarget(s->window->rend);
	s->texture = SDL_CreateTexture(s->window->rend, 0, SDL_TEXTUREACCESS_TARGET, s->x_dim_pix, s->y_dim_pix);
	SDL_SetTextureBlendMode(s->texture, SDL_BLENDMODE_BLEND);
	SDL_SetRenderTarget(s->window->rend, s->texture);
	/* SDL_SetRenderDrawColor(s->window->rend, 0, 0, 0, 0); */
	/* SDL_RenderClear(s->window->rend); */
	s->draw_fn((void *)s);
	SDL_SetRenderTarget(s->window->rend, saved_targ);
    } else {
	SDL_RenderCopy(s->window->rend, s->texture, NULL, dst);
    }
}

void symbol_draw_w_bckgrnd(Symbol *s, SDL_Rect *dst, SDL_Color *bckgrnd)
{
    SDL_SetRenderDrawColor(s->window->rend, sdl_colorp_expand(bckgrnd));
    int r = SYMBOL_DEFAULT_CORNER_R * s->window->dpi_scale_factor;
    geom_fill_rounded_rect(s->window->rend, dst, r);
    symbol_draw(s, dst);
}

