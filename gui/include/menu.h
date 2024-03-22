#ifndef JDAW_GUI_MENU
#define JDAW_GUI_MENU

#include "textbox.h"

typedef struct menu_section MenuSection;
typedef struct menu_column MenuColumn;
typedef struct menu Menu;

typedef struct menu_item {
    Textbox *tb;
    Textbox *annot_tb;
    MenuSection *section;
    char *label;
    char *annotation;
    bool available;
    void (*onclick)(Textbox *tb, void *target);
    void *target;
    bool hovering;
    Layout *layout;
} MenuItem;

typedef struct menu_section {
    MenuColumn *column;
    char *label;
    MenuItem *items[MAX_MENU_SCTN_LEN];
    uint8_t num_items;
    Layout *layout;
} MenuSection;
    
typedef struct menu_column {
    Menu *menu;
    char *label;
    MenuSection *sections[MAX_MENU_SECTIONS];
    uint8_t num_sections;
    Layout *layout;
} MenuColumn;

typedef struct menu {
    MenuColumn  *columns[MAX_MENU_COLUMNS];
    uint8_t num_columns;
    Layout *layout;
    Window *window;
} Menu;


Menu *menu_create(Layout *layout, Window *window);
MenuColumn *menu_column_add(Menu *menu, char *label);
MenuSection *menu_section_add(MenuColumn *column, char *label);
MenuItem *menu_item_add(
    MenuSection *sctn,
    char *label,
    char *annotation,
    void (*onclick)(Textbox *tb, void *target),
    void *target);

/* void menu_item_destroy(MenuItem *item); */
/* void menu_section_destroy(MenuSection *section); */
void menu_destroy(Menu *menu);

void menu_draw(Menu *menu);
void triage_mouse_menu(Menu *menu, SDL_Point *mousep, bool click);

#endif


