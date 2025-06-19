#ifndef JDAW_GUI_COMPONENTS
#define JDAW_GUI_COMPONENTS

/* #include "endpoint.h" */
#include "label.h"
#include "layout.h"
#include "symbol.h"
#include "textbox.h"
#include "value.h"

#define SLIDER_LABEL_STRBUFLEN 20
#define RADIO_BUTTON_MAX_ITEMS 64
#define RADIO_BUTTON_ITEM_H 24
#define RADIO_BUTTON_LEFT_W 24
#define RADIO_BUTTON_RAD_PAD (3 * main_win->dpi_scale_factor);

#define SLIDER_LABEL_H_PAD 4
#define SLIDER_LABEL_V_PAD 2

/*****************************************/
/************ Definitions ****************/
/*****************************************/

typedef int (*ComponentFn)(void *self, void *target);
typedef struct endpoint Endpoint;

typedef struct button {
    Textbox *tb;
    ComponentFn action;
    void *target;
    Animation *animation;
} Button;

typedef struct symbol_button {
    Symbol *symbol;
    ComponentFn action;
    void *target;
    Layout *layout;
    SDL_Color *background_color;
    Value stashed_val;
} SymbolButton;

typedef struct toggle {
    bool *value;
    ComponentFn action;
    void *target;
    Layout *layout;

    Endpoint *endpoint; /* Optional */
} Toggle;

typedef struct text_entry TextEntry;
typedef struct text_entry {
    Textbox *tb;
    /* Textbox *label; */
    ComponentFn action;
    void *target;
} TextEntry;

/* typedef void (SliderStrFn)(char *dst, size_t dstsize, void *value, ValType type); */
 
enum slider_orientation {
    SLIDER_HORIZONTAL,
    SLIDER_VERTICAL
};

enum slider_style {
    SLIDER_FILL,
    SLIDER_TICK
};

enum drag_comp_type {
    DRAG_SLIDER,
    DRAG_CLICK_SEG_BOUND,
    DRAG_EQ_FILTER_NODE,
    DRAG_TABVIEW_TAB
};

typedef struct draggable {
    enum drag_comp_type type;
    void *component;
} Draggable;

typedef struct radio_button {
    Layout *layout;
    Textbox *items[RADIO_BUTTON_MAX_ITEMS];
    Endpoint *ep;
    /* void *target; */
    /* ComponentFn action; */
    /* void (*external_action)(int selected_i, void *target); */
    uint8_t num_items;
    uint8_t selected_item;
    SDL_Color *text_color;
    char *dynamic_text;
} RadioButton;

typedef struct symbol_radio {
    Layout *layout;
    Symbol *symbols[RADIO_BUTTON_MAX_ITEMS];
    Endpoint *ep;
    uint8_t num_items;
    uint8_t selected_item;
    SDL_Color *sel_color;
    SDL_Color *unsel_color;
} SymbolRadio;

typedef struct waveform {
    Layout *layout;
    void **channels;
    ValType type;
    uint8_t num_channels;
    uint32_t len;
    SDL_Color *background_color;
    SDL_Color *plot_color;
} Waveform;

typedef struct slider {
    Layout *layout;
    Endpoint *ep;
    /* ValType val_type; */
    /* void *value; */
    Value min, max;
    enum slider_orientation orientation;
    enum slider_style style;
    Layout *bar_layout;
    SDL_Rect *bar_rect;
    float *val_dim;
    bool editing;
    /* Textbox *label; */
    /* char label_str[SLIDER_LABEL_STRBUFLEN]; */
    /* SliderStrFn *create_label; */
    /* ComponentFn action; */
    /* void *target; */
    /* int label_countdown; */
    Label *label;
    Draggable *drag_context;

    bool disallow_unsafe_mode;

} Slider;
typedef struct canvas Canvas;
typedef struct canvas {
    Layout *layout;
    void (*draw_fn)(void *, void *);
    void *draw_arg1, *draw_arg2;
    bool (*on_click)(SDL_Point mousep, Canvas *self, void *xarg1, void *xarg2);
} Canvas;

typedef struct dropdown Dropdown;
typedef struct dropdown {
    Layout *layout;
    Textbox *tb;
    const char *header;
    const char *description;
    char **item_names;
    char **item_annotations;
    void **item_args;
    uint8_t num_items;
    uint8_t selected_item;
    /* bool free_args_on_destroy; */
    int (*selection_fn)(Dropdown *self, void *arg);
} Dropdown;

typedef struct status_light {
    Layout *layout;
    void *value;
    size_t val_size;
} StatusLight;



/*****************************************/
/************ Interfaces ****************/
/*****************************************/


/* Draggable */

void draggable_handle_scroll(Draggable *d, int x, int y);

/* Slider */

Slider *slider_create(
    Layout *layout,
    Endpoint *ep,
    Value min, Value max,
    enum slider_orientation orientation,
    enum slider_style style,
    LabelStrFn set_str_fn,
    /* SliderStrFn *create_label_fn, */
    /* ComponentFn action, */
    /* void *target, */
    Draggable *drag_context);
/*     Layout *layout, */
/*     SliderOrientation orientation, */
/*     SliderType type, */
/*     SliderStrFn *fn); */
/* void fslider_set_range(FSlider *fs, float min, float max); */
/* void fslider_reset(FSlider *fs); */
/* void fslider_draw(FSlider *fs); */
/* float fslider_val_from_coord(FSlider *fs, int coord_pix); */
/* void fslider_destroy(FSlider *fs); */


