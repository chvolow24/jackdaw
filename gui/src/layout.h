#ifndef JDAW_GUI_LAYOUT_H
#define JDAW_GUI_LAYOUT_H

#include <stdio.h>
#include <stdbool.h>
#include "SDL.h"
#include "text.h"
#include "window.h"

#define MAX_LT_CHILDREN INT64_MAX
#define MAX_LT_NAMELEN 64
#define MAX_LT_ITERATIONS 32

#define LAYOUT_SCROLL_SCALAR 8

/* Specifies an edge of a rectangle */
typedef enum edge {
    LEFT,
    RIGHT,
    TOP,
    BOTTOM,
    NONEEDGE
} Edge;

/* Specifies a corner of a rectangle */
typedef enum corner {
    TPLFT,
    TPRT,
    BTTMLFT,
    BTTMRT,
    NONECRNR
} Corner;

/* Defines how a rect origin coordinate or dimension (x, y, w, or h) is determined from its parent */
typedef enum dimtype {
    REL, /* x or y relative to parent */
    REVREL, /* Equivalent to REL but starts from bottom or right instead of top or left */
    ABS, /* relative to Window only */
    SCALE, /* x, y, w, or h as proportion of parent (e.g. w = 0.5 => width is half of parent) */
    COMPLEMENT, /* fill the space in parent left by last sibling */
    STACK, /* start where last sibling ended */
    PAD /* applies to w or h; set right bound equal to x bound */
} DimType;

typedef enum rect_mem {
    X,
    Y,
    W,
    H
} RectMem;

/* /\* floatval is reserved for the SCALE dimtype *\/ */
/* typedef union dimval { */
/*     int intval; */
/*     float floatval; */
/* } DimVal; */

/* Used to describe X, Y, W, and H of a Layout */
typedef struct dimension {
    DimType type;
    float value;
} Dimension;


typedef enum layout_type {
    PRGRM_INTERNAL, /* Used for program operation. Not part of editable layout hierarchy */
    TEMPLATE, /* Used as a template for an iterator. Not part of editable layout hierarchy */
    ITERATION, /* A child of an iterator */
    NORMAL
} LayoutType;


typedef struct layout Layout;
typedef struct layout_iterator LayoutIterator;
typedef struct layout {
    SDL_Rect rect;
    Dimension x;
    Dimension y;
    Dimension w;
    Dimension h;
    char name[MAX_LT_NAMELEN];
    Layout *parent;
    Layout *cached_parent;
    Layout **children;
    int64_t num_children;
    int64_t children_arr_len;
    int64_t index;
    Layout *label_lt;
    /* SDL_Rect label_rect; */
    Text *namelabel;
    bool selected;
    LayoutType type;
    LayoutIterator *iterator; /* If the layout type == TEMPLATE, this is not null */
    bool hidden;
    int scroll_offset_v;
    int scroll_offset_h;
    int scroll_momentum_v;
    int scroll_momentum_h;
    // Layout *complement;
    // bool display;
    // bool internal;
} Layout;


/* If VERTICAL, iterations will appear below previous iterations. if HORIZONTAL, to the right. */
typedef enum iterator_type {
    VERTICAL,
    HORIZONTAL
} IteratorType;

/* A member of a TEMPLATE layout that describes how iterations are constructed from that template */
typedef struct layout_iterator {
    Layout *template;
    IteratorType type;
    int16_t num_iterations;
    Layout *iterations[MAX_LT_ITERATIONS];
    bool scrollable;
    float scroll_offset;
    float scroll_momentum;
    int scroll_stop_count;
    int total_height_pixels;
    int total_width_pixels;    
} LayoutIterator;


/* void center_layout_x(Layout *lt); */
/* void center_layout_y(Layout *lt); */

/* Create an empty layout */
Layout *layout_create();

/* Reset a layout's rect and rects of all child layouts */
void layout_reset(Layout *lt);

/* Does not check that layout intersects with window in order to reset children */
void layout_force_reset(Layout *lt);

/* Resize main layout in response to changed window size */
void layout_reset_from_window(Layout *lt, Window *win);

/* Special purpose function to create a main layout */
Layout *layout_create_from_window(Window *win);

/* Permanent deletion. Includes all children. */
void layout_destroy(Layout *lt);

