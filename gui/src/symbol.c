#include "SDL_render.h"
#include "color.h" 
#include "geometry.h"
#include "symbol.h"

#define SYMBOL_DEFAULT_PAD 4
#define SYMBOL_DEFAULT_CORNER_R 4
#define SYMBOL_DEFAULT_THICKNESS 3
#define SYMBOL_FILTER_DIM_SCALAR 3
#define SYMBOL_FILTER_PAD 6

Symbol *SYMBOL_TABLE[7];

static void x_draw_fn(void *self)
{
    Symbol *s = (Symbol *)self;
    SDL_SetRenderDrawColor(s->window->rend, 255, 255, 255, 255);
    SDL_Rect r = {0, 0, s->x_dim_pix, s->y_dim_pix};
    /* int corner_r = SYMBOL_DEFAULT_CORNER_R * s->window->dpi_scale_factor; */
    geom_draw_rounded_rect(s->window->rend, &r, s->corner_rad_pix);
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
    /* int corner_r = SYMBOL_DEFAULT_CORNER_R * s->window->dpi_scale_factor; */
    geom_draw_rounded_rect(s->window->rend, &r, s->corner_rad_pix);

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
    /* int corner_r = SYMBOL_DEFAULT_CORNER_R * s->window->dpi_scale_factor; */
    geom_draw_rounded_rect(s->window->rend, &r, s->corner_rad_pix);

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
    /* int corner_r = SYMBOL_DEFAULT_CORNER_R * s->window->dpi_scale_factor; */
    geom_draw_rounded_rect(s->window->rend, &r, s->corner_rad_pix);

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

/* draws at center, rather than at origin (top left) */
static void keyframe_draw_fn(void *self)
{
    Symbol *s = (Symbol *)self;
    /* int half_diag = KEYFRAME_DIAG / 2; */
    int half_diag = s->x_dim_pix / 2;
    int x = 0;
    int y1 = half_diag;
    int y2 = half_diag;
    SDL_SetRenderDrawColor(s->window->rend, 255, 255, 255, 255);
    for (int i=0; i<s->x_dim_pix; i++) {
	SDL_RenderDrawLine(s->window->rend, x, y1, x, y2);
	if (i < half_diag) {
	    y1--;
	    y2++;
	} else {
	    y1++;
	    y2--;
	}
	x++;
    }
}


static void lowshelf_draw_fn(void *self)
{
    Symbol *s = (Symbol *)self;

    /* int corner_r = SYMBOL_DEFAULT_CORNER_R * SYMBOL_FILTER_DIM_SCALAR * s->window->dpi_scale_factor; */
    int pad = SYMBOL_FILTER_PAD * s->window->dpi_scale_factor;
    SDL_Rect outer_rect = {0, 0, s->x_dim_pix, s->y_dim_pix};
    SDL_Rect inner_rect = {pad, pad, s->x_dim_pix - pad * 2, s->y_dim_pix - pad * 2};
    /* SDL_Rect inner_rect = { */
    int mid_y = s->y_dim_pix / 2;
    int last_y_up = mid_y;
    int last_y_down = mid_y;
    SDL_SetRenderDrawColor(s->window->rend, 255, 255, 255, 255);
    for (int x=pad; x<pad + inner_rect.w; x++) {
        double x_freq = (double)(x-pad) / inner_rect.w;
	double y_prop = 1.0 / (1.0 + pow(x_freq / 0.35, 5));
	int y_up = mid_y + y_prop * inner_rect.h / 2;
	int y_down = mid_y - y_prop * inner_rect.h / 2;
	if (x != pad) {
	    SDL_RenderDrawLine(s->window->rend, x-1, last_y_up, x, y_up);
	    SDL_RenderDrawLine(s->window->rend, x-1, last_y_down, x, y_down);
	}
	last_y_up = y_up;
	last_y_down = y_down;
    }
    geom_draw_rounded_rect(s->window->rend, &outer_rect, s->corner_rad_pix);

}


static void highshelf_draw_fn(void *self)
{
    Symbol *s = (Symbol *)self;

    /* int corner_r = SYMBOL_DEFAULT_CORNER_R * SYMBOL_FILTER_DIM_SCALAR * s->window->dpi_scale_factor; */
    int pad = SYMBOL_FILTER_PAD * s->window->dpi_scale_factor;
    SDL_Rect outer_rect = {0, 0, s->x_dim_pix, s->y_dim_pix};
    SDL_Rect inner_rect = {pad, pad, s->x_dim_pix - pad * 2, s->y_dim_pix - pad * 2};
    /* SDL_Rect inner_rect = { */
    int mid_y = s->y_dim_pix / 2;
    int last_y_up = mid_y;
    int last_y_down = mid_y;
    SDL_SetRenderDrawColor(s->window->rend, 255, 255, 255, 255);
    for (int x=pad; x<pad + inner_rect.w; x++) {
        double x_freq = (double)(inner_rect.w - x + pad) / inner_rect.w;
	double y_prop = 1.0 / (1.0 + pow(x_freq / 0.35, 5));
	int y_up = mid_y + y_prop * inner_rect.h / 2;
	int y_down = mid_y - y_prop * inner_rect.h / 2;
	if (x != pad) {
	    SDL_RenderDrawLine(s->window->rend, x-1, last_y_up, x, y_up);
	    SDL_RenderDrawLine(s->window->rend, x-1, last_y_down, x, y_down);
	}
	last_y_up = y_up;
	last_y_down = y_down;
    }
    geom_draw_rounded_rect(s->window->rend, &outer_rect, s->corner_rad_pix);

}



void init_symbol_table(Window *win)
{
    int std_dim = SYMBOL_STD_DIM * win->dpi_scale_factor;
    int filter_dim = SYMBOL_STD_DIM * 3 * win->dpi_scale_factor;
    int std_rad = SYMBOL_DEFAULT_CORNER_R * win->dpi_scale_factor;
    int filter_rad = SYMBOL_DEFAULT_CORNER_R * 3 * win->dpi_scale_factor;
    SYMBOL_TABLE[0] = symbol_create(
	win,
	std_dim,
	std_dim,
	std_rad,
	x_draw_fn);
    SYMBOL_TABLE[1] = symbol_create(
	win,
	std_dim,
	std_dim,
	std_rad,
	minimize_draw_fn);
    SYMBOL_TABLE[2] = symbol_create(
	win,
	std_dim,
	std_dim,
	std_rad,
	dropdown_draw_fn);
    SYMBOL_TABLE[3] = symbol_create(
	win,
	std_dim,
	std_dim,
	std_rad,
	dropup_draw_fn);
    SYMBOL_TABLE[4] = symbol_create(
	win,
	std_dim,
	std_dim,
	std_rad,
	keyframe_draw_fn);
    SYMBOL_TABLE[5] = symbol_create(
	win,
	filter_dim,
	filter_dim,
	filter_rad,
	lowshelf_draw_fn);
    SYMBOL_TABLE[6] = symbol_create(
	win,
	filter_dim,
	filter_dim,
	filter_rad,
	highshelf_draw_fn);
}



Symbol *symbol_create(
    Window *win,
    int x_dim_pix,
    int y_dim_pix,
    int corner_rad_pix,
    void (*draw_fn)(void *self))
{
    Symbol *s = calloc(1, sizeof(Symbol));
    s->window = win;
    s->x_dim_pix = x_dim_pix;
    s->y_dim_pix = y_dim_pix;
    s->corner_rad_pix = corner_rad_pix;
    s->draw_fn = draw_fn;
    return s;
}

void symbol_destroy(Symbol *s)
{
    if (s->texture) {
	SDL_DestroyTexture(s->texture);
    }
    free(s);
}

void symbol_quit(Window *win)
{
    for (int i=0; i<sizeof(SYMBOL_TABLE) / sizeof(Symbol *); i++) {
	symbol_destroy(SYMBOL_TABLE[i]);
    }

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
	SDL_SetRenderDrawColor(s->window->rend, 0, 0, 0, 0);
	SDL_RenderClear(s->window->rend);

	/* SDL_SetRenderDrawColor(s->window->rend, 0, 0, 0, 0); */
	/* SDL_RenderClear(s->window->rend); */
	s->draw_fn((void *)s);
	SDL_SetRenderTarget(s->window->rend, saved_targ);
	symbol_draw(s, dst);
    } else {
	SDL_RenderCopy(s->window->rend, s->texture, NULL, dst);
    }
}

void symbol_draw_w_bckgrnd(Symbol *s, SDL_Rect *dst, SDL_Color *bckgrnd)
{
    SDL_SetRenderDrawColor(s->window->rend, sdl_colorp_expand(bckgrnd));
    geom_fill_rounded_rect(s->window->rend, dst, s->corner_rad_pix);
    symbol_draw(s, dst);
}

