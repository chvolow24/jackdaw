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
    SCALE /* x, y, w, or h as proportion of parent (e.g. w = 0.5 => width is half of parent) */
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

typedef struct layout Layout;
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
    bool display;
    bool internal;
} Layout;


// int get_rect_val_from_dim(Dimension dim, int parent_rect_coord);

void center_layout_x(Layout *lt);
void center_layout_y(Layout *lt);

Layout *create_layout();
void reset_layout(Layout *lt);
void reset_layout_from_window(Layout *lt, Window *win);
Layout *create_layout_from_window(Window *win);
void delete_layout(Layout *lt);
Layout *add_child(Layout *parent);
void translate_layout(Layout *lt, int translate_x, int translate_y, bool block_snap);
void resize_layout(Layout *lt, int resize_w, int resize_h, bool block_snap);
void set_default_dims(Layout *lt);
void reparent(Layout *child, Layout *parent);
// Layout *read_layout(FILE *f, long endrange);

Layout *get_child(Layout *lt, const char *name);
Layout *get_child_recursive(Layout *lt, const char *name);
const char *get_dimtype_str(DimType dt);

void toggle_dimension(Dimension *dim, RectMem rm, SDL_Rect *rect, SDL_Rect *parent_rect);
void get_val_str(Dimension *dim, char *dst, int maxlen);


void set_edge(Layout *lt, Edge edge, int set_to, bool block_snap);
void set_corner(Layout *lt, Corner crnr, int x, int y, bool block_snap);
void set_position(Layout *lt, int x, int y, bool block_snap) ;
void move_position(Layout *lt, int move_by_x, int move_by_y, bool block_snap);

#endif