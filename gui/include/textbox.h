#ifndef JDAW_GUI_TEXTBOX_H
#define JDAW_GUI_TEXTBOX_H

#include "layout.h"
#include "text.h"

#define MAX_MENULIST_SCTN_LEN 64
#define MAX_MENULIST_SECTIONS 16
#define MAX_MENULIST_COLUMNS 8

/*
  Textbox layout must have "text" target
  and "box" target
*/
typedef struct textbox {
    Text *text;
    Layout *layout;
    SDL_Color *bckgrnd_clr;
    SDL_Color *border_clr;
    int border_thickness;
    int corner_radius;
} Textbox;

typedef struct menulist_item {
    char *label;
    char *annotation;
    bool available;
    void (*onclick)(Textbox *tb, void *target);
    void *target;
} MenulistItem;

typedef struct menulist_section {
    char *label;
    MenulistItem *items[MAX_MENULIST_SCTN_LEN];
    uint8_t num_items;
} MenulistSection;
    
typedef struct menulist_column {
    char *label;
    MenulistSection *sections[MAX_MENULIST_SECTIONS];
    uint8_t num_sections;
} MenulistColumn;

typedef struct Menulist {
    MenulistColumn  *columns[MAX_MENULIST_COLUMNS];
    uint8_t num_columns;
} Menulist;

Textbox *textbox_create();
void textbox_destroy(Textbox *);
Textbox *textbox_create_from_str(
    char *set_str,
    Layout *lt,
    TTF_Font *font,
    SDL_Renderer *rend
    );

void textbox_pad(Textbox *tb, int pad);
void textbox_draw(Textbox *tb);




#endif
