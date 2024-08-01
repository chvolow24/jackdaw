#ifndef JDAW_GUI_COMPONENTS
#define JDAW_GUI_COMPONENTS

#include "layout.h"
#include "textbox.h"
#include "value.h"

#define SLIDER_LABEL_STRBUFLEN 8
#define BUTTON_CORNER_RADIUS 4
#define RADIO_BUTTON_MAX_ITEMS 64
#define RADIO_BUTTON_ITEM_H 24
#define RADIO_BUTTON_LEFT_W 24
#define RADIO_BUTTON_RAD_PAD (3 * main_win->dpi_scale_factor);

typedef int (*ComponentFn)(void *self, void *target);

typedef struct button {
    Textbox *tb;
    ComponentFn action;
    /* void *(*action)(void *arg); */
} Button;

typedef struct toggle {
    bool *value;
    Layout *layout;
} Toggle;

typedef struct text_entry TextEntry;
typedef struct text_entry {
    Textbox *tb;
    Textbox *label;
    /* void (*validation)(TextEntry *self, void *xarg); */
    /* void (*completion)(TextEntry *self, void *xarg); */
    ComponentFn action;
} TextEntry;


typedef void (SliderStrFn)(char *dst, size_t dstsize, void *value, ValType type);
 
enum slider_orientation {
    SLIDER_HORIZONTAL,
    SLIDER_VERTICAL
};

enum slider_style {
    SLIDER_FILL,
    SLIDER_TICK
};

/* typedef struct f_slider { */
/*     Layout *layout; */
/*     float *value; */
/*     float max, min; */
/*     SliderOrientation orientation; */
/*     SliderType type; */
/*     DimVal *val_dim; */
/*     SDL_Rect *bar_rect; */
/*     bool editing; */
/*     Textbox *label; */
/*     char label_str[SLIDER_LABEL_STRBUFLEN]; */
/*     SliderStrFn *create_label; */
/* } FSlider; */


typedef struct radio_button {
    Layout *layout;
    Textbox *items[RADIO_BUTTON_MAX_ITEMS];
    void *target;
    ComponentFn action;
    /* void (*external_action)(int selected_i, void *target); */
    uint8_t num_items;
    uint8_t selected_item;
    SDL_Color *text_color; 
} RadioButton;


/* Slider */
typedef struct slider {
    Layout *layout;
    ValType val_type;
    void *value;
    Value min, max;
    enum slider_orientation orientation;
    enum slider_style style;
    Layout *bar_layout;
    SDL_Rect *bar_rect;
    DimVal *val_dim;
    bool editing;
    Textbox *label;
    char label_str[SLIDER_LABEL_STRBUFLEN];
    SliderStrFn *create_label;
    ComponentFn action;
    void *target;
} Slider;
Slider *slider_create(
    Layout *layout,
    void *value,
    ValType val_type,
    enum slider_orientation orientation,
    enum slider_style style,
    SliderStrFn *create_label_fn,
    ComponentFn action,
    void *target);
/*     Layout *layout, */
/*     SliderOrientation orientation, */
/*     SliderType type, */
/*     SliderStrFn *fn); */
/* void fslider_set_range(FSlider *fs, float min, float max); */
/* void fslider_reset(FSlider *fs); */
/* void fslider_draw(FSlider *fs); */
/* float fslider_val_from_coord(FSlider *fs, int coord_pix); */
/* void fslider_destroy(FSlider *fs); */
void slider_set_range(Slider *s, Value min, Value max);
void slider_reset(Slider *s);
void slider_draw(Slider *s);
Value slider_val_from_coord(Slider *s, int coord_pix);
void slider_destroy(Slider *s);

/* Button */
Button *button_create(Layout *lt, char *text, ComponentFn action, SDL_Color *text_color, SDL_Color *background_color);
void button_draw(Button *button);
void button_destroy(Button *button);

/* Radio button */
RadioButton *radio_button_create(
    Layout *lt,
    int text_size,
    SDL_Color *text_color,
    void *target,
    ComponentFn action,
    /* void (*external_action)(int selected_i, void *target), */
    const char **item_names,
    uint8_t num_items
    );

void radio_button_draw(RadioButton *rb);
bool radio_click(RadioButton *rb, Window *Win);
void radio_destroy(RadioButton *rb);

/* Toggle */

Toggle *toggle_create(Layout *lt, bool *value);
void toggle_destroy(Toggle *toggle);
/* Returns the new value of the toggle */
bool toggle_toggle(Toggle *toggle);
void toggle_draw(Toggle *toggle);

bool slider_mouse_motion(Slider *slider, Window *win);
bool toggle_mouse_click(Toggle *toggle, Window *win);

#endif
