#ifndef JDAW_GUI_SLIDER_H
#define JDAW_GUI_SLIDER_H


#include "layout.h"

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
} FSlider;

FSlider *fslider_create(float *value, Layout *layout, SliderOrientation orientation, SliderType type);
void fslider_reset(FSlider *fs);
void fslider_draw(FSlider *fs);


#endif