/* void slider_set_value(Slider *s, Value val); */
/* void slider_set_range(Slider *s, Value min, Value max); */

Value slider_reset(Slider *s);
void slider_draw(Slider *s);
bool slider_mouse_click(Slider *slider, Window *win);
bool slider_mouse_motion(Slider *slider, Window *win);
void slider_destroy(Slider *s);


void slider_std_labelmaker(char *dst, size_t dstsize, void *value, ValType type);
/* LabelStrFn slider_std_labelmaker; */
/* SliderStrFn slider_std_labelmaker; */
/* Value slider_val_from_coord(Slider *s, int coord_pix); */
/* void slider_edit_made(Slider *slider); */
void slider_nudge_right(Slider *slider);
void slider_nudge_left(Slider *slider);



/* Button */
/* Button *button_create(Layout *lt, char *text, ComponentFn action, SDL_Color *text_color, SDL_Color *background_color); */
Button *button_create(
    Layout *layout,
    char *text,
    ComponentFn action,
    void *target,
    Font *font,
    int text_size,
    SDL_Color *text_color,
    SDL_Color *background_color);
void button_draw(Button *button);
void button_destroy(Button *button);
void button_press_color_change(
    Button *button,
    SDL_Color *temp_color,
    SDL_Color *return_color,
    ComponentFn callback,
    void *callback_target);


/* Symbol button */


SymbolButton *symbol_button_create(
    Layout *lt,
    Symbol *symbol,
    ComponentFn action,
    void *target,
    SDL_Color *background_color);


void button_draw(Button *button);
void symbol_button_draw(SymbolButton *sbutton);
void symbol_button_destroy(SymbolButton *sbutton);
bool symbol_button_click(SymbolButton *sbutton, Window *win);

/* Textentry */

TextEntry *textentry_create(
    Layout *lt,
    char *value_handle,
    int buf_len,
    Font *font,
    uint8_t text_size,
    int (*validation)(Text *txt, char input),
    int (*completion)(Text *txt, void *target),
    void *completion_target,
    Window *win);

void textentry_destroy(TextEntry *te);
void textentry_draw(TextEntry *te);
void textentry_reset(TextEntry *te);
void textentry_edit(TextEntry *te);
void textentry_complete_edit(TextEntry *te);

/* Radio button */
RadioButton *radio_button_create(
    Layout *layout,
    int text_size,
    SDL_Color *text_color,
    Endpoint *ep,
    /* void *target, */
    /* ComponentFn action, */
    /* void (*external_action)(int selected_i, void *target), */
    const char **item_names,
    uint8_t num_items
    );
void radio_button_reset_from_endpoint(RadioButton *rb);
void radio_button_draw(RadioButton *rb);
bool radio_click(RadioButton *rb, Window *Win);
void radio_destroy(RadioButton *rb);
void radio_cycle(RadioButton *rb);
void radio_cycle_back(RadioButton *rb);


/* Symbol radio */

SymbolRadio *symbol_radio_create(
    Layout *lt,
    Symbol **symbols,
    uint8_t num_items,
    Endpoint *ep,
    bool align_horizontal,
    int padding,
    SDL_Color *sel_color,
    SDL_Color *unsel_color);

void symbol_radio_reset_from_endpoint(SymbolRadio *sr);
void symbol_radio_draw(SymbolRadio *sr);
void symbol_radio_cycle_back(SymbolRadio *sr);
void symbol_radio_cycle(SymbolRadio *sr);
bool symbol_radio_click(SymbolRadio *sr, Window *Win);
void symbol_radio_destroy(SymbolRadio *sr);

/* Waveform */

Waveform *waveform_create(
    Layout *lt,
    void **channels,
    ValType type,
    uint8_t num_channels,
    uint32_t len,
    SDL_Color *background_color,
    SDL_Color *plot_color);
void waveform_destroy(Waveform *w);
void waveform_draw(Waveform *w);


/* Toggle */

Toggle *toggle_create(Layout *lt, bool *value, ComponentFn action, void *target);
Toggle *toggle_create_from_endpoint(Layout *lt, Endpoint *ep);
void toggle_destroy(Toggle *toggle);
/* Returns the new value of the toggle */
bool toggle_toggle(Toggle *toggle);
void toggle_draw(Toggle *toggle);
bool toggle_click(Toggle *toggle, Window *win);


/* Mouse functions */
bool draggable_mouse_motion(Draggable *draggable, Window *win);
bool toggle_mouse_click(Toggle *toggle, Window *win);
bool button_click(Button *button, Window *win);


/* Canvas */

Canvas *canvas_create(
    Layout *lt,
    void (*draw_fn)(void *arg1, void *arg2),
    void *draw_arg1,
    void *draw_arg2
    );
void canvas_draw(Canvas *canvas);
void canvas_destroy(Canvas *canvas);

/* Dropdown */

Dropdown *dropdown_create(
    Layout *lt,
    const char *header,
    char **item_names,
    char **item_annotations,
    void **item_args,
    uint8_t num_items,
    /* bool free_args_on_destroy, */
    int (*selection_fn)(Dropdown *self, void *arg));
void dropdown_draw(Dropdown *d);
void dropdown_destroy(Dropdown *d);
void dropdown_create_menu(Dropdown *d);
bool dropdown_click(Dropdown *d, Window *win);

/* Status light */

StatusLight *status_light_create(Layout *lt, void *value, size_t val_size);
void status_light_draw(StatusLight *sl);
void status_light_destroy(StatusLight *sl);

#endif
