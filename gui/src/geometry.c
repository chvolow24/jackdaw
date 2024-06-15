#include "SDL.h"

/* Draw a circle quadrant. Quad 0 = upper right, 1 = upper left, 2 = lower left, 3 = lower right */
static void draw_quadrant(SDL_Renderer *rend, int xinit, int yinit, int r, const register uint8_t quad)
{
    int x = 0;
    int y = r;
    int d = 1-r;
    while (x <= y) {
        switch(quad) {
            case 0:
                SDL_RenderDrawPoint(rend, xinit + x, yinit - y);
                SDL_RenderDrawPoint(rend, xinit + y, yinit  - x);
                break;
            case 1:
                SDL_RenderDrawPoint(rend, xinit - x, yinit - y);
                SDL_RenderDrawPoint(rend, xinit - y, yinit - x);
                break;
            case 2:
                SDL_RenderDrawPoint(rend, xinit - x, yinit + y);
                SDL_RenderDrawPoint(rend, xinit - y, yinit + x);
                break;
            case 3:
                SDL_RenderDrawPoint(rend, xinit + x, yinit + y);
                SDL_RenderDrawPoint(rend, xinit + y, yinit + x);
                break;
        }
        if (d>0) {
            /* Select SE coordinate */
            d += (x<<1) - (y<<1) + 5;
            y--;
        } else {
            d += (x<<1) + 3;
        }
        x++;
    }
}

/* Draw and fill a quadrant. quad 0 = upper right, 1 = upper left, 2 = lower left, 3 = lower right*/
static void fill_quadrant(SDL_Renderer *rend, int xinit, int yinit, int r, const register uint8_t quad)
{
    int x = 0;
    int y = r;
    int d = 1-r;
    int fill_x = 0;

    while (x <= y) {
        if (d>0) {
            switch(quad) {
                case 0:
                    SDL_RenderDrawLine(rend, xinit, yinit - y, xinit + x, yinit - y);
                    SDL_RenderDrawLine(rend, xinit + y, yinit, xinit + y, yinit  - x);
                    break;
                case 1:
                    SDL_RenderDrawLine(rend, xinit, yinit - y, xinit - x, yinit - y);
                    SDL_RenderDrawLine(rend, xinit - y, yinit, xinit - y, yinit - x);
                    break;
                case 2:
                    SDL_RenderDrawLine(rend, xinit, yinit + y, xinit - x, yinit + y);
                    SDL_RenderDrawLine(rend, xinit - y, yinit, xinit - y, yinit + x);
                    break;
                case 3:
                    SDL_RenderDrawLine(rend, xinit, yinit + y, xinit + x, yinit + y);
                    SDL_RenderDrawLine(rend, xinit + y, yinit, xinit + y, yinit + x);
                    break;
            }
            /* Select SE coordinate */
            d += (x<<1) - (y<<1) + 5;
            y--;
        } else {
            d += (x<<1) + 3;
        }
        x++;
    }
    fill_x = x;

    SDL_Rect fill = {xinit, yinit, fill_x, fill_x};
    switch (quad) {
        case 0:
            fill.y -= fill_x -1;
            break;
        case 1:
            fill.x -= fill_x - 1;
            fill.y -= fill_x - 1;
            break;
        case 2:
            fill.x -= fill_x - 1;
            break;       
        case 3:
            break;
        default:
            break;
    }
    SDL_RenderFillRect(rend, &fill);
}

void geom_draw_rounded_rect(SDL_Renderer *rend, SDL_Rect *rect, int r)
{
    int left_x = rect->x + r;
    int right_x = rect->x + rect->w -r;
    int upper_y = rect->y + r;
    int lower_y = rect->y + rect->h - r;
    SDL_Point ul = {left_x, upper_y};
    SDL_Point ur = {right_x, upper_y};
    SDL_Point ll = {left_x, lower_y};
    SDL_Point lr = {right_x, lower_y};
    draw_quadrant(rend, ur.x, ur.y, r, 0);
    draw_quadrant(rend, ul.x, ul.y, r, 1);
    draw_quadrant(rend, ll.x, ll.y, r, 2);
    draw_quadrant(rend, lr.x, lr.y, r, 3);
    /* top */
    SDL_RenderDrawLine(rend, left_x, rect->y, right_x, rect->y);
    /* bottom */
    SDL_RenderDrawLine(rend, left_x, lower_y + r, right_x, lower_y + r);
    /* left */
    SDL_RenderDrawLine(rend, rect->x, upper_y, rect->x, lower_y);
    /* right */
    SDL_RenderDrawLine(rend, right_x + r, upper_y, right_x + r, lower_y);  
}

void geom_fill_rounded_rect(SDL_Renderer *rend, SDL_Rect *rect, int r)
{
    int left_x = rect->x + r;
    int right_x = rect->x + rect->w - r;
    int upper_y = rect->y + r;
    int lower_y = rect->y + rect->h - r;
    SDL_Point ul = {left_x, upper_y};
    SDL_Point ur = {right_x, upper_y};
    SDL_Point ll = {left_x, lower_y};
    SDL_Point lr = {right_x, lower_y};
    fill_quadrant(rend, ur.x, ur.y, r, 0);
    fill_quadrant(rend, ul.x, ul.y, r, 1);
    fill_quadrant(rend, ll.x, ll.y, r, 2);
    fill_quadrant(rend, lr.x, lr.y, r, 3);
    int d = r<<1; // decision criterion
    SDL_Rect top = {left_x + 1, rect->y, rect->w - d - 1, r};
    SDL_Rect bottom = {left_x + 1, lower_y, rect->w - d - 1, r + 1};
    SDL_Rect left = {rect->x, upper_y + 1, r, rect->h - d - 1};
    SDL_Rect right = {right_x, upper_y + 1, r + 1, rect->h - d - 1};
    SDL_Rect middle = {left_x, upper_y, rect->w - d, rect->h - d};
    SDL_RenderFillRect(rend, &top);
    SDL_RenderFillRect(rend, &bottom);
    SDL_RenderFillRect(rend, &right);
    SDL_RenderFillRect(rend, &left);
    SDL_RenderFillRect(rend, &middle);
}


void geom_draw_rect_thick(SDL_Renderer *rend, SDL_Rect *rect, int r, double dpi_scale_factor)
{
    r *= dpi_scale_factor;
    SDL_Rect temprect = *rect;
    for (int i=0; i<r; i++) {

	SDL_RenderDrawRect(rend, &temprect);
	temprect.x += 1;
	temprect.y += 1;
	temprect.w -= 2;
	temprect.h -= 2;
    }
    
}

