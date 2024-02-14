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
    }
}

void get_val_str(Dimension *dim, char *dst, int maxlen)
{
    switch (dim->type) {
        case ABS:
        case REL:
            snprintf(dst, maxlen - 1, "%d", dim->value.intval);
            break;
        case SCALE:
            snprintf(dst, maxlen - 1, "%f", dim->value.floatval);

    }
}

static int get_rect_dim_val_from_dim(Dimension dim, int parent_rect_coord, int parent_rect_dim)
{
    switch (dim.type) {
        case ABS:
            return dim.value.intval;
        case REL:
            return dim.value.intval;
        case SCALE:
            return dim.value.floatval * parent_rect_dim;
    }
}

static int get_rect_coord_val_from_dim(Dimension dim, int parent_rect_coord, int parent_rect_dim)
{
    switch (dim.type) {
        case ABS:
            return dim.value.intval;
        case REL:
            return dim.value.intval + parent_rect_coord;
        case SCALE:
            return parent_rect_coord + dim.value.floatval * parent_rect_dim;
    }
}

// static void set_dimval_from_draw_coord(Dimension *dim, RectMem rm, int draw_coord)
// {
//     switch (rm) {
//         case X:
//         case Y: {
//             switch (dim->type) {
//                 case ABS:
//                     dim->value.intval = draw_coord;
//                     break;
//                 case REL:
//                     dim->value.intval = draw_coord - dim

//             }
//         }
//             break;
//         case W:
//         case H:
//             break;
//     }
// }

static int get_edge_value(Layout *lt, Edge edge)
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

