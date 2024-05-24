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

#include "layout.h"
#include "text.h"
#include "window.h"

#ifndef LT_DEV_MODE
#define LT_DEV_MODE 1
#endif

#define TXT_H 40
#define SNAP_TOLERANCE 7

#define SCROLL_STOP_THRESHOLD 5

#define WINDOW_PAD 100

#define NUM_LT_TYPES 6

/* extern Layout *main_lt; */
/* extern SDL_Color white; */
//extern TTF_Font *open_sans;

extern SDL_Color color_global_white;

extern Window *main_win;

void layout_set_name(Layout *lt, char *new_name)
{
    int len = strlen(new_name);
    if (len < MAX_LT_NAMELEN - 1) {
	strcpy(lt->name, new_name);
    }
}


const char *layout_get_dimtype_str(DimType dt)
{
    switch (dt) {
    case REL:
	return "REL";
    case ABS:
	return "ABS";
    case SCALE:
	return "SCALE";
    case COMPLEMENT:
	return "COMPLEMENT";
    case STACK:
	return "STACK";
    case PAD:
	return "PAD";
    default:
	fprintf(stderr, "Error: DimType has value %d, which is not a member of enum.\n", dt);
	exit(1);
    }
}

const char *layout_get_itertype_str(IteratorType iter_type) 
{
    switch (iter_type) {
    case HORIZONTAL:
	return "HORIZONTAL";
    case VERTICAL:
	return "VERTICAL";
    }
}

void layout_get_dimval_str(Dimension *dim, char *dst, int maxlen)
{
    switch (dim->type) {
    case ABS:
    case COMPLEMENT:
    case STACK:
    case PAD:
    case REL:
	snprintf(dst, maxlen - 1, "%d", dim->value.intval);
	break;
    case SCALE:
	snprintf(dst, maxlen - 1, "%f", dim->value.floatval);
	break;
    }
}

/**********************************************************************/
/* These functions TRY to get sibling, parent, first child, of
   subject layout but return the subject layout if not found. */


static Layout *get_last_sibling(Layout *lt)
{
    Layout *ret = NULL;
    if (lt->parent && lt->index > 0) {
	return lt->parent->children[lt->index - 1];
    }
    return ret;
}

Layout *layout_iterate_siblings(Layout *from, int direction)
{
    if (direction == 1) {
	if (from->parent && from->index < from->parent->num_children - 1) {
	    return from->parent->children[from->index + 1];
	}
    } else if (direction == -1) {
	if (from->parent && from->index > 0) {
	    return from->parent->children[from->index - 1];
	}
    }
    return from;
}

Layout *layout_get_first_child(Layout *parent)
{
    if (parent->num_children > 0) {
	return parent->children[0];
    }
    return parent;
}
	    
Layout *layout_get_parent(Layout *child)
{
    if (child->parent) {
	return child->parent;
    }
    return child;
}
/**********************************************************************/

static int get_edge_value_pixels(Layout *lt, Edge edge)
{
    switch (edge) {
        case TOP:
            return lt->rect.y;
        case RIGHT:
            return lt->rect.x + lt->rect.w;
        case BOTTOM:
            return lt->rect.y + lt->rect.h;
        case LEFT:
            return lt->rect.x;
        default:
            return 0;
    }
}

static SDL_Rect get_logical_rect(SDL_Rect *pixels_p)
{
    SDL_Rect logical;
    SDL_Rect pixels = *pixels_p;
    logical.x = pixels.x / main_win->dpi_scale_factor;
    logical.y = pixels.y / main_win->dpi_scale_factor;
    logical.w = pixels.w / main_win->dpi_scale_factor;
    logical.h = pixels.h / main_win->dpi_scale_factor;
    return logical;
}

void layout_set_wh_from_rect(Layout *lt)
{
    switch (lt->w.type) {
    case ABS:
	lt->w.value.intval = round((float)lt->rect.w / main_win->dpi_scale_factor);
	break;
    case REL:
	lt->w.value.intval = round((float) lt->rect.w / main_win->dpi_scale_factor);
	break;
    case SCALE:
	if (lt->parent) {
	    if (lt->parent->rect.w != 0) {
		lt->w.value.floatval = (float) lt->rect.w / lt->parent->rect.w;
	    }
	} else {
	    fprintf(stderr, "Error: attempting to set scaled dimension values on layout with no parent.\n");
	    return;
	}
	break;
    case COMPLEMENT:
	break; /* Vales are irrelevant for COMPLEMENT */
    case STACK:
	break;
    case PAD:
	break;
	    
		
    }

    switch (lt->h.type) {
    case ABS:
	lt->h.value.intval = round( (float)lt->rect.h / main_win->dpi_scale_factor);
	break;
    case REL:
	lt->h.value.intval = round((float)lt->rect.h / main_win->dpi_scale_factor);
	break;
    case SCALE:
	if (lt->parent) {
	    if (lt->parent->rect.h != 0) {
		lt->h.value.floatval = (float) lt->rect.h / lt->parent->rect.h;
	    }
	} else {
	    fprintf(stderr, "Error: attempting to set scaled dimension values on layout with no parent.\n");
	    // exit(1);
	}
	break;
    case COMPLEMENT:
	break;
    case STACK:
	break;
    case PAD:
	break;
	
    }
}  

