#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "adsr.h"
#include "endpoint_callbacks.h"
#include "session.h"
#include <math.h>
/* #include "color.h" */



static void dsp_cb_attack(Endpoint *ep)
{
    int msec_prev = ep->overwrite_val.int_v;
    int msec = endpoint_safe_read(ep, NULL).int_v;
    int32_t samples_prev = (double)msec_prev * (double)session_get_sample_rate() / 1000.0;
    int32_t samples = (double)msec * (double)session_get_sample_rate() / 1000.0;
    
    ADSRParams *p = ep->xarg1;
    adsr_reset_env_remaining(p, ADSR_A, samples - samples_prev);
    adsr_set_params(p, samples, p->d, p->s_ep_targ, p->r, p->ramp_exp);
}

static void dsp_cb_decay(Endpoint *ep)
{
    int msec_prev = ep->overwrite_val.int_v;
    int msec = endpoint_safe_read(ep, NULL).int_v;
    int32_t samples_prev = msec_prev * session_get_sample_rate() / 1000;
    int32_t samples = msec * session_get_sample_rate() / 1000;
    ADSRParams *p = ep->xarg1;
    adsr_reset_env_remaining(p, ADSR_D, samples - samples_prev);
    adsr_set_params(p, p->a, samples, p->s_ep_targ, p->r, p->ramp_exp);
}

static void dsp_cb_sustain(Endpoint *ep)
{
    float s = endpoint_safe_read(ep, NULL).float_v;
    ADSRParams *p = ep->xarg1;
    adsr_set_params(p, p->a, p->d, s, p->r, p->ramp_exp);
}


static void dsp_cb_release(Endpoint *ep)
{
    int msec_prev = ep->overwrite_val.int_v;
    int msec = endpoint_safe_read(ep, NULL).int_v;
    int32_t samples = msec * session_get_sample_rate() / 1000;
    int32_t samples_prev = msec_prev * session_get_sample_rate() / 1000;
    ADSRParams *p = ep->xarg1;
    adsr_reset_env_remaining(p, ADSR_R, samples - samples_prev);
    adsr_set_params(p, p->a, p->d, p->s_ep_targ, samples, p->ramp_exp);
}
static void dsp_cb_ramp_exp(Endpoint *ep)
{
    ADSRParams *p = ep->xarg1;
    adsr_set_params(p, p->a, p->d, p->s_ep_targ, p->r, p->ramp_exp);
}

void adsr_endpoints_init(ADSRParams *p, Page **cb_page, APINode *parent_node, char *node_name)
{
    api_node_register(&p->api_node, parent_node, node_name);
    endpoint_init(
	&p->a_ep,
	&p->a_msec,
	JDAW_INT,
	"attack",
	"Attack",
	JDAW_THREAD_DSP,
	page_el_gui_cb, NULL, dsp_cb_attack,
	p, NULL, cb_page, "attack_slider");
    endpoint_set_default_value(&p->a_ep, (Value){.int_v = 8});
    endpoint_set_allowed_range(&p->a_ep, (Value){.int_v = 0}, (Value){.int_v = 2000});
    endpoint_set_label_fn(&p->a_ep, label_msec);
    api_endpoint_register(&p->a_ep, &p->api_node);

    endpoint_init(
	&p->d_ep,
	&p->d_msec,
	JDAW_INT,
	"decay",
	"Decay",
	JDAW_THREAD_DSP,
	page_el_gui_cb, NULL, dsp_cb_decay,
	p, NULL, cb_page, "decay_slider");
    endpoint_set_default_value(&p->d_ep, (Value){.int_v = 200});
    endpoint_set_allowed_range(&p->d_ep, (Value){.int_v = 0}, (Value){.int_v = 2000});
    endpoint_set_label_fn(&p->d_ep, label_msec);
    api_endpoint_register(&p->d_ep, &p->api_node);

    endpoint_init(
	&p->s_ep,
	&p->s_ep_targ,
	JDAW_FLOAT,
	"sustain",
	"Sustain",
	JDAW_THREAD_DSP,
	page_el_gui_cb, NULL, dsp_cb_sustain,
	p, NULL, cb_page, "sustain_slider");
    endpoint_set_default_value(&p->s_ep, (Value){.float_v = 0.4f});
    endpoint_set_allowed_range(&p->s_ep, (Value){.float_v = 0.0f}, (Value){.float_v = 1.0f});
    api_endpoint_register(&p->s_ep, &p->api_node);

    endpoint_init(
	&p->r_ep,
	&p->r_msec,
	JDAW_INT,
	"release",
	"Release",
	JDAW_THREAD_DSP,
	page_el_gui_cb, NULL, dsp_cb_release,
	p, NULL, cb_page, "release_slider");
    endpoint_set_default_value(&p->r_ep, (Value){.int_v = 300});
    endpoint_set_allowed_range(&p->r_ep, (Value){.int_v = 0}, (Value){.int_v = 2000});
    endpoint_set_label_fn(&p->r_ep, label_msec);
    api_endpoint_register(&p->r_ep, &p->api_node);


    endpoint_init(
	&p->ramp_exp_ep,
	&p->ramp_exp,
	JDAW_FLOAT,
	"ramp_exponent",
	"Ramp exponent",
	JDAW_THREAD_DSP,
	page_el_gui_cb, NULL, dsp_cb_ramp_exp,
	p, NULL, cb_page, "ramp_exp_slider");
    endpoint_set_default_value(&p->ramp_exp_ep, (Value){.float_v = 2.0});
    endpoint_set_allowed_range(&p->ramp_exp_ep, (Value){.float_v = 1.0}, (Value){.float_v = 20.0});
    api_endpoint_register(&p->ramp_exp_ep, &p->api_node);
}

