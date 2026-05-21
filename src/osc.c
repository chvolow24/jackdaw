#include <math.h>
#include "consts.h"
#include "osc.h"
#include "session.h"



void osc_init(
    OscGeneric *osc,
    OscType type,
    double freq_hz,
    double *external_phase, // If NULL, external phase not tracked
    double *external_phase_incr,
    double *external_phase_incr_dst, /* For linear ramps */
    double phase_shift
)
{
    osc->type = type;
    osc->freq_hz = freq_hz;
    osc->phase_incr = freq_hz * TAU / session_get_sample_rate();
    osc->external_phase = external_phase;
    osc->external_phase_incr = external_phase_incr;
    osc->external_phase_incr_dst = external_phase_incr_dst;
    osc->phase_shift = phase_shift;
}

static inline void fill_buf_sine(OscGeneric *osc, float *restrict buf, int len)
{
    double phase = osc->external_phase ? *osc->external_phase + osc->phase_shift : osc->phase;
    double phase_incr = osc->external_phase ? *osc->external_phase_incr : osc->phase_incr;
    for (int i=0; i<len; i++) {
	buf[i] = sin(phase);
	phase += phase_incr;
    }
    while (phase > TAU) phase -= TAU;
    osc->phase = phase;
}

static inline void fill_buf_square(OscGeneric *osc, float *restrict buf, int len)
{
    double phase = osc->external_phase ? *osc->external_phase + osc->phase_shift : osc->phase;
    double phase_incr = osc->external_phase ? *osc->external_phase_incr : osc->phase_incr;
    for (int i=0; i<len; i++) {
	buf[i] = phase < PI ? -1.0 : 1.0;
	phase += phase_incr;
	if (phase > TAU) phase -= TAU;
    }
    osc->phase = phase;

}

static inline void fill_buf_tri(OscGeneric *osc, float *restrict buf, int len)
{
    double phase = osc->external_phase ? *osc->external_phase + osc->phase_shift : osc->phase;
    double phase_incr = osc->external_phase ? *osc->external_phase_incr : osc->phase_incr;
    double half_pi = PI / 2.0;
    double three_halves_pi = 3.0 * PI / 2.0;
    for (int i=0; i<len; i++) {
	if (phase < half_pi) {
	    buf[i] = phase / half_pi;
	} else if (phase < PI) {
	    buf[i] = (PI - phase) / half_pi;
	} else if (phase < three_halves_pi) {
	    buf[i] = -(phase - PI) / half_pi;
	} else {
	    buf[i] = -(TAU - phase) / half_pi;
	}
	phase += phase_incr;
	if (phase > TAU) phase -= TAU;
    }
    osc->phase = phase;
}

static inline void fill_buf_saw_up(OscGeneric *osc, float *restrict buf, int len)
{
    double phase = osc->external_phase ? *osc->external_phase + osc->phase_shift : osc->phase;\
    double phase_incr = osc->external_phase ? *osc->external_phase_incr : osc->phase_incr;
    for (int i=0; i<len; i++) {
	buf[i] = 2 * phase / TAU - 1.0;
	phase += phase_incr;
	if (phase > TAU) phase -= TAU;
    }
    osc->phase = phase;

}
static inline void fill_buf_saw_down(OscGeneric *osc, float *restrict buf, int len)
{
    double phase = osc->external_phase ? *osc->external_phase + osc->phase_shift : osc->phase;
    double src_phase_incr = osc->external_phase ? *osc->external_phase_incr : osc->phase_incr;
    double dst_phase_incr = osc->external_phase ? *osc->external_phase_incr_dst : osc->phase_incr;
    double diff = dst_phase_incr - src_phase_incr;
    bool do_grad = fabs(diff) > 1e-9;
    while (phase > TAU) phase -= TAU;
    for (int i=0; i<len; i++) {
	double phase_incr = do_grad ?
	    src_phase_incr + diff * (double)i / len
	    : src_phase_incr;
	    
	buf[i] = 2 * (TAU - phase) / TAU - 1.0;
	phase += phase_incr;
	if (phase > TAU) phase -= TAU;
    }
    osc->phase = phase;
}




void osc_generic_get_buf(OscGeneric *osc, float *restrict buf, int len)
{
    switch (osc->type) {
    case OSC_SINE:
	fill_buf_sine(osc, buf, len);
	break;
    case OSC_SQUARE:
	fill_buf_square(osc, buf, len);
	break;	
    case OSC_TRI:
	fill_buf_tri(osc, buf, len);
	break;
    case OSC_SAW_UP:
	fill_buf_saw_up(osc, buf, len);
	break;
    case OSC_SAW_DOWN:
	fill_buf_saw_down(osc, buf, len);
	break;
    }
}