void layout_set_values_from_rect(Layout *lt)
{
    switch (lt->x.type) {
    case ABS:
	lt->x.value.intval = round((float) lt->rect.x / main_win->dpi_scale_factor);
	break;
    case REL:
	if (lt->parent) {
	    lt->x.value.intval = round((float) (lt->rect.x - lt->parent->rect.x) / main_win->dpi_scale_factor) ;
	} else {
	    lt->x.value.intval = round((float) lt->rect.x / main_win->dpi_scale_factor);
	}
	break;
    case SCALE:
	if (lt->parent) {
	    if (lt->parent->rect.w != 0) {
		lt->x.value.floatval = ((float)lt->rect.x - lt->parent->rect.x) / lt->parent->rect.w;
	    }
	}
	break;
    case COMPLEMENT:
	break;
    case STACK: {
	if (lt->parent) {
	    Layout *last_sibling = get_last_sibling(lt);
	    if (last_sibling) {
		lt->x.value.intval = round((float)(lt->rect.x - last_sibling->rect.x - last_sibling->rect.w) / main_win->dpi_scale_factor);
	    } else {
		lt->x.value.intval = round((float)(lt->rect.x - lt->parent->rect.x) / main_win->dpi_scale_factor);
	    }
	}
	break;
    }
    case PAD:
	break;
    }
    switch (lt->y.type) {
    case ABS:
	lt->y.value.intval = round((float)lt->rect.y / main_win->dpi_scale_factor);
	break;
    case REL:
	if (lt->parent) {
	    lt->y.value.intval = round(((float) lt->rect.y - lt->parent->rect.y)/main_win->dpi_scale_factor);
	} else {
	    lt->y.value.intval = round((float) lt->rect.y / main_win->dpi_scale_factor);
	}
	break;
    case SCALE:
	if (lt->parent) {
	    if (lt->parent->rect.h != 0) {
		lt->y.value.floatval = ((float)lt->rect.y - lt->parent->rect.y) / lt->parent->rect.h;
	    }
	} else {
	    fprintf(stderr, "Error: attempting to set scaled dimension values on layout with no parent.\n");
	    // exit(1);
	}
	break;
    case COMPLEMENT:
	break;
    case STACK: {
	Layout *last_sibling = get_last_sibling(lt);
	if (last_sibling) {
	    lt->y.value.intval = (lt->rect.y - last_sibling->rect.y - last_sibling->rect.h) / main_win->dpi_scale_factor;
	} else {
	    lt->y.value.intval = (lt->rect.y - lt->parent->rect.y) / main_win->dpi_scale_factor;
	}
    }
	break;
    case PAD:
	break;
    }

    layout_set_wh_from_rect(lt);
    /* switch (lt->w.type) { */
    /*     case ABS: */
    /*         lt->w.value.intval = round((float)lt->rect.w / main_win->dpi_scale_factor); */
    /*         break; */
    /*     case REL: */
    /*         lt->w.value.intval = round((float) lt->rect.w / main_win->dpi_scale_factor); */
    /*         break; */
    /*     case SCALE: */
    /*         if (lt->parent) { */
    /*             if (lt->parent->rect.w != 0) { */
    /*                 lt->w.value.floatval = (float) lt->rect.w / lt->parent->rect.w; */
    /*             } */
    /*         } else { */
    /*             fprintf(stderr, "Error: attempting to set scaled dimension values on layout with no parent.\n"); */
    /* 		return; */
    /*             // exit(1); */
    /*         } */
    /*         break; */
    /*     case COMPLEMENT: */
    /* 	    break; /\* Vales are irrelevant for COMPLEMENT *\/ */
	    
		
    /* } */

    /* switch (lt->h.type) { */
    /*     case ABS: */
    /*         lt->h.value.intval = round( (float)lt->rect.h / main_win->dpi_scale_factor); */
    /*         break; */
    /*     case REL: */
    /*         lt->h.value.intval = round((float)lt->rect.h / main_win->dpi_scale_factor); */
    /*         break; */
    /*     case SCALE: */
    /*         if (lt->parent) { */
    /*             if (lt->parent->rect.h != 0) { */
    /*                 lt->h.value.floatval = (float) lt->rect.h / lt->parent->rect.h; */
    /*             } */
    /*         } else { */
    /*             fprintf(stderr, "Error: attempting to set scaled dimension values on layout with no parent.\n"); */
    /*             // exit(1); */
    /*         } */
    /*         break; */
    /*     case COMPLEMENT: */
    /* 	    break; */
	
    /* } */

}

static Layout *layout_top_parent(Layout *lt)
{
    while (lt->parent) {
	lt = lt->parent;
    }
    return lt;

}

void do_snap(Layout *lt, Layout *main, Edge edge);
void layout_set_edge(Layout *lt, Edge edge, int set_to, bool block_snap) 
{
    switch (edge) {
    case TOP:
	lt->rect.h = lt->rect.y + lt->rect.h - set_to;
	lt->rect.y = set_to;
	break;
    case BOTTOM:
	lt->rect.h = set_to - lt->rect.y;
	break;
    case LEFT:
	lt->rect.w = lt->rect.x + lt->rect.w - set_to;
	lt->rect.x = set_to;
	break;
    case RIGHT:
	lt->rect.w = set_to - lt->rect.x;
	break;
    case NONEEDGE:
	break;
    }
    if (!block_snap) {
        do_snap(lt, layout_top_parent(lt), edge);
    }
    layout_set_values_from_rect(lt);
    layout_reset(lt);
}

/* Get the outermost scrollable layout at the mouse point. If none, return null. */
static Layout *get_scrollable_layout_at_point(Layout *lt, SDL_Point *mousep)
{
    Layout *ret = NULL;
    if (lt->hidden) {
	ret = NULL;
    } else if (!lt->parent && SDL_PointInRect(mousep, &(lt->rect))) {
	for (int i=0; !ret && i<lt->num_children; i++) {
	    ret = get_scrollable_layout_at_point(lt->children[i], mousep);
	}
    } else if (lt->iterator && lt->iterator->scrollable) {
	return lt;
    } else if (SDL_PointInRect(mousep, &(lt->rect))) {
	for (int i=0; !ret && i<lt->num_children; i++) {
	    ret = get_scrollable_layout_at_point(lt->children[i], mousep);
	}
    }
    return ret;
}
	    	    

