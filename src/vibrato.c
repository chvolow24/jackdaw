#include "endpoint_callbacks.h"
#include "mod_delay.h"
#include "session.h"
#include "vibrato.h"

#define MAX_VIB_DELAY_LEN (session_get_sample_rate() / 4)
#define VIB_DEFAULT_DEPTH 0.1
#define VIB_DEFAULT_FREQ_HZ 2
#define VIB_MAX_FREQ_HZ 200.0
#define VIB_HALF_FREQ_HZ 20.0

static void freq_dsp_cb(Endpoint *ep)
{
    const double n = log(VIB_HALF_FREQ_HZ / VIB_MAX_FREQ_HZ) / log(0.5);
    double ctrl = ep->current_write_val.double_v;
    Vibrato *vib = ep->xarg1;
    vib->freq_hz = VIB_MAX_FREQ_HZ * pow(ctrl, n);
    mod_delay_set_freq(&vib->mdL, vib->freq_hz);
    mod_delay_set_freq(&vib->mdR, vib->freq_hz);
}

static void depth_dsp_cb(Endpoint *ep)
{
    double depth_ctrl = ep->current_write_val.double_v;
    Vibrato *vib = ep->xarg1;
    vib->depth = depth_ctrl * depth_ctrl;
    mod_delay_set_amp(&vib->mdL, vib->depth);
    mod_delay_set_amp(&vib->mdR, vib->depth);
}



void vibrato_init(Vibrato *vib)
{
    mod_delay_init(
	&vib->mdL,
	MAX_VIB_DELAY_LEN,
	VIB_DEFAULT_DEPTH,
	VIB_DEFAULT_FREQ_HZ,
	1,
	OSC_SINE);

    mod_delay_init(
	&vib->mdR,
	MAX_VIB_DELAY_LEN,
	VIB_DEFAULT_DEPTH,
	VIB_DEFAULT_FREQ_HZ,
	1,
	OSC_SINE);

    endpoint_init(
	&vib->freq_hz_ep,
	&vib->freq_ctrl,
	JDAW_DOUBLE,
	"freq_hz",
	"Freq hz",
	JDAW_THREAD_DSP,
	component_gui_cb, NULL, freq_dsp_cb,
	vib, NULL, NULL, NULL);
    endpoint_set_allowed_range(
	&vib->freq_hz_ep,
	(Value){.double_v = 0.001},
	(Value){.double_v = 1.0});
    api_endpoint_register(&vib->freq_hz_ep, &vib->effect->api_node);

    endpoint_init(
	&vib->depth_ep,
	&vib->depth_ctrl,
	JDAW_DOUBLE,
	"depth",
	"Depth",
	JDAW_THREAD_DSP,
	component_gui_cb, NULL, depth_dsp_cb,
	vib, NULL, NULL, NULL);
    endpoint_set_allowed_range(
	&vib->depth_ep,
	(Value){.double_v = 0.0},
	(Value){.double_v = 0.9});
    api_endpoint_register(&vib->depth_ep, &vib->effect->api_node);
    
	
}

float vibrato_buf_apply(void *vib_v, float *restrict L, float *restrict R, int len, float input_amp)
{
    Vibrato *vib = vib_v;
    if (L)
	mod_delay_buf(&vib->mdL, L, len);
    if (R) {
	mod_delay_buf(&vib->mdR, R, len);
	if (L) {
	    vib->mdR.phase = vib->mdL.phase;
	}
    }
    
    return input_amp;

}
void vibrato_clear(Vibrato *vib)
{
    mod_delay_clear(&vib->mdL);
    mod_delay_clear(&vib->mdR);
}

void vibrato_deinit(Vibrato *vib)
{
    mod_delay_deinit(&vib->mdL);
    mod_delay_deinit(&vib->mdR);
}

