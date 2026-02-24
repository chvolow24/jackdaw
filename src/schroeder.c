#include <stdbool.h>
#include "schroeder.h"
#include "allpass.h"
#include "dsp_utils.h"
#include "status.h"

static bool do_allpass = true;
static bool do_lop_delay = true;

void schroeder_toggle_do_allpass()
{
    do_allpass = !do_allpass;
    status_set_alertstr(do_allpass ? "allpass ON" : "allpass OFF");

}

void schroeder_toggle_do_lop_delay()
{
    do_lop_delay = !do_lop_delay;
    status_set_alertstr(do_lop_delay ? "lop delay ON" : "lop delay OFF");
}

/* static int32_t stereo_spread = 23 * 96000 / 44100; */
static float stereo_spread = 1.0f;

void schroeder_sample(Schroeder *sch, float in_L, float in_R, float *out_L, float *out_R)
{
    float allpassed_L = do_allpass ? allpass_group_sample(&sch->series_aps[0], in_L) : in_L;
    float allpassed_R = do_allpass ? allpass_group_sample(&sch->series_aps[1], in_R) : in_R;
    float intermed_L = 0.0f;
    float intermed_R = 0.0f;
    if (sch->num_parallel_lop_delays == 0 || !do_lop_delay) {
	intermed_L = allpassed_L;
	intermed_R = allpassed_R;
    } else {
	for (int i=0; i<sch->num_parallel_lop_delays; i++) {
	    float pan_scale_L = pan_scale(i % 2 == 0 ? stereo_spread : 1.0 - stereo_spread, 0);
	    float pan_scale_R = pan_scale(i % 2 == 0 ? stereo_spread : 1.0 - stereo_spread, 1);
	    intermed_L += pan_scale_L * /* pan_scale((float)i / sch->num_parallel_lop_delays, 0) * */ lop_delay_sample(&sch->parallel_lop_delays[0][i], allpassed_L) / sch->num_parallel_lop_delays;
	    intermed_R += pan_scale_R *  /* pan_scale((float)i / sch->num_parallel_lop_delays, 1) * */ lop_delay_sample(&sch->parallel_lop_delays[1][i], allpassed_R) / sch->num_parallel_lop_delays;
	}
    }
    *out_L = intermed_L;
    *out_R = intermed_R;
}

float schroeder_buf_apply(void *sch_v, float *restrict in_L, float *restrict in_R, int len, float input_amp)
{
    static float dry = 0.2;
    Schroeder *sch = sch_v;
    for (int32_t i=0; i<len; i++) {
	float old_L = in_L[i];
	float old_R = in_R[i];
	schroeder_sample(sch, in_L[i], in_R[i], in_L + i, in_R + i);
	in_L[i] = old_L * dry + (1 - dry) * in_L[i];
	in_R[i] = old_R * dry + (1 - dry) * in_R[i];
	
    }
    return input_amp;
}

static int delay_scalar = 1;

void schroeder_init_freeverb(Schroeder *sch)
{
    int32_t lens[4] = {
	225 * 96000 / 44100 * delay_scalar,
	556 * 96000 / 44100 * delay_scalar,
	441 * 96000 / 44100 * delay_scalar,
	341 * 96000 / 44100 * delay_scalar
	/* 3433, 4129, 6047, 7727 */
    };
    int32_t lens_r[4];
    for (int i=0; i<4; i++) {
	lens_r[i] = lens[i] + stereo_spread;
    }
    allpass_group_init(&sch->series_aps[0], 4, lens, 0.99);
    allpass_group_init(&sch->series_aps[1], 4, lens_r, 0.99);
    sch->num_parallel_lop_delays = 8;
    int32_t lop_lens[8] = {
	1557 * 96000 / 44100 * delay_scalar,
	1617 * 96000 / 44100 * delay_scalar,
	1491 * 96000 / 44100 * delay_scalar,
	1422 * 96000 / 44100 * delay_scalar,
	1277 * 96000 / 44100 * delay_scalar,
	1356 * 96000 / 44100 * delay_scalar,
	1188 * 96000 / 44100 * delay_scalar,
	1116 * 96000 / 44100 * delay_scalar
    };
    int32_t lop_lens_r[8];
    for (int i=0; i<8; i++) {
	lop_lens_r[i] = lop_lens[i] + stereo_spread;
	lop_delay_init(sch->parallel_lop_delays[0] + i, lop_lens[i], 0.99, 0.2);
	lop_delay_init(sch->parallel_lop_delays[1] + i, lop_lens_r[i], 0.99, 0.2);
    }
    /* sch->num_parallel_lop_delays = 8; */
}