void reset_iterations(LayoutIterator *iter);
/* Assumes the correct scrollable layout has been found (see 'get_scrollable_layout_at_point') */
static void handle_scroll_internal(Layout *lt, float scroll_x, float scroll_y, bool dynamic)
{

    lt->iterator->scroll_stop_count = 0;
    float offset_increment = lt->iterator->type == VERTICAL ? scroll_y : scroll_x;
    float new_offset = offset_increment + lt->iterator->scroll_offset;
    int iter_dim = lt->iterator->type == VERTICAL ? lt->iterator->total_height_pixels : lt->iterator->total_width_pixels;

    if (new_offset < 0) {
	lt->iterator->scroll_momentum = 0;
	lt->iterator->scroll_offset = 0;
	/* return; */
    } else if (new_offset > iter_dim) {
	lt->iterator->scroll_momentum = 0;
	lt->iterator->scroll_offset = iter_dim;
	/* return; */
    } else {
	lt->iterator->scroll_offset = new_offset;
    }
	   
    if (dynamic) {
	lt->iterator->scroll_momentum = offset_increment;
    }
    reset_iterations(lt->iterator);
}

Layout *layout_handle_scroll(Layout *main_lt, SDL_Point *mousep, float scroll_x, float scroll_y, bool dynamic)
{
    Layout *scrollable = get_scrollable_layout_at_point(main_lt, mousep);
    if (!scrollable) return NULL;
    
    handle_scroll_internal(scrollable, scroll_x, scroll_y, dynamic);
    return scrollable;
}


void reset_iterations(LayoutIterator *iter);
/* Run every frame on scrollable layout to reset scroll offset and momentum. Return 1 if scroll should continue, 0 if it's done */
int layout_scroll_step(Layout *lt)
{
    if (!lt->iterator || !lt->iterator->scrollable || lt->iterator->scroll_momentum == 0) return 0;

    int iter_dim = lt->iterator->type == VERTICAL ? lt->iterator->total_height_pixels : lt->iterator->total_width_pixels;
    
    lt->iterator->scroll_momentum += (lt->iterator->scroll_momentum < 0 ? 1 : -1);
    if (lt->iterator->scroll_stop_count > SCROLL_STOP_THRESHOLD) {
	lt->iterator->scroll_momentum = 0;
	return 0;
    }
    else if (fabs(lt->iterator->scroll_momentum) < 1) {
	lt->iterator->scroll_momentum = 0;
	return 0;
    }
    if (lt->iterator->scroll_offset + lt->iterator->scroll_momentum < 0) {
	lt->iterator->scroll_offset = 0;
	lt->iterator->scroll_momentum = 0;
	reset_iterations(lt->iterator);
	return 0;
    } else if (lt->iterator->scroll_offset + lt->iterator->scroll_momentum > iter_dim) {
	lt->iterator->scroll_offset = iter_dim;
	lt->iterator->scroll_momentum = 0;
	return 0;
    } else {
	lt->iterator->scroll_offset += lt->iterator->scroll_momentum;
	reset_iterations(lt->iterator);
	return 1;
    }
}

void layout_stop_scroll(Layout *lt)
{
    if (lt->iterator) {
	lt->iterator->scroll_momentum = 0;
    }
}
				      

void layout_set_corner(Layout *lt, Corner crnr, int x, int y, bool block_snap)
{
    switch (crnr) {
    case TPLFT:
	layout_set_edge(lt, LEFT, x, block_snap);
	layout_set_edge(lt, TOP, y, block_snap);
	break;
    case BTTMLFT:
	layout_set_edge(lt, LEFT, x, block_snap);
	layout_set_edge(lt, BOTTOM, y, block_snap);
	break;
    case BTTMRT:
	layout_set_edge(lt, BOTTOM, y, block_snap);
	layout_set_edge(lt, RIGHT, x, block_snap);
	break;
    case TPRT:
	layout_set_edge(lt, RIGHT, x, block_snap);
	layout_set_edge(lt, TOP, y, block_snap);
	break;
    case NONECRNR:
	break;
    }
}

static int check_snap_x_recursive(Layout *main, int check_x, int *distance)
{
    int temp_dist, ret_x;
    bool result_found = false;
    if (main->type == PRGRM_INTERNAL || main->type == ITERATION) {
        return -1;
    }
    int compare_x = get_edge_value_pixels(main, LEFT);
    if ((temp_dist = abs(compare_x - check_x)) < *distance && temp_dist != 0) {
        *distance = temp_dist;
        result_found = true;
        ret_x = compare_x;
    }
    compare_x = get_edge_value_pixels(main, RIGHT);
    if ((temp_dist = abs(compare_x - check_x)) < *distance && temp_dist != 0) {
        *distance = temp_dist;
        result_found = true;
        ret_x = compare_x;
    }
    for (int i=0; i<main->num_children; i++) {
        Layout *child = main->children[i];
        int childret = check_snap_x_recursive(child, check_x, distance);
        if (childret > 0) {
            result_found = true;
            ret_x = childret;
        }
    }
    if (result_found) {
        return ret_x;
    } else {
        return -1;
    }
}

static int check_snap_y_recursive(Layout *main, int check_y, int *distance)
{
    int temp_dist, ret_y;
    bool result_found = false;
    if (main->type == PRGRM_INTERNAL || main->type == ITERATION) {
        return -1;
    }
    int compare_y = get_edge_value_pixels(main, TOP);
    if ((temp_dist = abs(compare_y - check_y)) < *distance && temp_dist != 0) {
        *distance = temp_dist;
        ret_y = compare_y;
        result_found = true;
    }
    compare_y = get_edge_value_pixels(main, BOTTOM);
    if ((temp_dist = abs(compare_y - check_y)) < *distance && temp_dist != 0) {
        *distance = temp_dist;
        ret_y = compare_y;
        result_found = true;
    }
    for (int i=0; i<main->num_children; i++) {
        Layout *child = main->children[i];
        int childret = check_snap_y_recursive(child, check_y, distance);
        if (childret > 0) {
            ret_y = childret;
            result_found = true;
        }
    }
    if (result_found) {
        return ret_y;
    } else {
        return -1;
    }
}

/* return -1 if no snap, otherwise return x coord to snap to */
int check_snap_x(Layout *main, int check_x)
{
    int distance = SNAP_TOLERANCE;
    return check_snap_x_recursive(main, check_x, &distance);
}