void adsr_params_add_follower(ADSRParams *p, ADSRState *follower)
{
    follower->params = p;
    if (p->followers_arrlen == 0) {
	p->followers_arrlen = 8;
	p->followers = calloc(p->followers_arrlen, sizeof(ADSRState *));
    }
    if (p->num_followers == p->followers_arrlen) {
	if ((long)p->followers_arrlen * 2 > INT_MAX) {
	    fprintf(stderr, "Error: num followers on ADSRParams exceeds INT_MAX\n");
	    return;
	}
	p->followers_arrlen *= 2;
	p->followers = realloc(p->followers, p->followers_arrlen * sizeof(ADSRState *));
    }
    p->followers[p->num_followers] = follower;
    p->num_followers++;
}

void adsr_reset_env_remaining(ADSRParams *p, enum adsr_stage stage, int32_t delta)
{
    for (int i=0; i<p->num_followers; i++) {
        ADSRState *s = p->followers[i];
	if (s->current_stage == stage) {
	    /* fprintf(stderr, "p %p STAGE %d delta %d %d->%d\n", p, stage, delta, s->env_remaining, s->env_remaining + delta); */
	    s->env_remaining += delta;
	    if (s->env_remaining < 0) {
		s->env_remaining = 0;
	    }
	}
    }
}

void adsr_set_params(
    ADSRParams *p,
    int32_t a,
    int32_t d,
    float s,
    int32_t r,
    float ramp_exp)
{
    /* fprintf(stderr, "SET PARAMS %p %d %d %f %d %f\n", p,a, d, s, r, ramp_exp); */
    /* const char *thread = get_thread_name(); */
    /* fprintf(stderr, "PARAM CALL ON THREAD %s\n", thread); */
    
    /* First, clear all "env remaining" values */
    /* for (int i=0; i<p->num_followers; i++) { */
    /* 	p->followers[i]->env_remaining = 0; */
    /* } */
    
    p->a = a;
    p->d = d;
    /* if (ramp_exp > 1e-4) */
    /* 	p->s = pow(s, 1.0/ramp_exp); */
    /* else */
	p->s = s;
    p->r = r;
    p->ramp_exp = ramp_exp;

    if (p->a_ramp) free(p->a_ramp);
    if (p->d_ramp) free(p->d_ramp);
    /* if (p->r_ramp) free(p->r_ramp); */
    p->a_ramp = malloc(sizeof(float) * a);
    p->d_ramp = malloc(sizeof(float) * d);
    for (int32_t i=0; i<a; i++) {
	p->a_ramp[i] = pow((double)i / a, ramp_exp);
    }
    for (int32_t i=0; i<d; i++) {
	float norm = (float)(d - i) / d;
	norm = pow(norm, ramp_exp);
	float scaled = norm * (1.0 - p->s);
	float shifted = scaled + p->s;
	p->d_ramp[i] = shifted;
    }
}

void adsr_init(ADSRState *s, int32_t after)
{
    s->current_stage = ADSR_UNINIT;
    s->env_remaining = after;
    s->start_release_after = -1;
    s->release_start_env = 0.0;
    s->s_time = 0;
    #ifdef TESTBUILD
    if (after < 0) {
	breakfn();
    }
    #endif
    /* s->env_remaining = s->params->a; */
}

void adsr_start_release(ADSRState *s, int32_t after)
{
    /* fprintf(stderr, "\t\t\tADSR %p start release after : %d\n", s, after); */
    s->start_release_after = after;
    #ifdef TESTBUILD
    if (after < 0) {
	breakfn();
    }
    #endif
}

/* int Id=0; */

