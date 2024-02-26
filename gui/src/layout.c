#include "layout.h"
#include "text.h"
#include "window.h"

#define TXT_H 40
#define SNAP_TOLERANCE 7

extern Layout *main_lt;
extern SDL_Color white;
extern TTF_Font *open_sans;
extern Window *main_win;

const char *get_dimtype_str(DimType dt)
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
    }
}

const char *get_itertype_str(IteratorType iter_type) 
{
    switch (iter_type) {
        case HORIZONTAL:
            return "HORIZONTAL";
        case VERTICAL:
            return "VERTICAL";
    }
}

void get_val_str(Dimension *dim, char *dst, int maxlen)
{
    switch (dim->type) {
        case ABS:
        case COMPLEMENT:
        case REL:
            snprintf(dst, maxlen - 1, "%d", dim->value.intval);
            break;
        case SCALE:
            snprintf(dst, maxlen - 1, "%f", dim->value.floatval);
        
    }
}

/**********************************************************************/
/* These functions TRY to get sibling, parent, first child, of
   subject layout but return the subject layout if not found. */

Layout *iterate_siblings(Layout *from, int direction)
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

Layout *get_first_child(Layout *parent)
{
    if (parent->num_children > 0) {
	return parent->children[0];
    }
    return parent;
}
	    
Layout *get_parent(Layout *child)
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


static void set_values_from_rect(Layout *lt)
{
    // fprintf(stderr, "Set values from rect\n");
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
    }

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
                // exit(1);
            }
            break;
        case COMPLEMENT:
	    break; /* Vales are irrelevant for COMPLEMENT */
	    
		
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
	
    }

}

void do_snap(Layout *lt, Layout *main, Edge edge);
void set_edge(Layout *lt, Edge edge, int set_to, bool block_snap) 
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
        do_snap(lt, main_lt, edge);
    }
    set_values_from_rect(lt);
    reset_layout(lt);

}

void handle_scroll(Layout *lt, SDL_Point *mousep, int scroll_x, int scroll_y)
{
    if (lt->type == PRGRM_INTERNAL) {
        return;
    } else if (
        lt->type == TEMPLATE
        && lt->iterator->scrollable
        && SDL_PointInRect(mousep, &(lt->parent->rect))
    ) {
        switch(lt->iterator->type) {
            case VERTICAL:
                lt->iterator->scroll_momentum += scroll_y;
                break;
            case HORIZONTAL:
                lt->iterator->scroll_momentum += scroll_x;
                break;
        }
    } else {
        for (int i=0; i<lt->num_children; i++) {
            handle_scroll(lt->children[i], mousep, scroll_x, scroll_y);
        }
    }
}

void set_corner(Layout *lt, Corner crnr, int x, int y, bool block_snap)
{
    switch (crnr) {
        case TPLFT:
            set_edge(lt, LEFT, x, block_snap);
            set_edge(lt, TOP, y, block_snap);
            break;
        case BTTMLFT:
            set_edge(lt, LEFT, x, block_snap);
            set_edge(lt, BOTTOM, y, block_snap);
            break;
        case BTTMRT:
            set_edge(lt, BOTTOM, y, block_snap);
            set_edge(lt, RIGHT, x, block_snap);
            break;
        case TPRT:
            set_edge(lt, RIGHT, x, block_snap);
            set_edge(lt, TOP, y, block_snap);
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
                set_edge(lt, check_edge, y_snap, true);
            }
            break;
        }
        case LEFT:
        case RIGHT: {
            int check_x = get_edge_value_pixels(lt, check_edge);
            int x_snap = check_snap_x(main, check_x);
            if (x_snap > 0) {
                set_edge(lt, check_edge, x_snap, true);
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
        set_position_pixels(lt, snap, lt->rect.y, true);
        goto verticals;
        // return;
    }
    check_val = get_edge_value_pixels(lt, RIGHT);
    snap = check_snap_x(main, check_val);
    if (snap > 0) {
        set_position_pixels(lt, snap - lt->rect.w, lt->rect.y, true);
        // return;
    }
    verticals:
    check_val = get_edge_value_pixels(lt, TOP);
    snap = check_snap_y(main, check_val);
    if (snap > 0) {
        set_position_pixels(lt, lt->rect.x, snap, true);
        // return;
    }
    check_val = get_edge_value_pixels(lt, BOTTOM);
    snap = check_snap_y(main, check_val);
    if (snap > 0) {
        set_position_pixels(lt, lt->rect.x, snap - lt->rect.h, true);
        // return;
    }
}

void set_position_pixels(Layout *lt, int x, int y, bool block_snap) 
{
    lt->rect.x = x;
    lt->rect.y = y;
    if (!block_snap) {
        do_snap_translate(lt, main_lt);
    }
    set_values_from_rect(lt);
    reset_layout(lt);

}