int check_snap_y(Layout *main, int check_y)
{
    int distance = SNAP_TOLERANCE;
    return check_snap_y_recursive(main, check_y, &distance);
}

void do_snap(Layout *lt, Layout *main, Edge check_edge)
{
    switch (check_edge) {
    case TOP:
    case BOTTOM: {
	int check_y = get_edge_value_pixels(lt, check_edge);
	int y_snap = check_snap_y(main, check_y);
	if (y_snap > 0) {
	    layout_set_edge(lt, check_edge, y_snap, true);
	}
	break;
    }
    case LEFT:
    case RIGHT: {
	int check_x = get_edge_value_pixels(lt, check_edge);
	int x_snap = check_snap_x(main, check_x);
	if (x_snap > 0) {
	    layout_set_edge(lt, check_edge, x_snap, true);
	}
	break;
    }
    case NONEEDGE:
	return;
    }
}

void do_snap_translate(Layout *lt, Layout *main)
{
    int check_val = get_edge_value_pixels(lt, LEFT);
    int snap = check_snap_x(main, check_val);
    if (snap > 0) {
        layout_set_position_pixels(lt, snap, lt->rect.y, true);
        goto verticals;
        // return;
    }
    check_val = get_edge_value_pixels(lt, RIGHT);
    snap = check_snap_x(main, check_val);
    if (snap > 0) {
        layout_set_position_pixels(lt, snap - lt->rect.w, lt->rect.y, true);
        // return;
    }
    verticals:
    check_val = get_edge_value_pixels(lt, TOP);
    snap = check_snap_y(main, check_val);
    if (snap > 0) {
        layout_set_position_pixels(lt, lt->rect.x, snap, true);
        // return;
    }
    check_val = get_edge_value_pixels(lt, BOTTOM);
    snap = check_snap_y(main, check_val);
    if (snap > 0) {
        layout_set_position_pixels(lt, lt->rect.x, snap - lt->rect.h, true);
        // return;
    }
}

void layout_set_position_pixels(Layout *lt, int x, int y, bool block_snap) 
{
    lt->rect.x = x;
    lt->rect.y = y;
    if (!block_snap) {
        do_snap_translate(lt, layout_top_parent(lt));
    }
    layout_set_values_from_rect(lt);
    layout_reset(lt);

}

void layout_move_position(Layout *lt, int move_by_x, int move_by_y, bool block_snap)
{

    lt->rect.x += move_by_x;
    lt->rect.y += move_by_y;
    if (!block_snap) {
        do_snap_translate(lt, layout_top_parent(lt));

    }
    layout_set_values_from_rect(lt);
    layout_reset(lt);

}

int set_rect_xy(Layout *lt)
{
    switch (lt->x.type) {
    case ABS:
	lt->rect.x = lt->x.value.intval * main_win->dpi_scale_factor;
	break;
    case REL:
	lt->rect.x = lt->parent->rect.x + lt->x.value.intval * main_win->dpi_scale_factor;
	break;
    case SCALE:
	lt->rect.x = lt->parent->rect.x + lt->parent->rect.w * lt->x.value.floatval;
	break;
    case COMPLEMENT:
	return 0;
	break;
    case STACK: {
	Layout *last_sibling = get_last_sibling(lt);
	if (last_sibling) {
	    lt->rect.x = last_sibling->rect.x + last_sibling->rect.w + lt->x.value.intval * main_win->dpi_scale_factor;
	} else {
	    lt->rect.x = lt->parent->rect.x + lt->x.value.intval * main_win->dpi_scale_factor;
	}
	break;
    }
    case PAD:
	break;
    }
    switch (lt->y.type) {
    case ABS:
	lt->rect.y = lt->y.value.intval * main_win->dpi_scale_factor;
	break;
    case REL:
	// fprintf(stderr, "Y is rel. ")
	lt->rect.y = lt->parent->rect.y + lt->y.value.intval * main_win->dpi_scale_factor;
	break;
    case SCALE:
	lt->rect.y = lt->parent->rect.y + lt->parent->rect.h * lt->y.value.floatval;
	break;
    case COMPLEMENT:
	return 0;
	break;
    case STACK: {
	Layout *last_sibling = get_last_sibling(lt);
	if (last_sibling) {
	    lt->rect.y = last_sibling->rect.y + last_sibling->rect.h + lt->y.value.intval * main_win->dpi_scale_factor;
	} else {
	    lt->rect.y = lt->parent->rect.y + lt->y.value.intval * main_win->dpi_scale_factor;
	}
	break;
    }
    case PAD:
	break;
    }
    return 1;
}

int set_rect_wh(Layout *lt)
{
    switch (lt->w.type) {
    case ABS:
    case REL:
	lt->rect.w = lt->w.value.intval * main_win->dpi_scale_factor;
	break;
    case SCALE:
	lt->rect.w = round(((float) lt->parent->rect.w) * lt->w.value.floatval);
	break;
    case COMPLEMENT: {
	Layout *last_sibling = lt->parent->children[lt->index - 1];
	while (last_sibling && last_sibling->w.type == COMPLEMENT && last_sibling->index > 0) {
	    last_sibling = last_sibling->parent->children[last_sibling->index - 1];
	}
	if (!last_sibling) {
	    fprintf(stderr, "Error: layout %s has type dim COMPLEMENT but no last sibling\n", lt->name);
	    return 0;
	}
	lt->rect.w = lt->parent->rect.w - last_sibling->rect.w;
	break;
    }
    case STACK:
	break;
    case PAD:
	if (!lt->parent) {
	    break;
	}
	set_rect_xy(lt);
	lt->rect.w = lt->parent->rect.w - 2 * (lt->rect.x - lt->parent->rect.x);
	break;

    }
    switch (lt->h.type) {
    case ABS:
    case REL:
	lt->rect.h = lt->h.value.intval * main_win->dpi_scale_factor;
	break;
    case SCALE:
	lt->rect.h = round(((float) lt->parent->rect.h) * lt->h.value.floatval);
	break;
    case COMPLEMENT: {
	Layout *last_sibling = lt->parent->children[lt->index - 1];
	while (last_sibling && last_sibling->h.type == COMPLEMENT && last_sibling->index > 0) {
	    last_sibling = last_sibling->parent->children[last_sibling->index - 1];
	}

	if (!last_sibling) {
	    fprintf(stderr, "Error: layout %s has type dim COMPLEMENT but no last sibling\n", lt->name);
	    return 0;
	}
	lt->rect.h = lt->parent->rect.h - last_sibling->rect.h;
	break;
    }
    case STACK:
	break;
    case PAD:
	if (!lt->parent) {
	    break;
	}
	set_rect_xy(lt);
	lt->rect.h = lt->parent->rect.h - 2 * (lt->rect.y - lt->parent->rect.y);
	break;
    }
    return 1;
}

