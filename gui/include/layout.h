#ifndef JDAW_GUI_LAYOUT_H
#define JDAW_GUI_LAYOUT_H

#include <stdio.h>
#include <stdbool.h>
#include "SDL.h"
#include "text.h"
#include "window.h"

#define MAX_CHILDREN 255
#define MAX_LT_NAMELEN 64

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
    ABS, /* relative to Window only */
    SCALE, /* x, y, w, or h as proportion of parent (e.g. w = 0.5 => width is half of parent) */
    COMPLEMENT
} DimType;

typedef enum rect_mem {
    X,
    Y,
    W,
    H
} RectMem;

typedef union dimval {
    int intval;
    float floatval;
} DimVal;

typedef struct dimension {
    DimType type;
    DimVal value;
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
    Layout *children[MAX_CHILDREN];
    uint8_t num_children;
    uint8_t index;
    SDL_Rect label_rect;
    Text *namelabel;
    bool selected;
    LayoutType type;
    LayoutIterator *iterator; /* If the layout type == TEMPLATE, this is not null */
    // Layout *complement;
    // bool display;
    // bool internal;
} Layout;


typedef enum iterator_type {
    VERTICAL,
    HORIZONTAL
} IteratorType;

typedef struct layout_iterator {
    Layout *template;
    IteratorType type;
    uint8_t num_iterations;
    Layout *iterations[MAX_CHILDREN];
    bool scrollable;
    float scroll_offset;
    float scroll_momentum;
    
    
} LayoutIterator;

// int get_rect_val_from_dim(Dimension dim, int parent_rect_coord);

void center_layout_x(Layout *lt);
void center_layout_y(Layout *lt);

Layout *create_layout();
void reset_layout(Layout *lt);
void reset_layout_from_window(Layout *lt, Window *win);
Layout *create_layout_from_window(Window *win);
void delete_layout(Layout *lt);
Layout *add_child(Layout *parent);
Layout *add_complementary_child(Layout *parent, RectMem comp_rm);
void translate_layout(Layout *lt, int translate_x, int translate_y, bool block_snap);
void resize_layout(Layout *lt, int resize_w, int resize_h, bool block_snap);
void set_default_dims(Layout *lt);
void reparent(Layout *child, Layout *parent);
// Layout *read_layout(FILE *f, long endrange);
Layout *copy_layout(Layout *to_copy, Layout *parent);

Layout *get_child(Layout *lt, const char *name);
Layout *get_child_recursive(Layout *lt, const char *name);
void set_layout_type_recursive(Layout *lt, LayoutType type);
Layout *deepest_layout_at_point(Layout *search, SDL_Point *point);
const char *get_dimtype_str(DimType dt);
const char *get_itertype_str(IteratorType iter_type);

Layout *iterate_siblings(Layout *from, int direction);
Layout *get_first_child(Layout *parent);
Layout *get_parent(Layout *child);
void toggle_dimension(Layout *lt, Dimension *dim, RectMem rm, SDL_Rect *rect, SDL_Rect *parent_rect);
void get_val_str(Dimension *dim, char *dst, int maxlen);


void set_edge(Layout *lt, Edge edge, int set_to, bool block_snap);
void set_corner(Layout *lt, Corner crnr, int x, int y, bool block_snap);
void set_position_pixels(Layout *lt, int x, int y, bool block_snap) ;
void move_position(Layout *lt, int move_by_x, int move_by_y, bool block_snap);

LayoutIterator *create_iterator_from_template(Layout *template, IteratorType type, int num_iterations, bool scrollable);

Layout *handle_scroll(Layout *main_lt, SDL_Point *mousep, float scroll_x, float scroll_y);
int scroll_step(Layout *lt);

void add_iteration_to_layout(Layout *lt, IteratorType type, bool scrollable);
void remove_iteration_from_layout(Layout *lt);

#endif