void move_position(Layout *lt, int move_by_x, int move_by_y, bool block_snap)
{
    lt->rect.x += move_by_x;
    lt->rect.y += move_by_y;
    if (!block_snap) {
        do_snap_translate(lt, main_lt);

    }
    set_values_from_rect(lt);
    reset_layout(lt);
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
            if (!last_sibling) {
                fprintf(stderr, "Error: layout %s has type dim COMPLEMENT but no last sibling\n", lt->name);
                return 0;
            }
            lt->rect.w = lt->parent->rect.w - last_sibling->rect.w;
            break;
        }
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
            if (!last_sibling) {
                fprintf(stderr, "Error: layout %s has type dim COMPLEMENT but no last sibling\n", lt->name);
                return 0;
            }
            lt->rect.h = lt->parent->rect.h - last_sibling->rect.h;
            break;
        }
    }
    return 1;
}

void reset_iterations(LayoutIterator *iter);
void reset_layout(Layout *lt)
{
    if (lt->parent) {
        if (!set_rect_wh(lt)) {
            fprintf(stderr, "Error: failed to set wh on %s\n", lt->name);
        }
        if (!(set_rect_xy(lt))) {
            fprintf(stderr, "Error: failed to set xy on %s\n", lt->name);
        }
    }
    lt->label_rect = (SDL_Rect) {lt->rect.x, lt->rect.y - TXT_H, 0, 0};
    reset_text_display_value(lt->namelabel);

    for (uint8_t i=0; i<lt->num_children; i++) {
        Layout *child = lt->children[i];
        reset_layout(child);
    }

    if (lt->iterator) {
        reset_iterations(lt->iterator);
        /* for (int i=0; i<lt->iterator->num_iterations; i++) { */
        /*     reset_layout(lt->iterator->iterations[i]); */
        /* } */
    }
}

Layout *create_layout()
{
    Layout *lt = malloc(sizeof(Layout));
    lt->num_children = 0;
    lt->index = 0;
    lt->name[0] = 'L';
    lt->name[1] = 't';
    lt->name[2] = '\0';
    lt->selected = false;
    lt->type = NORMAL;
    lt->iterator = NULL;
    // lt->display = true;
    // lt->internal = false;
    lt->parent = NULL;
    lt->rect = (SDL_Rect) {0,0,0,0};
    lt->label_rect = (SDL_Rect) {0,0,0,0};
    lt->namelabel = create_text_from_str(lt->name, MAX_LT_NAMELEN, &(lt->label_rect), open_sans, white, CENTER_LEFT, false, main_win->rend);
    return lt;
}

void delete_iterator(LayoutIterator *iter);
void delete_layout(Layout *lt)
{
    fprintf(stderr, "Delete layout %s. Parent? %p\n", lt->name, lt->parent);
    if (lt->parent) {
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
        delete_layout(lt->children[i]);
    }
    free(lt);
}

Layout *create_layout_from_window(Window *win)
{
    DimVal dv;
    Layout *lt = create_layout();
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

void reset_layout_from_window(Layout *lt, Window *win)
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
    reset_layout(lt);
}

Layout *add_child(Layout *parent)
{
    // fprintf(stderr, "Add child to %s\n", parent->name);
    Layout *child = create_layout();
    child->parent = parent;
    parent->children[parent->num_children] = child;
    child->index = parent->num_children;
    parent->num_children++;
    snprintf(child->name, 32, "%s_child%d", parent->name, parent->num_children);
    // fprintf(stderr, "\t->done add child\n");
    return child;
}