void reset_iterations(LayoutIterator *iter);


/* New iterative implementation */
void layout_force_reset(Layout *lt)
{
    if (lt->hidden) {
	return;
    }
    Layout *top_parent = lt;
    
    while (lt) {
	/* DO CALCS */
	if (lt->parent) {
	    if (!set_rect_wh(lt)) {
		fprintf(stderr, "Error: failed to set wh on %s\n", lt->name);
	    }
	    if (!(set_rect_xy(lt))) {
		fprintf(stderr, "Error: failed to set xy on %s\n", lt->name);
	    }
	}
	if (lt->namelabel) {
	    lt->label_rect = (SDL_Rect) {lt->rect.x, lt->rect.y - TXT_H, 0, 0};
	    txt_reset_display_value(lt->namelabel);
	}
	if (lt->iterator) {
	    reset_iterations(lt->iterator);
	}


	/* TRAVERSE TREE */

	if (lt->num_children > 0) {
	    lt = lt->children[0];
	} else if (lt->parent) {
	    while (lt != top_parent && lt->parent->num_children == lt->index + 1) { /* Is last sibling */
		lt = lt->parent; /* locally update lt pointer, but do no calcs */
	    }
	    if (lt == top_parent) {
		return;
	    } else {
		lt = lt->parent->children[lt->index + 1]; /* Next sibling */
	    }
	} else {
	    lt = NULL;
	}
	    
    }
    
}

/* Old recursive implementation */
void layout_reset(Layout *lt)
{
    if (lt->hidden) {
	return;
    }
    if (lt->parent) {
        if (!set_rect_wh(lt)) {
            fprintf(stderr, "Error: failed to set wh on %s\n", lt->name);
        }
        if (!(set_rect_xy(lt))) {
            fprintf(stderr, "Error: failed to set xy on %s\n", lt->name);
        }
    }
    lt->label_rect = (SDL_Rect) {lt->rect.x, lt->rect.y - TXT_H, 0, 0};
    if (lt->namelabel) {	
	txt_reset_display_value(lt->namelabel);
    }
    if (!main_win || !main_win->layout) {
	return;
    }
    SDL_Rect padded_win = main_win->layout->rect;
    padded_win.x -= WINDOW_PAD;
    padded_win.y -= WINDOW_PAD;
    padded_win.w += WINDOW_PAD * 2;
    padded_win.h += WINDOW_PAD * 2;

    if (SDL_HasIntersection(&lt->rect, &padded_win)) {
	for (uint8_t i=0; i<lt->num_children; i++) {
	    Layout *child = lt->children[i];
	    layout_reset(child);
	}

	if (lt->iterator) {
	    reset_iterations(lt->iterator);
	}
    }
}

Layout *layout_create()
{
    if (!main_win || !main_win->std_font) {
	fprintf(stderr, "Cannot create a layout from a window without a standard font initialized. Make sure to call window_assign_std_font after creating a window.\n");
	exit(1);
    }
	
    /* TTF_Font *open_sans_12 = ttf_get_font_at_size(main_win->std_font, 12); */
    Layout *lt = malloc(sizeof(Layout));
    lt->num_children = 0;
    lt->index = 0;
    lt->name[0] = 'L';
    lt->name[1] = 't';
    lt->name[2] = '\0';
    lt->selected = false;
    lt->type = NORMAL;
    lt->iterator = NULL;

    lt->x.type = REL;
    lt->y.type = REL;
    lt->w.type = ABS;
    lt->h.type = ABS;
    lt->x.value.intval = 0;
    lt->y.value.intval = 0;
    lt->w.value.intval = 0;
    lt->h.value.intval = 0;
    // lt->display = true;
    // lt->internal = false;
    lt->parent = NULL;
    lt->rect = (SDL_Rect) {0,0,0,0};
    lt->label_rect = (SDL_Rect) {0,0,0,0};
    if (LT_DEV_MODE) {
	lt->namelabel = txt_create_from_str(lt->name, MAX_LT_NAMELEN, &(lt->label_rect), main_win->std_font, 12, color_global_white, CENTER_LEFT, false, main_win);
    } else {
	lt->namelabel = NULL;
    }
    lt->hidden = false;
    return lt;
}

void delete_iterator(LayoutIterator *iter);
void layout_destroy(Layout *lt)
{
    if (lt->parent && lt->type != ITERATION) {
        for (uint8_t i=lt->index; i<lt->parent->num_children - 1; i++) {
            lt->parent->children[i] = lt->parent->children[i + 1];
            lt->parent->children[i]->index--;
        }
        lt->parent->num_children--;
    }
    if (lt->type == TEMPLATE && lt->iterator) {
        delete_iterator(lt->iterator);
	lt->iterator = NULL;
    }
    for (uint8_t i=0; i<lt->num_children; i++) {
        layout_destroy(lt->children[i]);
    }
    if (lt->namelabel) {
	txt_destroy(lt->namelabel);
    }
    free(lt);
}

Layout *layout_create_from_window(Window *win)
{
    DimVal dv;
    Layout *lt = layout_create();
    dv.intval = 0;
    lt->x = (Dimension) {ABS, dv};
    lt->y = (Dimension) {ABS, dv};
    dv.intval = win->w;
    lt->w = (Dimension) {ABS, dv};
    dv.intval = win->h;
    lt->h = (Dimension) {ABS, dv};
    lt->rect = (SDL_Rect) {0, 0, win->w, win->h};
    lt->label_rect = (SDL_Rect) {lt->rect.x, lt->rect.y - TXT_H, 0, 0};
    lt->index = 0;
    sprintf(lt->name, "main");
    return lt;
}

