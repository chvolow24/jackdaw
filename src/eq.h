/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

/*****************************************************************************************************************
    eq.h

    * Parametric EQ
    * Implemented as IIR group and accompanying freq plot
 *****************************************************************************************************************/

#ifndef JDAW_EQ_H
#define JDAW_EQ_H

#include "endpoint.h"
#include "iir.h"


#define EQ_DEFAULT_NUM_FILTERS 6
#define EQ_DEFAULT_CHANNELS 2
#define EQ_MAX_AMPLITUDE 20.0
#define EQ_CTRL_RAD (6 * main_win->dpi_scale_factor)
#define EQ_CTRL_LABEL_BUFLEN 32

typedef struct eq EQ;
typedef struct eq_filter_ctrl {
    EQ *eq;
    bool filter_active;
    int index;
    double freq_amp_raw[2];
    /* double freq_raw; */
    /* double amp_raw; */
    double bandwidth_scalar; /* Bandwidth = bandwidth_scalar * freq_raw */
    Endpoint freq_ep;
    Endpoint amp_ep;
    Endpoint freq_amp_ep;
    Endpoint bandwidth_scalar_ep;
    int x;
    int y;
    char label_str[EQ_CTRL_LABEL_BUFLEN];
    Label *label;
} EQFilterCtrl;

typedef struct eq {
    bool active;
    IIRGroup group;
    struct freq_plot *fp;
    EQFilterCtrl ctrls[EQ_DEFAULT_NUM_FILTERS];
    int current_ctrl;
} EQ;

/* void eq_set_peak(EQ *eq, int filter_index, double freq_raw, double amp_raw, double bandwidth); */
void eq_init(EQ *eq);
void eq_deinit(EQ *eq);
void eq_create_freq_plot(EQ *eq, Layout *container);
void eq_set_filter_from_ctrl(EQ *eq, int index);
void eq_destroy_freq_plot(EQ *eq);
/* void eq_set_filter_from_mouse(EQ *eq, int filter_index, SDL_Point mousep); */
bool eq_mouse_click(EQ *eq, SDL_Point mousep);
void eq_mouse_motion(EQFilterCtrl *ctrl, Window *win);
void eq_draw(EQ *eq);
double eq_sample(EQ *eq, double in, int channel);
void eq_advance(EQ *eq, int channel);

#endif