static void set_values_from_rect(Layout *lt)
{
    switch (lt->x.type) {
        case ABS:
            lt->x.value.intval = lt->rect.x;
            break;
        case REL:
            if (lt->parent) {
                lt->x.value.intval = lt->rect.x - lt->parent->rect.x;
            } else {
                lt->x.value.intval = lt->rect.x;
            }
            break;
        case SCALE:
            if (lt->parent) {
                if (lt->parent->rect.w != 0) {
                    lt->x.value.floatval = ((double)lt->rect.x - lt->parent->rect.x)/lt->parent->rect.w;
                }
            }
            //TODO: need else here?
            break;
    }
    switch (lt->y.type) {
        case ABS:
            lt->y.value.intval = lt->rect.y;
            break;
        case REL:
            if (lt->parent) {
                lt->y.value.intval = lt->rect.y - lt->parent->rect.y;
            } else {
                lt->y.value.intval = lt->rect.y;
            }
            break;
        case SCALE:
            if (lt->parent) {
                if (lt->parent->rect.h != 0) {
                    lt->y.value.floatval = ((double)lt->rect.y - lt->parent->rect.y)/lt->parent->rect.h;
                }
            }
            //TODO: need else here?
            break;
    }

    switch (lt->w.type) {
        case ABS:
            lt->w.value.intval = lt->rect.w;
            break;
        case REL:
            lt->w.value.intval = lt->rect.w;
            break;
        case SCALE:
            if (lt->parent) {
                if (lt->parent->rect.w != 0) {
                    lt->w.value.floatval = (double)lt->rect.w / lt->parent->rect.w;
                }
            }
            //TODO: need else here?
            break;
    }

    switch (lt->h.type) {
        case ABS:
            lt->h.value.intval = lt->rect.h;
            break;
        case REL:
            lt->h.value.intval = lt->rect.h;
            break;
        case SCALE:
            if (lt->parent) {
                if (lt->parent->rect.h != 0) {
                    lt->h.value.floatval = (double)lt->rect.h / lt->parent->rect.h;
                }
            }
            //TODO: need else here?
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
    if (main->type != NORMAL) {
        return -1;
    }
    int compare_x = get_edge_value(main, LEFT);
    if ((temp_dist = abs(compare_x - check_x)) < *distance && temp_dist != 0) {
        *distance = temp_dist;
        result_found = true;
        ret_x = compare_x;
    }
    compare_x = get_edge_value(main, RIGHT);
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
    if (main->type != NORMAL) {
        return -1;
    }
    int compare_y = get_edge_value(main, TOP);
    if ((temp_dist = abs(compare_y - check_y)) < *distance && temp_dist != 0) {
        *distance = temp_dist;
        ret_y = compare_y;
        result_found = true;
    }
    compare_y = get_edge_value(main, BOTTOM);
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
            int check_y = get_edge_value(lt, check_edge);
            int y_snap = check_snap_y(main, check_y);
            if (y_snap > 0) {
                set_edge(lt, check_edge, y_snap, true);
            }
            break;
        }
        case LEFT:
        case RIGHT: {
            int check_x = get_edge_value(lt, check_edge);
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
    int check_val = get_edge_value(lt, LEFT);
    int snap = check_snap_x(main, check_val);
    if (snap > 0) {
        set_position(lt, snap, lt->rect.y, true);
        goto verticals;
        // return;
    }
    check_val = get_edge_value(lt, RIGHT);
    snap = check_snap_x(main, check_val);
    if (snap > 0) {
        set_position(lt, snap - lt->rect.w, lt->rect.y, true);
        // return;
    }
    verticals:
    check_val = get_edge_value(lt, TOP);
    snap = check_snap_y(main, check_val);
    if (snap > 0) {
        set_position(lt, lt->rect.x, snap, true);
        // return;
    }
    check_val = get_edge_value(lt, BOTTOM);
    snap = check_snap_y(main, check_val);
    if (snap > 0) {
        set_position(lt, lt->rect.x, snap - lt->rect.h, true);
        // return;
    }
}

void set_position(Layout *lt, int x, int y, bool block_snap) 
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

void reset_iterations(LayoutIterator *iter);
void reset_layout(Layout *lt)
{
    if (lt->parent) {
        SDL_Rect parent_rect = lt->parent->rect;
        lt->rect.x = get_rect_coord_val_from_dim(lt->x, parent_rect.x, parent_rect.w);
        lt->rect.y = get_rect_coord_val_from_dim(lt->y, parent_rect.y, parent_rect.h);
        lt->rect.w = get_rect_dim_val_from_dim(lt->w, parent_rect.x, parent_rect.w);
        lt->rect.h = get_rect_dim_val_from_dim(lt->h, parent_rect.y, parent_rect.h);
    }
    lt->label_rect = (SDL_Rect) {lt->rect.x, lt->rect.y - TXT_H, 0, 0};
    reset_text_display_value(lt->namelabel);

    for (uint8_t i=0; i<lt->num_children; i++) {
        Layout *child = lt->children[i];
        reset_layout(child);
    }

    if (lt->type == TEMPLATE) {
        reset_iterations(lt->iterator);
        for (int i=0; i<lt->iterator->num_iterations; i++) {
            reset_layout(lt->iterator->iterations[i]);
        }
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
    fprintf(stderr, "\t\t->Delete layout at %p\n", lt);
    if (lt->parent) {
        for (uint8_t i=lt->index; i<lt->parent->num_children - 1; i++) {
            lt->parent->children[i] = lt->parent->children[i + 1];
            lt->parent->children[i]->index--;
        }
        lt->parent->num_children--;
    }
    if (lt->type == TEMPLATE && lt->iterator) {
        delete_iterator(lt->iterator);
    }
    for (uint8_t i=0; i<lt->num_children; i++) {
        delete_layout(lt->children[i]);
    }
    fprintf(stderr, "\t\t->ABOUT to free %p\n", lt);

    free(lt);
    fprintf(stderr, "\t\t->Freed%p\n", lt);

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
    dv.intval = 100;
    lt->x = (Dimension) {REL, dv};
    lt->y = (Dimension) {REL, dv};
    lt->w = (Dimension) {ABS, dv};
    lt->h = (Dimension) {ABS, dv};
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

void toggle_dimension(Dimension *dim, RectMem rm, SDL_Rect *rect, SDL_Rect *parent_rect)
{
    // fprintf(stderr, "ENTER TOGGLE\n");
    if (!parent_rect) {
        return;
    }
    dim->type++;
    dim->type %= 3;
    switch(rm) {
        case X:
            switch(dim->type) {
                case ABS:
                    dim->value.intval = rect->x;
                    break;
                case REL:
                    dim->value.intval = rect->x - parent_rect->x;
                    break;
                case SCALE:
                    dim->value.floatval = ((float)rect->x - parent_rect->x) / parent_rect->w;
                    break;
            }
            break;
        case Y:
            switch(dim->type) {
                case ABS:
                    dim->value.intval = rect->y;
                    break;
                case REL:
                    dim->value.intval = rect->y - parent_rect->y;
                    break;
                case SCALE:
                    dim->value.floatval = ((float)rect->y - parent_rect->y) / parent_rect->h;
                    break;
            }
            break;
        case W:
            switch(dim->type) {
                case ABS:
                    dim->value.intval = rect->w;
                    break;
                case REL:
                    dim->value.intval = rect->w;
                    break;
                case SCALE:
                    dim->value.floatval = (float)rect->w / parent_rect->w;
                    break;
            }
            break;
        case H:
            switch(dim->type) {
                case ABS:
                    dim->value.intval = rect->h;
                    break;
                case REL:
                    dim->value.intval = rect->h;
                    break;
                case SCALE:
                    dim->value.floatval = (float)rect->h / parent_rect->h;
                    break;
            }
            break;             
    }
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
    return iter;
}

// void reset_iteration_rects(LayoutIterator *iter)
// {
// // static int get_rect_coord_val_from_dim(Dimension dim, int parent_rect_coord, int parent_rect_dim)

//     int x = iter->template->rect.x;
//     int y = iter->template->rect.x;
//     int w = iter->template->rect.x;
//     int h = iter->template->rect.x;

//     int x_dist_from_parent = x - iter->template->parent->rect.x;
//     int y_dist_from_parent = y - iter->template->parent->rect.y;

//     // Dimension x = iter->template->x;
//     // Dimension y = iter->template->y;
//     // Dimension w = iter->template->w;
//     // Dimension h = iter->template->h;
//     for (int i=0; i<iter->num_iterations; i++) {
//         SDL_Rect *iteration = iter->iterations[i];
//         iteration->x = x;
//         iteration->y = y;
//         iteration->w = w;
//         iteration->h = h;
//         switch (iter->type) {
//             case VERTICAL:
//                 y += h + y_dist_from_parent;
//             break;
//             case HORIZONTAL:    
//                 x += w + x_dist_from_parent;
//             break;
//         }

//     }
// }

// typedef struct layout {
//     SDL_Rect rect;
//     Dimension x;
//     Dimension y;
//     Dimension w;
//     Dimension h;
//     char name[MAX_LT_NAMELEN];
//     Layout *parent;
//     Layout *children[MAX_CHILDREN];
//     uint8_t num_children;
//     uint8_t index;
//     SDL_Rect label_rect;
//     Text *namelabel;
//     bool selected;
//     LayoutType type;
//     LayoutIterator *iterator; /* If the layout type == TEMPLATE, this is not null */
//     // bool display;
//     // bool internal;
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

    reparent(copy, parent);
    for (int i=0; i<to_copy->num_children; i++) {
        copy_layout(to_copy->children[i], copy);
        // reparent(child_copy, copy);
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

    reparent(copy, parent);
    for (int i=0; i<to_copy->num_children; i++) {
        copy_layout_as_iteration(to_copy->children[i], copy);
        // reparent(child_copy, copy);
    }
    fprintf(stderr, "Returning copy iteration of type %d\n", copy->type);
    return copy;
}


void add_iteration(LayoutIterator *iter)
{
    Layout *iteration = copy_layout_as_iteration(iter->template, iter->template->parent);

    iter->iterations[iter->num_iterations] = iteration;
    iter->num_iterations++;
    iteration->type = ITERATION;
    // reset_iteration_rects(iter);
}
void reset_iterations(LayoutIterator *iter)
{

    int x = iter->template->rect.x;
    int y = iter->template->rect.y;
    int w = iter->template->rect.w;
    int h = iter->template->rect.h;
    int x_dist_from_parent = x - iter->template->parent->rect.x;
    int y_dist_from_parent = y - iter->template->parent->rect.y;
    for (int i=0; i<iter->num_iterations; i++) {
        Layout *iteration = iter->iterations[i];
        iteration->rect.x = x;
        iteration->rect.y = y;
        iteration->rect.w = w;
        iteration->rect.h = h;
        set_values_from_rect(iteration);
        switch (iter->type) {
            case VERTICAL:
                y += h + y_dist_from_parent;
                break;
            case HORIZONTAL:
                x += w + x_dist_from_parent;
                break;
        }
    }
}


LayoutIterator *create_iterator_from_template(Layout *template, IteratorType type, int num_iterations) 
{
    template->type = TEMPLATE;
    LayoutIterator *iter = create_iterator();
    template->iterator = iter;
    iter->template = template;
    // iter->num_iterations = num_iterations;
    for (int i=0; i<num_iterations; i++) {
        add_iteration(iter);
    }
    reset_iterations(iter);
    return iter;
}

void delete_iterator(LayoutIterator *iter)
{
    for (int i=0; i<iter->num_iterations; i++) {
        delete_layout(iter->iterations[i]);
    }
    free(iter);
}