void layout_reset_from_window(Layout *lt, Window *win)
{
    DimVal dv;
    dv.intval = win->w;
    lt->w = (Dimension) {ABS, dv};
    dv.intval = win->h;
    lt->h = (Dimension) {ABS, dv};
    lt->rect.x = 0;
    lt->rect.y = 0;
    lt->rect.w = win->w;
    lt->rect.h = win->h;
    layout_reset(lt);
}

Layout *layout_add_child(Layout *parent)
{
    Layout *child = layout_create();
    child->parent = parent;
    parent->children[parent->num_children] = child;
    child->index = parent->num_children;
    parent->num_children++;
    snprintf(child->name, 32, "%s_child%d", parent->name, parent->num_children);
    // fprintf(stderr, "\t->done add child\n");
    return child;
}

Layout *layout_add_complementary_child(Layout *parent, RectMem comp_rm)
{
    Layout *new_child = layout_add_child(parent);
    /* layout_set_default_dims(new_child); */
    switch (comp_rm) {
    case X:
	new_child->x.type = COMPLEMENT;
	break;
    case Y:
	new_child->y.type = COMPLEMENT;
	break;
    case W:
	new_child->w.type = COMPLEMENT;
	break;
    case H:
	new_child->h.type = COMPLEMENT;
	break;
    }
    return new_child;
    
}

void layout_reparent(Layout *child, Layout *parent)
{
    child->parent = parent;
    parent->children[parent->num_children] = child;
    child->index = parent->num_children;
    parent->num_children++;
    layout_reset(child);
}

void layout_center_agnostic(Layout *lt, bool horizontal, bool vertical)
{
    if (horizontal) lt->rect.x = lt->parent->rect.x + lt->parent->rect.w / 2 - lt->rect.w / 2;
    if (vertical) lt->rect.y = lt->parent->rect.y + lt->parent->rect.h / 2 - lt->rect.h / 2;
    layout_set_values_from_rect(lt);
}

void layout_center_relative(Layout *lt, bool horizontal, bool vertical)
{
    if (!lt->parent) {
	return;
    }
    if (horizontal) lt->x.type = REL;
    if (vertical) lt->y.type = REL;
    layout_center_agnostic(lt, horizontal, vertical);

}

void layout_center_scale(Layout *lt, bool horizontal, bool vertical)
{
    if (!lt->parent) {
	return;
    }
    if (horizontal) lt->x.type = SCALE;
    if (vertical) lt->y.type = SCALE;
    layout_center_agnostic(lt, horizontal, vertical);
}

void layout_set_default_dims(Layout *lt)
{
    DimVal dv;

    if (lt->parent) {
	SDL_Rect parent_logical = get_logical_rect(&(lt->parent->rect));
	lt->x = (Dimension) {REL, {parent_logical.w / 4}};
	lt->y = (Dimension) {REL, {parent_logical.h / 4}};
	lt->w = (Dimension) {REL, {parent_logical.w / 2}};
	lt->h = (Dimension) {REL, {parent_logical.h / 2}};
    } else {   
	dv.intval = 20;
	lt->x = (Dimension) {REL, dv};
	lt->y = (Dimension) {REL, dv};
	lt->w = (Dimension) {ABS, dv};
	lt->h = (Dimension) {ABS, dv};
    }
}
Layout *layout_get_child_by_name(Layout *lt, const char *name)
{
    for (uint8_t i=0; i<lt->num_children; i++) {
        if (strcmp(lt->children[i]->name, name) == 0) {
            return lt->children[i];
        }
    }
    return NULL;
}


Layout *layout_get_child_by_name_recursive(Layout *lt, const char *name)
{
    Layout *ret = NULL;
    if (strcmp(lt->name, name) == 0) {
        ret = lt;
    } else {
        for (uint8_t i=0; i<lt->num_children; i++) {
            ret = layout_get_child_by_name_recursive(lt->children[i], name);
            if (ret) {
                break;
            }
        }
    }
    return ret;
}

void layout_set_type_recursive(Layout *lt, LayoutType type)
{
    lt->type = type;
    for (int i=0; i<lt->num_children; i++) {
        layout_set_type_recursive(lt->children[i], type);
    }
}

void layout_pad(Layout *lt, int h_pad, int v_pad)
{
    Layout *parent = lt->parent;
    if (!parent) {
	return;
    }
    int h_pix_pad = h_pad * (main_win->dpi_scale_factor);
    int v_pix_pad = v_pad * (main_win->dpi_scale_factor);
    lt->rect.x = parent->rect.x + h_pix_pad;
    lt->rect.y = parent->rect.y + v_pix_pad;
    lt->rect.w = parent->rect.w - 2 * h_pix_pad;
    lt->rect.h = parent->rect.h - 2 * v_pix_pad;
    layout_set_values_from_rect(lt);

}



Layout *layout_deepest_at_point(Layout *search, SDL_Point *point)
{
    if (!(SDL_PointInRect(point, &(search->rect)))) {
	return NULL;
    }
    Layout *selected = search;
    Layout *test = NULL;
    for (int i=0; i<search->num_children; i++) {
	if ((test = layout_deepest_at_point(search->children[i], point))) {
	    selected = test;
	}
    }
    return selected;
}
	

void layout_toggle_dimension(Layout *lt, Dimension *dim, RectMem rm, SDL_Rect *rect, SDL_Rect *parent_rect)
{
    // fprintf(stderr, "ENTER TOGGLE\n");
    if (!parent_rect) {
        return;
    }
    dim->type++;
    dim->type %= NUM_LT_TYPES;
    layout_set_values_from_rect(lt);
}