/* Fill a foat buffer with envelope values and return the end state */
enum adsr_stage adsr_get_chunk(ADSRState *s, float *restrict buf, int32_t buf_len)
{
    /* fprintf(stderr, "GET CHUNK state %p, STAGE: %d, ENV_R: %d, REL_AFTER: %d\n", s, s->current_stage, s->env_remaining, s->start_release_after); */
    /* const char *thread = get_thread_name(); */
    /* fprintf(stderr, "\tget chunk CALL ON THREAD %s\n", thread); */
    /* fprintf(stderr, "\t\tint %p\n", s); */
    /* fprintf(stderr, "\t\t\tadsr buf apply\n"); */
    int32_t buf_i = 0;
    bool skip_to_release = false;
    while (buf_i < buf_len) {
	/* fprintf(stderr, "\t\t\t\tbuf i: %d; stage %d; env_rem: %d; release_after: %d\n", buf_i, s->current_stage, s->env_remaining, s->start_release_after); */
	if (buf_i > buf_len || buf_i < 0) {
	    breakfn();
	}
	int32_t stage_len = s->env_remaining < buf_len - buf_i ? s->env_remaining : buf_len - buf_i;
	if (s->start_release_after >= 0 && s->start_release_after < stage_len) {
	    stage_len = s->start_release_after;
	    skip_to_release = true;
	    if (s->start_release_after == 0) {
		goto reset_stage;
	    }
	}
	switch(s->current_stage) {
	case ADSR_UNINIT:
	    memset(buf + buf_i, '\0', sizeof(float) * stage_len);
	    s->env_remaining -= stage_len;
	    #ifdef TESTBUILD
	    if (s->env_remaining < 0) breakfn();
	    #endif
	    break;
	case ADSR_A:
	    memcpy(buf + buf_i, s->params->a_ramp + s->params->a - s->env_remaining, stage_len * sizeof(float));
	    s->env_remaining -= stage_len;
	    break;
	case ADSR_D:
	    memcpy(buf + buf_i, s->params->d_ramp + s->params->d - s->env_remaining, stage_len * sizeof(float));
 	    /* float_buf_mult( */
	    /* 	buf + buf_i, */
	    /* 	s->params->d_ramp + s->params->d - s->env_remaining, */
	    /* 	stage_len); */
	    s->env_remaining -= stage_len;
	    break;
	case ADSR_S:
	    for (int32_t i=0; i<stage_len; i++) {
		buf[buf_i + i] = s->params->s;
	    }
	    /* memset(buf + buf_i, s->params->s,  */
	    /* float_buf_mult_const( */
	    /* 	buf + buf_i, */
	    /* 	s->params->s, */
	    /* 	stage_len); */
	    s->env_remaining -= stage_len;
	    s->s_time += stage_len;
	    break;
	case ADSR_R: {
	    for (int i=0; i<stage_len; i++) {
		float env_norm = (double)s->env_remaining / (double)s->params->r;
		float env = s->release_start_env * pow(env_norm, s->params->ramp_exp);
		buf[i + buf_i] = env;
		s->env_remaining--;
	    }
	}
	    break;
	case ADSR_OVERRUN:
	    /* Fill remainder of buffer with silence and exit */
	    memset(buf + buf_i, '\0', sizeof(float) * (buf_len - buf_i));
	    return ADSR_OVERRUN;
	}
    reset_stage:
	if (skip_to_release) {
	    switch(s->current_stage) {
	    case ADSR_A:
		s->release_start_env = s->params->a_ramp[s->params->a - s->env_remaining];
		s->current_stage = ADSR_R;
		s->env_remaining = s->params->r;
		s->start_release_after = -1;

		break;
	    case ADSR_D:
		s->release_start_env = s->params->d_ramp[s->params->d - s->env_remaining];
		s->current_stage = ADSR_R;
		s->env_remaining = s->params->r;
		s->start_release_after = -1;

		break;
	    case ADSR_S:
		s->release_start_env = s->params->s;
		s->current_stage = ADSR_R;
		s->env_remaining = s->params->r;
		s->start_release_after = -1;
		break;
	    case ADSR_R:
		s->current_stage = ADSR_R;
		s->start_release_after = -1;
		break;
	    default:
		s->release_start_env = s->params->s;
		s->current_stage = ADSR_R;
		s->start_release_after = -1;
		/* fprintf(stderr, "Error: env stage error: release started in stage %d\n", s->current_stage); */
		break;
	    }
	    /* fprintf(stderr, "\nSET START OF RELEASE ENV from stage %d, env rem: %d, val: %f\n", s->current_stage, s->env_remaining, s->release_start_env); */
	    skip_to_release = false;
	} else if (s->env_remaining == 0) {
	    if (s->current_stage < ADSR_OVERRUN)
		s->current_stage++;
	    switch(s->current_stage) {
	    case ADSR_A: s->env_remaining = s->params->a; break;
	    case ADSR_D: s->env_remaining = s->params->d; break;
	    case ADSR_S: s->env_remaining = INT32_MAX; break;
	    case ADSR_R: s->env_remaining = s->params->r; break;
	    case ADSR_OVERRUN: s->env_remaining = 0; break;
	    case ADSR_UNINIT:
		fprintf(stderr, "Error: Env stage error\n");
		return ADSR_UNINIT;
	    }
	}
	if (s->start_release_after > 0) {
	    s->start_release_after -= stage_len;
	}
	buf_i += stage_len;
    }
    /* for (int i=0; i<buf_len; i++) { */
    /* 	int fc = fpclassify(buf[i]); */
    /* 	if (fc < FP_ZERO || fabs(buf[i]) > 10.0f) { */
    /* 	    fprintf(stderr, "i %d == %f\n", i, buf[i]); */
    /* 	    exit(1); */
    /* 	} */
    /* } */
    /* fprintf(stderr, "\t\t\t\t%p current stage %d\n", s, s->current_stage); */
    return s->current_stage;
}

