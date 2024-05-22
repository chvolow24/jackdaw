#ifndef JDAW_GUI_SLIDER_H
#define JDAW_GUI_SLIDER_H


#include "layout.h"
#include "textbox.h"

#define SLIDER_LABEL_STRBUFLEN 8


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

#endif