void layout_size_to_fit_children(Layout *lt, bool fixed_origin, int padding)
{
    int min_x = main_win->layout->rect.w;
    int max_x = 0;
    int min_y = main_win->layout->rect.h;
    int max_y = 0;
    int right, bottom;
    for (uint8_t i=0; i<lt->num_children; i++) {
	Layout *child = lt->children[i];
	if (child->rect.x < min_x) min_x = child->rect.x;
	if (child->rect.y < min_y) min_y = child->rect.y;
	if ((right = child->rect.x + child->rect.w) > max_x) max_x = right;
	if ((bottom = child->rect.y + child->rect.h) > max_y) max_y = bottom;
    }
    if (!fixed_origin) {
	lt->rect.x = min_x - padding * main_win->dpi_scale_factor;
	lt->rect.y = min_y - padding * main_win->dpi_scale_factor;
    }
    lt->rect.w = max_x - lt->rect.x + padding * main_win->dpi_scale_factor;
    lt->rect.h = max_y - lt->rect.y + padding * main_win->dpi_scale_factor;
    layout_set_values_from_rect(lt);
}



/* Layout *template; */
/* IteratorType type; */
/* uint8_t num_iterations; */
/* Layout *iterations; */
LayoutIterator *create_iterator() 
{
    LayoutIterator *iter = malloc(sizeof(LayoutIterator));
    iter->type = VERTICAL;
    iter->template = NULL;
    iter->num_iterations = 0;
    iter->scrollable = false;
    iter->scroll_offset = 0;
    iter->scroll_momentum = 0;
    iter->total_height_pixels = 0;
    return iter;
}


LayoutIterator *copy_iterator(LayoutIterator *to_copy)
{
    LayoutIterator *copy = create_iterator();
    copy->template = to_copy->template;
    copy->type = to_copy->type;
    copy->num_iterations = to_copy->num_iterations;
    copy->scrollable = to_copy->scrollable;
    /* TODO: copy the iterations! */
    for (int i=0; i<to_copy->num_iterations; i++) {
	fprintf(stderr, "\tCopy iteration %d/%d\n", i, to_copy->num_iterations);
	Layout *iter_copy = layout_copy(to_copy->iterations[i], NULL);
	copy->iterations[i] = iter_copy;
    }
    return copy;
}
    
Layout *layout_copy(Layout *to_copy, Layout *parent)
{
    Layout *copy = layout_create();
    strcpy(copy->name, to_copy->name);
    copy->rect = to_copy->rect;
    copy->x = to_copy->x;
    copy->y = to_copy->y;
    copy->w = to_copy->w;
    copy->h = to_copy->h;
    copy->type = to_copy->type;
    /* TODO: make sure to copy iterator templates correctly! */

    if (parent) {
	layout_reparent(copy, parent);
    }
    for (int i=0; i<to_copy->num_children; i++) {
        layout_copy(to_copy->children[i], copy);
    }

    if (to_copy->type == TEMPLATE && to_copy->iterator) {
	copy->iterator = copy_iterator(to_copy->iterator);
	copy->iterator->template = copy;
    }

    return copy;

}

static Layout *copy_layout_as_iteration(Layout *to_copy, Layout *parent) 
{
    Layout *copy = layout_create();
    copy->type = ITERATION;
    strcpy(copy->name, to_copy->name);
    copy->rect = to_copy->rect;
    copy->x = to_copy->x;
    copy->y = to_copy->y;
    copy->w = to_copy->w;
    copy->h = to_copy->h;
    // copy->type = to_copy->type;
    copy->iterator = NULL;

    if (parent) {
	copy->parent = parent;
	// reparent(copy, parent);
    } else {
	/* fprintf(stderr, "Setting null parent on layout named %s\n", to_copy->name); */
	/* exit(1); */
	copy->parent = NULL;
//	copy->parent = parent;
    }
    for (int i=0; i<to_copy->num_children; i++) {
        Layout *copied_child = layout_copy(to_copy->children[i], copy);
	copied_child->type = ITERATION;
	copied_child->parent = copy;
        // reparent(child_copy, copy);
    }
    /* if (to_copy->iterator) { */
    /* 	fprintf(stderr, "IN copy layout as iteration, copying iterator.\n"); */
    /* 	copy->iterator = copy_iterator(to_copy->iterator); */
    /* 	copy->iterator->template = copy; */
    /* } */
    return copy;
}


void layout_write(FILE *f, Layout *lt, int indent);
static void add_iteration(LayoutIterator *iter)
{
    Layout *iteration = copy_layout_as_iteration(iter->template, NULL);/*iter->template->parent*/
    iteration->parent = iter->template;
    /* layout_write(stdout, iter->template, 0); */
    iter->iterations[iter->num_iterations] = iteration;
    /* layout_write(stdout, iteration, 0); */
    /* exit(0); */
    iter->num_iterations++;
    iteration->type = ITERATION;
    // reset_iteration_rects(iter);
}

static void remove_iteration(LayoutIterator *iter) 
{
    fprintf(stderr, "Remove iteration from %s\n", iter->template->name);
    if (iter->num_iterations > 0) {
	fprintf(stderr, "->deleting %p, not %p\n", iter->iterations[iter->num_iterations-1], iter->template);
        layout_destroy(iter->iterations[iter->num_iterations - 1]);
	iter->num_iterations--;
    }
}

void layout_remove_iter_at(LayoutIterator *iter, uint8_t at)
{
    layout_destroy(iter->iterations[at]);
    for (uint8_t i=at + 1; i<iter->num_iterations; i++) {
	iter->iterations[i-1] = iter->iterations[i];
    }
    iter->num_iterations--;

}

Layout *layout_add_iter(Layout *lt, IteratorType type, bool scrollable)
{
    if (!(lt->iterator)) {
        layout_create_iter_from_template(lt, type, 1, scrollable);
    } else {
        add_iteration(lt->iterator);
    }
    layout_reset(lt);
    return lt->iterator->iterations[lt->iterator->num_iterations - 1];
}

