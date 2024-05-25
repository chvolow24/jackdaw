#ifndef JDAW_GUI_MODAL
#define JDAW_GUI_MODAL

#include "menu.h"

#define MAX_MODAL_ELEMENTS 255

typedef struct text_entry {
    Textbox *tb;
    int (*validation)(char *to_test, const char *msg);
    int (*completion)(void *arg);
} TextEntry;

typedef struct button {
    Textbox *tb;
    void *(*completion)(void *arg);	
} Button;

enum mod_s_type {
    MODAL_EL_MENU,
    MODAL_EL_TEXTENTRY,
    MODAL_EL_TEXTAREA,
    MODAL_EL_TEXT,
    MODAL_EL_DIRNAV
};

/* union mod_s { */
/*     Menu *menu; */
/*     Textbox *tb; /\* editable *\/ */
/*     TextArea *textarea; /\* not editable *\/ */
/* }; */

typedef struct ModalEl {
    enum mod_s_type type;
    void *obj;
    Layout *layout;
} ModalEl;

typedef struct modal {
    Layout *layout; /* Children in same order as modal elements */
    uint8_t num_els;
    ModalEl *els[MAX_MODAL_ELEMENTS];
} Modal;

Modal *modal_create(Layout *lt);
void modal_destroy(Modal *modal);
ModalEl *modal_add_header(Modal *modal, const char *text, SDL_Color *color, int level);
ModalEl *modal_add_p(Modal *modal, const char *text, SDL_Color *color);
ModalEl *modal_add_dirnav(Modal *modal, const char *dirpath, bool show_dirs, bool show_files);
void modal_reset(Modal *modal);
void modal_draw(Modal *modal);

#endif
