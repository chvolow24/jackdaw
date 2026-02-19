/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

/*****************************************************************************************************************
    endpoint_callbacks.h

    * Prototypes of all endpoint callback functions
 *****************************************************************************************************************/


#include "project.h"
#include "endpoint.h"

void play_speed_gui_cb(Endpoint *ep);
void track_slider_cb(Endpoint *ep);

/* void filter_cutoff_gui_cb(Endpoint *ep); */

/* void filter_bandwidth_gui_cb(Endpoint *ep); */

/* void filter_irlen_gui_cb(Endpoint *ep); */


/* void filter_type_gui_cb(Endpoint *ep); */

void delay_line_len_dsp_cb(Endpoint *ep);
void delay_line_len_gui_cb(Endpoint *ep);
void delay_line_amp_dsp_cb(Endpoint *ep);
void delay_line_amp_gui_cb(Endpoint *ep);
void delay_line_stereo_offset_gui_cb(Endpoint *ep);

void click_track_ebb_gui_cb(Endpoint *ep);

void click_segment_bound_proj_cb(Endpoint *ep);
void click_segment_bound_gui_cb(Endpoint *ep);

/* void saturation_gain_gui_cb(Endpoint *ep); */
/* void saturation_type_gui_cb(Endpoint *ep); */

void page_el_gui_cb(Endpoint *ep);