void layout_remove_iter(Layout *lt)
{
    if (!(lt->iterator)) {
        return;
    }
    if (lt->iterator->num_iterations == 1) {
        delete_iterator(lt->iterator);
	lt->iterator = NULL;
        layout_set_type_recursive(lt, NORMAL);
        // lt->type = NORMAL;
    } else {
        remove_iteration(lt->iterator);
    }
    layout_reset(lt);
}

void reset_iterations(LayoutIterator *iter)
{
    int x = iter->template->rect.x;
    int y = iter->template->rect.y;
    int w = iter->template->rect.w;
    int h = iter->template->rect.h;
    if (!iter->template->parent) {
        fprintf(stderr, "Error: iterator template has no parent\n");
        return;
    }
    int x_dist_from_parent = x - iter->template->parent->rect.x;
    int y_dist_from_parent = y - iter->template->parent->rect.y;
    iter->total_width_pixels = (w + x_dist_from_parent) * iter->num_iterations;
    iter->total_height_pixels = (h + y_dist_from_parent) * iter->num_iterations; 
    int scroll_offset = iter->scrollable ? iter->scroll_offset : 0;
    for (int i=0; i<iter->num_iterations; i++) {
        Layout *iteration = iter->iterations[i];
        iteration->rect.x = x;
        iteration->rect.y = y;
        iteration->rect.w = w;
        iteration->rect.h = h;
//        set_values_from_rect(iteration);
        switch (iter->type) {
            case VERTICAL:
		iteration->rect.y -= scroll_offset;
                y += h + y_dist_from_parent;
                break;
            case HORIZONTAL:
		iteration->rect.x -= scroll_offset;
                x += w + x_dist_from_parent;
                break;
        }
	for (int i=0; i<iteration->num_children; i++) {
	    layout_reset(iteration->children[i]);
	}
    }
}


LayoutIterator *layout_create_iter_from_template(Layout *template, IteratorType type, int num_iterations, bool scrollable) 
{
    template->type = TEMPLATE;
    LayoutIterator *iter = create_iterator();

    template->iterator = iter;

    iter->template = template;
    iter->type = type;
    iter->scrollable = scrollable;

    for (int i=0; i<num_iterations; i++) {
        add_iteration(iter);
    }

    if (iter->template->parent) { /* During xml read, iteration template won't yet have parent */
        reset_iterations(iter);
    }

    return iter;
}

void delete_iterator(LayoutIterator *iter)
{
    for (int i=0; i<iter->num_iterations; i++) {
        layout_destroy(iter->iterations[i]);
    }
    free(iter);
}

void layout_fprint(FILE *f, Layout *lt)
{
    char xval[255], yval[255], wval[255], hval[255];
    layout_get_dimval_str(&(lt->x), xval, 255);
    layout_get_dimval_str(&(lt->y), yval, 255);
    layout_get_dimval_str(&(lt->w), wval, 255);
    layout_get_dimval_str(&(lt->h), hval, 255);

    fprintf(f, "Layout %s\n\tx: %s %s\n\ty: %s %s\n\tw: %s %s\n\th: %s %s\n",
	    lt->name,
	    layout_get_dimtype_str(lt->x.type),
	    xval,
	    layout_get_dimtype_str(lt->y.type),
	    yval,
	    layout_get_dimtype_str(lt->w.type),
	    wval,
	    layout_get_dimtype_str(lt->h.type),
	    hval
	);
}


/***************************************************/
/******************** DRAW CODE ********************/
/***************************************************/
SDL_Color iter_clr = {0, 100, 100, 255};
SDL_Color rect_clrs[2] = {
    {255, 0, 0, 255},
    {0, 255, 0, 255}
};
SDL_Color rect_clrs_dttd[2] = {
    {255, 0, 0, 100},
    {0, 255, 0, 100}
};

#define DTTD_LN_LEN 20

static void draw_dotted_horizontal(SDL_Renderer *rend, int x1, int x2, int y)
{
    while (x1 < x2) {
        SDL_RenderDrawLine(rend, x1, y, x1 + DTTD_LN_LEN, y);
        x1 += DTTD_LN_LEN * 2;
    }
}

static void draw_dotted_vertical(SDL_Renderer *rend, int x, int y1, int y2)
{
    while (y1 < y2) {
        SDL_RenderDrawLine(rend, x, y1, x, y1 + DTTD_LN_LEN);
        y1 += DTTD_LN_LEN * 2;
    }
}


void layout_draw(Window *win, Layout *lt)
{   
    if (lt->type == PRGRM_INTERNAL) {
        return;
    }

    if (lt->iterator) {
        for (int i=0; i<lt->iterator->num_iterations; i++) {
            layout_draw(win, lt->iterator->iterations[i]);
        }
    }
    
    SDL_Color picked_clr;
    if (lt->type == ITERATION) {
        picked_clr = iter_clr;
    } else {
        picked_clr = lt->selected ? rect_clrs[1] : rect_clrs[0];
    }

    SDL_SetRenderDrawColor(win->rend, picked_clr.r, picked_clr.g, picked_clr.b, picked_clr.a);

    if (lt->selected) {
	if (lt->namelabel) {
	    txt_draw(lt->namelabel);
	}
	    
        SDL_RenderDrawRect(win->rend, &(lt->label_rect));
    }
    SDL_RenderDrawRect(win->rend, &(lt->rect));

    if (lt->type != ITERATION) {
	SDL_Color dotted_clr = lt->selected ? rect_clrs_dttd[1] : rect_clrs_dttd[0];
        SDL_SetRenderDrawColor(win->rend, dotted_clr.r, dotted_clr.g, dotted_clr.b, dotted_clr.a);
        draw_dotted_horizontal(win->rend, 0, win->w, lt->rect.y);
        draw_dotted_horizontal(win->rend, 0, win->w, lt->rect.y + lt->rect.h);
        draw_dotted_vertical(win->rend, lt->rect.x, 0, win->h);
        draw_dotted_vertical(win->rend, lt->rect.x + lt->rect.w, 0, win->h);
    }


    for (uint8_t i=0; i<lt->num_children; i++) {
        layout_draw(win, lt->children[i]);
    }

}


