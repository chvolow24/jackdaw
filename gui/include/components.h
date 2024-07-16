#ifndef JDAW_GUI_COMPONENTS
#define JDAW_GUI_COMPONENTS

#include "layout.h"
#include "textbox.h"

#define SLIDER_LABEL_STRBUFLEN 8
#define BUTTON_CORNER_RADIUS 4


typedef struct button {
    Textbox *tb;
    void *(*action)(void *arg);
} Button;


typedef struct text_entry TextEntry;
typedef struct text_entry {
    Textbox *tb;
    Textbox *label;
    /* void (*validation)(TextEntry *self, void *xarg); */
    void (*completion)(TextEntry *self, void *xarg);
} TextEntry;


typedef void (SliderStrFn)(char *dst, size_t dstsize, float value);
 
typedef enum slider_orientation {
    SLIDER_HORIZONTAL,
    SLIDER_VERTICAL
} SliderOrientation;

typedef enum slider_type {
    SLIDER_FILL,
    SLIDER_TICK
} SliderType;

typedef struct f_slider {
    Layout *layout;
    float *value;
    float max, min;
    SliderOrientation orientation;
    SliderType type;
    DimVal *val_dim;
    SDL_Rect *bar_rect;
    bool editing;
    Textbox *label;
    char label_str[SLIDER_LABEL_STRBUFLEN];
    SliderStrFn *create_label;
} FSlider;


typedef struct radio_button {
    Layout *lt;
    TextLines *items;
    uint8_t num_items;
    uint8_t current_item;
} RadioButton;


/* FSlider */
FSlider *fslider_create(
    float *value,
    Layout *layout,
    SliderOrientation orientation,
    SliderType type,
    SliderStrFn *fn);
void fslider_set_range(FSlider *fs, float min, float max);
void fslider_reset(FSlider *fs);
void fslider_draw(FSlider *fs);
float fslider_val_from_coord(FSlider *fs, int coord_pix);
void fslider_destroy(FSlider *fs);


/* Button */
Button *button_create(Layout *lt, char *text, void *(*action)(void *arg), SDL_Color *text_color, SDL_Color *background_color);
void button_draw(Button *button);
void button_destroy(Button *button);

/* Radio button */
RadioButton *radio_button_create(
    Layout *lt,
    int (*item_to_tline_filter)(void *item, void *x_arg));


#endif