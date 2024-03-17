#include "menu.h"

Menu *menu_create(Layout *layout)
{
    Menu *menu = malloc(sizeof(Menu));
    menu->num_columns = 0;
    menu->layout = layout;
    return menu;
}

MenuColumn *menu_column_add(Menu *menu, char *label)
{
    MenuColumn *col = malloc(sizeof(MenuColumn));
    col->num_sections = 0;
    col->label = label;
    col->menu = menu;
    menu->columns[menu->num_columns] = col;
    menu->num_columns++;
    return col;
}

MenuSection *menu_section_add(MenuColumn *col, char *label)
{
    MenuSection *sctn = malloc(sizeof(MenuSection));
    sctn->num_items = 0;
    sctn->label = label;
    sctn->column = col;
    col->sections[col->num_sections] = sctn;
    col->num_sections++;
    return sctn;
}

MenuItem *menu_item_add(
    MenuSection *sctn,
    char *label,
    char *annotation,
    void (*onclick)(Textbox *tb, void *target),
    void *target
)
{
    MenuItem *item = malloc(sizeof(MenuItem));
    item->label = label;
    item->annotation = NULL;
    item->available = true;
    item->onclick = NULL;
    item->target = NULL;
    item->section = sctn;
    sctn->items[sctn->num_items] = item;
    sctn->num_items++;
    return item;
}


void menu_destroy(Menu *menu)
{
    for (int c=0; c<menu->num_columns; c++) {
	MenuColumn *column = menu->columns[c];
	for (int s=0; s < column->num_sections; s++) {
	    MenuSection *sctn = column->sections[s];
	    for (int i=0; i<sctn->num_items; i++) {
		MenuItem *item = sctn->items[i];
		free(item);
	    }
	    free(sctn);
	}
	free(column);
    }
    free(menu);
}
