#ifndef JDAW_PAGE_EL_TYPE_H
#define JDAW_PAGE_EL_TYPE_H

typedef enum page_el_type: int {
    EL_TEXTAREA,
    EL_TEXTBOX,
    EL_TEXTENTRY,
    EL_SLIDER,
    EL_RADIO,
    EL_TOGGLE,
    EL_WAVEFORM,
    EL_FREQ_PLOT,
    EL_BUTTON,
    EL_CANVAS,
    EL_EQ_PLOT,
    EL_SYMBOL_BUTTON,
    EL_SYMBOL_RADIO,
    EL_DROPDOWN,
    EL_STATUS_LIGHT,
    EL_PIANO,
    EL_DIVIDER,
    EL_VU_METER,
    EL_PAGE_LIST,
    /* EL_TOGGLE_EP */
} PageElType;

#endif
