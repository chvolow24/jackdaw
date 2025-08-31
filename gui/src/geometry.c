#include <stdbool.h>
#include "SDL.h"

#define RT2_OVER_2 0.70710678118;

/* Draw a circle quadrant. Quad 0 = upper right, 1 = upper left, 2 = lower left, 3 = lower right */
static void draw_quadrant(SDL_Renderer *rend, int xinit, int yinit, int r, const register uint8_t quad)
{
    r--;
    switch (quad) {
    case 0:
	yinit--;
	break;
    case 1:
	yinit--;
	xinit--;
	break;
    case 2:
	xinit--;
	break;
    case 3:
	break;
    }
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
            d += (x<<1) - (y<<1);
            y--;
        } else {
            d += (x<<1);
        }
        x++;
    }
}

/* Draw and fill a quadrant. quad 0 = upper right, 1 = upper left, 2 = lower left, 3 = lower right*/
static void fill_quadrant(SDL_Renderer *rend, int xinit, int yinit, int r, const register uint8_t quad)
{
    r--;
    switch (quad) {
    case 0:
	yinit--;
	break;
    case 1:
	yinit--;
	xinit--;
	break;
    case 2:
	xinit--;
	break;
    case 3:
	break;
    }
    int x = 0;
    int y = r;
    int d = 1 - r;
    int fill_x = 0;
    /* fprintf(stdout, "\n\n"); */
    while (x <= y) {
	/* fprintf(stdout, "X & y: %d, %d, (quad %d)\n", x, y, quad); */
        if (d>0) {
            switch(quad) {
                case 0:
                    SDL_RenderDrawLine(rend, xinit, yinit - y, xinit + x, yinit - y);
                    SDL_RenderDrawLine(rend, xinit + y, yinit, xinit + y, yinit - x);
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
            d += (x<<1) - (y<<1);
            y--;
        } else {
            d += (x<<1);
        }
        x++;
    }
    fill_x = x;
    SDL_Rect fill = {xinit, yinit, fill_x, fill_x};
    switch (quad) {
        case 0:
            fill.y -= fill_x - 1;
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

static void fill_quadrant_complement(SDL_Renderer *rend, int xinit, int yinit, int r, const register uint8_t quad)
{

    r--;

    switch (quad) {
    case 0:
	yinit--;
	break;
    case 1:
	yinit--;
	xinit--;
	break;
    case 2:
	xinit--;
	break;
    case 3:
	break;
    }

    int x = 0;
    int y = r;
    int d = 1-r;
    int outer_x;
    int outer_y;
    int inner_rect_dim = (r + 1) * RT2_OVER_2;
    while (x <= y) {
        if (d>0) {
            switch(quad) {
                case 0:
		    outer_x = xinit + inner_rect_dim;
		    outer_y = yinit - r;
		    SDL_RenderDrawLine(rend, xinit + x, yinit - y, outer_x, yinit - y);
		    SDL_RenderDrawLine(rend, xinit + y, yinit - x, xinit + y, outer_y);
                    break;
                case 1:
		    outer_x = xinit - inner_rect_dim;;
		    outer_y = yinit - r;
		    SDL_RenderDrawLine(rend, outer_x, yinit - y, xinit - x, yinit - y);
                    SDL_RenderDrawLine(rend, xinit - y, yinit-x, xinit - y, outer_y);
                    break;
                case 2:
		    outer_x = xinit - inner_rect_dim;
		    outer_y = yinit + r;
		    SDL_RenderDrawLine(rend, xinit - x, yinit + y, outer_x, yinit + y);
                    SDL_RenderDrawLine(rend, xinit - y, yinit + x, xinit - y, outer_y);
                    break;
                case 3:
		    outer_x = xinit + inner_rect_dim;
		    outer_y = yinit + r;
                    SDL_RenderDrawLine(rend, xinit + x, yinit + y, outer_x, yinit + y);
                    SDL_RenderDrawLine(rend, xinit + y, yinit + x, xinit + y, outer_y);
                    break;
            }
            /* Select SE coordinate */
            d += (x<<1) - (y<<1);
            y--;
        } else {
            d += (x<<1);
        }
        x++;
    }
}

void geom_draw_circle(SDL_Renderer *rend, int orig_x, int orig_y, int r)
{
    int center_x = orig_x + r;
    int center_y = orig_y + r;
    draw_quadrant(rend, center_x, center_y, r, 1);
    draw_quadrant(rend, center_x, center_y, r, 0);
    draw_quadrant(rend, center_x, center_y, r, 2);
    draw_quadrant(rend, center_x, center_y, r, 3);
}

void geom_fill_circle(SDL_Renderer *rend , int orig_x, int orig_y, int r)
{   
    int center_x = orig_x + r;
    int center_y = orig_y + r;
    fill_quadrant(rend, center_x, center_y, r, 1);
    fill_quadrant(rend, center_x, center_y, r, 0);
    fill_quadrant(rend, center_x, center_y, r, 2);
    fill_quadrant(rend, center_x, center_y, r, 3);
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
    SDL_RenderDrawLine(rend, left_x, lower_y + r - 1, right_x, lower_y + r - 1);
    /* left */
    SDL_RenderDrawLine(rend, rect->x, upper_y, rect->x, lower_y);
    /* right */
    SDL_RenderDrawLine(rend, right_x + r - 1, upper_y, right_x + r - 1, lower_y);  
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
    
    SDL_Rect top = {left_x , rect->y, rect->w - d , r};
    SDL_Rect bottom = {left_x , lower_y, rect->w - d , r };
    SDL_Rect left = {rect->x, upper_y , r, rect->h - d };
    SDL_Rect right = {right_x, upper_y , r , rect->h - d };
    SDL_Rect middle = {left_x, upper_y, rect->w - d, rect->h - d};

    SDL_RenderFillRect(rend, &top);
   
    
    SDL_RenderFillRect(rend, &bottom);
    SDL_RenderFillRect(rend, &right);
    SDL_RenderFillRect(rend, &left);
    SDL_RenderFillRect(rend, &middle);
}


void geom_draw_rect_thick(SDL_Renderer *rend, SDL_Rect *rect, int thickness, double dpi_scale_factor)
{
    thickness *= dpi_scale_factor;
    SDL_Rect temprect = *rect;
    for (int i=0; i<thickness; i++) {

	SDL_RenderDrawRect(rend, &temprect);
	temprect.x += 1;
	temprect.y += 1;
	temprect.w -= 2;
	temprect.h -= 2;
    }
}

void geom_draw_rounded_rect_thick(SDL_Renderer *rend, SDL_Rect *rect, int thickness, int r, double dpi_scale_factor)
{
    thickness *= dpi_scale_factor;
    SDL_Rect temprect = *rect;
    for (int i=0; i<thickness; i++) {
	geom_draw_rounded_rect(rend, &temprect, r);
	temprect.x += 1;
	temprect.y += 1;
	temprect.w -= 2;
	temprect.h -= 2;
    }


}


void geom_draw_tab(SDL_Renderer *rend, SDL_Rect *rect, int r, double dpi_scale_factor)
{
    r *= dpi_scale_factor;

    /* Lower left */
    int center_x = rect->x - r;
    int center_y = rect->y + rect->h - r;
    draw_quadrant(rend, center_x, center_y, r, 3);
    

    /* Lower right */
    center_x = rect->x + rect->w + r;
    center_y = rect->y + rect->h - r;
    draw_quadrant(rend, center_x, center_y, r, 2);

    /* Upper left */
    center_x = rect->x + r;
    center_y = rect->y + r;
    draw_quadrant(rend, center_x, center_y, r, 1);

    /* Upper right */
    center_x = rect->x + rect->w - r;
    draw_quadrant(rend, center_x, center_y, r, 0);

    /* Straight lines */
    int x1,x2,y1,y2;
    x1 = rect->x + r;
    x2 = rect->x + rect->w - r;
    y1 = rect->y;
    SDL_RenderDrawLine(rend, x1, y1, x2, y1);

    y1 += r;
    y2 = rect->y + rect->h - r;
    x1 = rect->x;
    x2 += r;
    SDL_RenderDrawLine(rend, x1, y1, x1, y2);
    SDL_RenderDrawLine(rend, x2, y1, x2, y2);
}

void geom_fill_tab(SDL_Renderer *rend, SDL_Rect *rect, int r, double dpi_scale_factor)
{
    r *= dpi_scale_factor;

    /* Lower left */
    int center_x = rect->x - r;
    int center_y = rect->y + rect->h - r;
    fill_quadrant_complement(rend, center_x, center_y, r, 3);
    

    /* Lower right */
    center_x = rect->x + rect->w + r;
    center_y = rect->y + rect->h - r;
    fill_quadrant_complement(rend, center_x, center_y, r, 2);

    /* Upper left */
    center_x = rect->x + r;
    center_y = rect->y + r;
    fill_quadrant(rend, center_x, center_y, r, 1);

    /* Upper right */
    center_x = rect->x + rect->w - r;
    fill_quadrant(rend, center_x, center_y, r, 0);

    /* Top rect (between corners) */

    SDL_Rect temp = *rect;
    temp.x += r;
    temp.w -= (r<<1);
    temp.h = r;
    SDL_RenderFillRect(rend, &temp);

    /* Bottom rect */

    temp.x -= r;
    temp.y += r;
    temp.w = rect->w;
    temp.h = rect->h - r;
    SDL_RenderFillRect(rend, &temp);
}

/* if x_ptr provided, it is set to be in the bounding box */
bool geom_x_in_rect(int x, SDL_Rect *rect, int *x_ptr)
{
    if (x < rect->x) {
	if (x_ptr) *x_ptr = rect->x;
	return false;
    }
    if (x >= rect->x + rect->w) {
	if (x_ptr) *x_ptr = rect->x + rect->w - 1;
	return false;
    }
    return true;
}
