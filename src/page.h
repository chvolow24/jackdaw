#ifndef JDAW_PAGE_H
#define JDAW_PAGE_H

#include "layout.h"
#include "textbox.h"

#define MAX_ELEMENTS 255
#define MAX_TABS 16

typedef enum element_type {
    EL_TEXTAREA,
    EL_TEXTBOX,
    EL_TEXTENTRY,
    EL_SLIDER,
    EL_RADIO,
    EL_TOGGLE,
    EL_PLOT,
    EL_FREQ_PLOT,
    EL_BUTTON
} ElType;

typedef struct page_element {
    ElType type;
    void *component;
    Layout *layout;
} PageEl;

typedef struct page {
    const char *title;
    PageEl *elements[MAX_ELEMENTS];
    uint8_t num_elements;
    Layout *layout;
    SDL_Color *background_color;
} Page;

typedef struct tab_view {
    const char *title;
    Page *tabs[MAX_TABS];
    Textbox *labels[MAX_TABS];
    uint8_t num_tabs;
    uint8_t current_tab;
    Layout *layout;
} TabView;



struct slider_params {
    Layout *layout;
    void *value;
    
    
};

struct something_params {
    float sdfdfd;
};




#endif
