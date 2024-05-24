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
    const char *label;
    const char *annotation;
    bool available;
    void (*onclick)(void *);
    void *target;
    bool free_target_on_destroy;
    bool selected;
    Layout *layout;
} MenuItem;

typedef struct menu_section {
    MenuColumn *column;
    const char *label;
    MenuItem *items[MAX_MENU_SCTN_LEN];
    uint8_t num_items;
    uint8_t sel_item;
    Layout *layout;
} MenuSection;
 
typedef struct menu_column {
    Menu *menu;
    const char *label;
    MenuSection *sections[MAX_MENU_SECTIONS];
    uint8_t num_sections;
    uint8_t sel_sctn;
    Layout *layout;
} MenuColumn;

typedef struct menu {
    const char *title;
    const char *description;
    TextArea *header;
    MenuColumn  *columns[MAX_MENU_COLUMNS];
    uint8_t num_columns;
    MenuItem *selected;
    uint8_t sel_col;
    Layout *layout;
    Font *font;
    /* TTF_Font *font; */
    Window *window;
} Menu;


Menu *menu_create(Layout *layout, Window *window);
Menu *menu_create_at_point(int x_pix, int y_pix);
MenuColumn *menu_column_add(Menu *menu, const char *label);
MenuSection *menu_section_add(MenuColumn *column, const char *label);
MenuItem *menu_item_add(
    MenuSection *sctn,
    const char *label,
    const char *annotation,
    void (*onclick)(void *target),
    void *target);

/* void menu_item_destroy(MenuItem *item); */
/* void menu_section_destroy(MenuSection *section); */
void menu_destroy(Menu *menu);
void menu_reset_layout(Menu *menu);
void menu_draw(Menu *menu);
void triage_mouse_menu(Menu *menu, SDL_Point *mousep, bool click);

MenuItem *menu_item_at_index(Menu *menu, int index);
void menu_translate(Menu *menu, int translate_x, int translate_y);
void menu_add_header(Menu *menu, const char *title, const char *description);
#endif


