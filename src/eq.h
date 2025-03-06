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

#include "iir.h"

#define DEFAULT_NUM_FILTERS 4
#define DEFAULT_CHANNELS 2
#define EQ_MAX_AMPLITUDE 20.0

typedef struct eq {
    IIRGroup group;
    struct freq_plot *fp;
} EQ;

void eq_set_peak(EQ *eq, int filter_index, double freq_raw, double amp_raw, double bandwidth);