/* Delete layout without adjust parent num children */
void layout_destroy_no_offset(Layout *lt);

/* Add a new child with default name to layout */
Layout *layout_add_child(Layout *parent);

void layout_set_name(Layout *lt, const char *new_name);

/* Add a child with a complementary dimension. comp_rm should be one of W or H */
Layout *layout_add_complementary_child(Layout *parent, RectMem comp_rm);
void layout_translate(Layout *lt, int translate_x, int translate_y, bool block_snap);
void layout_resize(Layout *lt, int resize_w, int resize_h, bool block_snap);
void layout_set_default_dims(Layout *lt);
void layout_reparent(Layout *child, Layout *parent);
// Layout *read_layout(FILE *f, long endrange);
Layout *layout_copy(Layout *to_copy, Layout *parent);

void layout_center_agnostic(Layout *lt, bool horizontal, bool vertical);
void layout_center_scale(Layout *lt, bool horizontal, bool vertical);
void layout_center_relative(Layout *lt, bool horizontal, bool vertical);

Layout *layout_get_child_by_name(Layout *lt, const char *name);
Layout *layout_get_child_by_name_recursive(Layout *lt, const char *name);
void layout_set_type_recursive(Layout *lt, LayoutType type);
void layout_pad(Layout *lt, int h_pad, int v_pad);
Layout *layout_deepest_at_point(Layout *search, SDL_Point *point);
void layout_size_to_fit_children(Layout *lt, bool fixed_origin, int padding);
void layout_size_to_fit_children_h(Layout *lt, bool fixed_origin, int padding);
void layout_size_to_fit_children_v(Layout *lt, bool fixed_origin, int padding);
const char *layout_get_dimtype_str(DimType dt);
const char *layout_get_itertype_str(IteratorType iter_type);

Layout *layout_iterate_siblings(Layout *from, int direction);
Layout *layout_get_first_child(Layout *parent);
Layout *layout_get_parent(Layout *child);
void layout_toggle_dimension(Layout *lt, Dimension *dim, RectMem rm, SDL_Rect *rect, SDL_Rect *parent_rect);
void layout_get_dimval_str(Dimension *dim, char *dst, int maxlen);


void layout_set_edge(Layout *lt, Edge edge, int set_to, bool block_snap);
void layout_set_corner(Layout *lt, Corner crnr, int x, int y, bool block_snap);
void layout_set_position_pixels(Layout *lt, int x, int y, bool block_snap) ;
void layout_move_position(Layout *lt, int move_by_x, int move_by_y, bool block_snap);

LayoutIterator *layout_create_iter_from_template(Layout *template, IteratorType type, int num_iterations, bool scrollable);

/* DEPRECATED. This function uses layout iterators, which are for all intents and purposes deprecated and replaced with stacked layouts */
Layout *layout_handle_scroll(Layout *main_lt, SDL_Point *mousep, float scroll_x, float scroll_y, bool dynamic);

/* Replaces "layout_handle_scroll */
void layout_scroll(Layout *layout, float scroll_x, float scroll_y, bool dynamic);

void layout_halt_scroll(Layout *lt);

int layout_scroll_step(Layout *lt);

Layout *layout_add_iter(Layout *lt, IteratorType type, bool scrollable);
void layout_remove_iter(Layout *lt);
void layout_remove_iter_at(LayoutIterator *iter, int16_t at);

void layout_remove_child(Layout *child);
void layout_insert_child_at(Layout *child, Layout *parent, int16_t index);
void layout_swap_children(Layout *child1, Layout *child2);
void layout_set_wh_from_rect(Layout *lt);
void layout_set_values_from_rect(Layout *lt);

void layout_fprint(FILE *f, Layout *lt);

void layout_draw(Window *win, Layout *lt);


void layout_write(FILE *f, Layout *lt, int indent);
void layout_write_limit_depth(FILE *f, Layout *lt, int indent, int maxdepth);
void layout_write_debug(FILE *f, Layout *lt, int indent, int maxdepth);

#ifdef LT_DEV_MODE
void layout_select(Layout *lt);
void layout_deselect(Layout *lt);
#endif

int layout_test_indices_recursive(Layout *lt);

#endif