Layout *add_complementary_child(Layout *parent, RectMem comp_rm)
{
    Layout *new_child = add_child(parent);
    set_default_dims(new_child);
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

void reparent(Layout *child, Layout *parent)
{
    child->parent = parent;
    parent->children[parent->num_children] = child;
    child->index = parent->num_children;
    parent->num_children++;
    reset_layout(child);
}

void set_default_dims(Layout *lt)
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
Layout *get_child(Layout *lt, const char *name)
{
    for (uint8_t i=0; i<lt->num_children; i++) {
        if (strcmp(lt->children[i]->name, name) == 0) {
            return lt->children[i];
        }
    }
    return NULL;
}


Layout *get_child_recursive(Layout *lt, const char *name)
{
    Layout *ret = NULL;
    if (strcmp(lt->name, name) == 0) {
        ret = lt;
    } else {
        for (uint8_t i=0; i<lt->num_children; i++) {
            ret = get_child_recursive(lt->children[i], name);
            if (ret) {
                break;
            }
        }
    }
    return ret;
}

void set_layout_type_recursive(Layout *lt, LayoutType type)
{
    lt->type = type;
    for (int i=0; i<lt->num_children; i++) {
        set_layout_type_recursive(lt->children[i], type);
    }
}


Layout *deepest_layout_at_point(Layout *search, SDL_Point *point)
{
    if (!(SDL_PointInRect(point, &(search->rect)))) {
	return NULL;
    }
    Layout *selected = search;
    Layout *test = NULL;
    for (int i=0; i<search->num_children; i++) {
	if ((test = deepest_layout_at_point(search->children[i], point))) {
	    selected = test;
	}
    }
    return selected;
}
	

void toggle_dimension(Layout *lt, Dimension *dim, RectMem rm, SDL_Rect *rect, SDL_Rect *parent_rect)
{
    // fprintf(stderr, "ENTER TOGGLE\n");
    if (!parent_rect) {
        return;
    }
    dim->type++;
    dim->type %= 3;
    set_values_from_rect(lt);
}

    Layout *template;
    IteratorType type;
    uint8_t num_iterations;
    Layout *iterations;
LayoutIterator *create_iterator() 
{
    LayoutIterator *iter = malloc(sizeof(LayoutIterator));
    iter->type = VERTICAL;
    iter->template = NULL;
    iter->num_iterations = 0;
    iter->scrollable = false;
    iter->scroll_offset = 0;
    iter->scroll_momentum = 0;
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
	Layout *iter_copy = copy_layout(to_copy->iterations[i], NULL);
	copy->iterations[i] = iter_copy;
    }
    return copy;
}
    
Layout *copy_layout(Layout *to_copy, Layout *parent)
{
    Layout *copy = create_layout();
    strcpy(copy->name, to_copy->name);
    copy->rect = to_copy->rect;
    copy->x = to_copy->x;
    copy->y = to_copy->y;
    copy->w = to_copy->w;
    copy->h = to_copy->h;
    copy->type = to_copy->type;
    /* TODO: make sure to copy iterator templates correctly! */

    if (parent) {
	reparent(copy, parent);
    }
    for (int i=0; i<to_copy->num_children; i++) {
        copy_layout(to_copy->children[i], copy);
    }

    if (to_copy->type == TEMPLATE && to_copy->iterator) {
	copy->iterator = copy_iterator(to_copy->iterator);
	copy->iterator->template = copy;
    }

    return copy;

}

static Layout *copy_layout_as_iteration(Layout *to_copy, Layout *parent) 
{
    Layout *copy = create_layout();
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
        Layout *copied_child = copy_layout(to_copy->children[i], copy);
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


static void add_iteration(LayoutIterator *iter)
{
    Layout *iteration = copy_layout_as_iteration(iter->template, NULL);/*iter->template->parent*/

    iter->iterations[iter->num_iterations] = iteration;
    iter->num_iterations++;
    iteration->type = ITERATION;
    // reset_iteration_rects(iter);
}

static void remove_iteration(LayoutIterator *iter) 
{
    fprintf(stderr, "Remove iteration from %s\n", iter->template->name);
    if (iter->num_iterations > 0) {
	fprintf(stderr, "->deleting %p, not %p\n", iter->iterations[iter->num_iterations-1], iter->template);
        delete_layout(iter->iterations[iter->num_iterations - 1]);
	iter->num_iterations--;
    }
}

void add_iteration_to_layout(Layout *lt, IteratorType type, bool scrollable)
{
    if (!(lt->iterator)) {
        create_iterator_from_template(lt, type, 1, scrollable);
    } else {
        add_iteration(lt->iterator);
    }
    reset_layout(lt);
}

void remove_iteration_from_layout(Layout *lt)
{
    if (!(lt->iterator)) {
        return;
    }
    if (lt->iterator->num_iterations == 1) {
        delete_iterator(lt->iterator);
	lt->iterator = NULL;
        set_layout_type_recursive(lt, NORMAL);
        // lt->type = NORMAL;
    } else {
        remove_iteration(lt->iterator);
    }
    reset_layout(lt);
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
    for (int i=0; i<iter->num_iterations; i++) {
        Layout *iteration = iter->iterations[i];
        iteration->rect.x = x;
        iteration->rect.y = y;
        iteration->rect.w = w;
        iteration->rect.h = h;
//        set_values_from_rect(iteration);
        switch (iter->type) {
            case VERTICAL:
                y += h + y_dist_from_parent;
                break;
            case HORIZONTAL:
                x += w + x_dist_from_parent;
                break;
        }
	for (int i=0; i<iteration->num_children; i++) {
	    reset_layout(iteration->children[i]);
	}
    }
}


LayoutIterator *create_iterator_from_template(Layout *template, IteratorType type, int num_iterations, bool scrollable) 
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
        delete_layout(iter->iterations[i]);
    }
    free(iter);
}
