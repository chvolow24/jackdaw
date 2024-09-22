#ifndef JDAW_GUI_MODAL
#define JDAW_GUI_MODAL

#include "components.h"
#include "menu.h"

#define MAX_MODAL_ELEMENTS 255

enum mod_s_type {
    MODAL_EL_MENU,
    MODAL_EL_TEXTENTRY,
    MODAL_EL_TEXTAREA,
    MODAL_EL_TEXT,
    MODAL_EL_DIRNAV,
    MODAL_EL_BUTTON,
    MODAL_EL_RADIO
};

typedef struct ModalEl {
    enum mod_s_type type;
    void *obj;
    Layout *layout;
} ModalEl;

typedef struct modal {
    Layout *layout; /* Children in same order as modal elements */
    uint8_t num_els;
    ModalEl *els[MAX_MODAL_ELEMENTS];
    uint8_t selectable_indices[MAX_MODAL_ELEMENTS];
    uint8_t num_selectable;
    uint8_t selected_i;
    ComponentFn submit_form;
} Modal;

Modal *modal_create(Layout *lt);
void modal_destroy(Modal *modal);
ModalEl *modal_add_header(Modal *modal, const char *text, SDL_Color *color, int level);
ModalEl *modal_add_p(Modal *modal, const char *text, SDL_Color *color);
ModalEl *modal_add_dirnav(Modal *modal, const char *dirpath, int (*dir_to_tline_filter)(void *dp_v, void *dn_v));
ModalEl *modal_add_textentry(Modal *modal, char *init_val, int (*validation)(Text *txt, char input), int (*completion)(Text *));
/* ModalEl *modal_add_textentry(Modal *modal, char *init_val, int (*validation)(Text *txt, char input), ComponentFn completion); */
ModalEl *modal_add_button(Modal *modal, char *text, ComponentFn action);
    /* Layout *layout, */
    /* int text_size, */
    /* SDL_Color *text_color, */
    /* void *target, */
    /* ComponentFn action, */
    /* /\* void (*external_action)(int selected_i, void *target), *\/ */
    /* const char **item_names, */
    /* uint8_t num_items */
    /* ); */

ModalEl *modal_add_radio(
    Modal *modal,
    SDL_Color *color,
    void *target,
    ComponentFn action,
    const char **item_names,
    uint8_t num_items);
void modal_reset(Modal *modal);
void modal_draw(Modal *modal);

void modal_next(Modal *modal);
void modal_previous(Modal *modal);
void modal_next_escape(Modal *modal);
void modal_previous_escape(Modal *modal);
void modal_select(Modal *modal);
void modal_move_onto(Modal *modal);
void modal_submit_form(Modal *modal);
bool modal_triage_mouse(Modal *modal, SDL_Point *mousep, bool click);
Layout *modal_triage_wheel(Modal *modal, SDL_Point *mousep, int x, int y, bool dynamic);

#endif