/* float adsr_sample(ADSRState *s, bool *is_finished) */
/* { */
/*     float env = 1.0; */
/*     switch(s->current_stage) { */
/*     case ADSR_UNINIT: */
/* 	env = 0.0; */
/* 	break; */
/*     case ADSR_A: */
/* 	env = ((double)s->params->a - s->env_remaining) / s->params->a; */
/* 	break; */
/*     case ADSR_D: */
/* 	env = 1.0 - (double)(s->params->d - s->env_remaining) * (1.0f - s->params->s) / (double)s->params->d; */
/* 	/\* env = 1.0f + (float)s->env_remaining * (1.0f - s->params->s) / (float)s->params->d; *\/ */
/* 	break; */
/*     case ADSR_S: */
/* 	env = s->params->s; */
/* 	break; */
/*     case ADSR_R: */
/* 	/\* env = (double)s->env_remaining * s->release_start_env / (float)s->params->d; *\/ */
/* 	/\* fprintf(stderr, "->START ENV? %f\n", s->release_start_env); *\/ */
/* 	/\* if (s->release_start_env == 0) exit(1); *\/ */
/* 	env = (double)s->env_remaining * s->release_start_env / (double)s->params->r; */
/* 	break; */
/*     case ADSR_OVERRUN: */
/* 	env = 0.0; */
/* 	break; */
/*     default: */
/* 	break;        */
/*     } */

/*     /\* if (Id % 100 == 0) { *\/ */
/*     /\* if (Id < 300 || Id % 300 == 0) { *\/ */
/*     /\* 	fprintf(stderr, "Stage: %d, envr: %d, env: %f\n", s->current_stage, s->env_remaining, env); *\/ */
/*     /\* } *\/ */
/*     /\* Id++; *\/ */


/*     if (s->current_stage < ADSR_R) { */
/* 	/\* fprintf(stderr, "\t\tSETTING release start env in stage %d\n", s->current_stage); *\/ */
/* 	s->release_start_env = env; */
/* 	/\* if (s->release_start_env == 0.0f) exit(1); *\/ */
/*     } */

/*     if(s->current_stage != ADSR_S) */
/* 	s->env_remaining--; */
    
/*     /\* RESET STAGES based on env_remaining *\/ */
/*     if (s->env_remaining == 0) { */
/* 	s->current_stage++; */
/* 	switch(s->current_stage) { */
/* 	case ADSR_D: */
/* 	    s->env_remaining = s->params->d; */
/* 	    break; */
/* 	case ADSR_R: */
/* 	    s->env_remaining = s->params->r; */
/* 	    break; */
/* 	case ADSR_OVERRUN: */
/* 	    *is_finished = true; */
/* 	    break; */
/* 	default: */
/* 	    s->env_remaining = -1; */
/* 	    break; */
/* 	} */
/*     } */
/*     if (s->params->ramp_exp > 1e-4) */
/* 	env = pow(env, s->params->ramp_exp); */
/*     /\* fprintf(stderr, " *\/ */
/*     return env; */
/* } */

int32_t adsr_query_position(ADSRState *s)
{
    int32_t pos = 0;
    switch(s->current_stage) {
    case ADSR_OVERRUN:
    case ADSR_R:
	pos += s->params->r;
    case ADSR_S:
	pos += s->s_time;
    case ADSR_D:
	pos += s->params->d;
    case ADSR_A:
	pos += s->params->a;
    case ADSR_UNINIT:
	break;
    }
    if (s->current_stage != ADSR_S) {
	pos -= s->env_remaining;
    }
    return pos;
}
