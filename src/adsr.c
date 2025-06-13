#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "adsr.h"


void adsr_set_params(
    ADSRParams *p,
    int32_t a,
    int32_t d,
    float s,
    int32_t r,
    float ramp_exp)
{
    
    p->a = a;
    p->d = d;
    if (ramp_exp > 1e-4)
	p->s = pow(s, 1.0/ramp_exp);
    else
	p->s = s;
    p->r = r;
    p->ramp_exp = ramp_exp;

    if (p->a_ramp) free(p->a_ramp);
    if (p->d_ramp) free(p->d_ramp);
    /* if (p->r_ramp) free(p->r_ramp); */
    p->a_ramp = malloc(sizeof(float) * a);
    p->d_ramp = malloc(sizeof(float) * d);
    for (int32_t i=0; i<a; i++) {
	p->a_ramp[i] = (double)i / a;
    }
    for (int32_t i=0; i<d; i++) {
	p->d_ramp[i] = 1.0 - (float)i * (1.0 - p->s)/d;
    }
}


void adsr_init(ADSRState *s, int32_t after)
{
    /* fprintf(stderr, "\n\n\nNOTE INIT %p\n", s); */
    s->current_stage = ADSR_UNINIT;
    s->env_remaining = after;
    s->start_release_after = -1;
    s->release_start_env = 0.0;
    s->s_time = 0;
    /* s->env_remaining = s->params->a; */
}

void adsr_start_release(ADSRState *s, int32_t after)
{
    /* fprintf(stderr, "\n\n\n%p RELEASE NOTE AFTER %d\n", s, after); */
    s->start_release_after = after;
    /* if (s->current_stage >= ADSR_R) return; */
    /* s->current_stage = ADSR_R; */
    /* s->env_remaining = s->params->r; */
}

/* int Id=0; */

/* Fill a foat buffer with envelope values and return the end state */
enum adsr_stage adsr_get_chunk(ADSRState *s, float *restrict buf, int32_t buf_len)
{
    /* fprintf(stderr, "BUF APPLY len %d\n", buf_len); */
    /* fprintf(stderr, "\t\t\tadsr buf apply\n"); */
    int32_t buf_i = 0;
    bool skip_to_release = false;
    while (buf_i < buf_len) {
	/* fprintf(stderr, "\t\t\t\tbuf i: %d; stage %d; env_rem: %d; release_after: %d\n", buf_i, s->current_stage, s->env_remaining, s->start_release_after); */
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
	    break;
	case ADSR_A:
	    memcpy(buf + buf_i, s->params->a_ramp + s->params->a - s->env_remaining, stage_len * sizeof(float));
	    /* float_buf_mult( */
	    /* 	buf + buf_i, */
	    /* 	s->params->a_ramp + s->params->a - s->env_remaining, */
	    /* 	stage_len); */
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
		float env = (double)s->env_remaining * s->release_start_env / (double)s->params->r;
		buf[i] = env;
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
		fprintf(stderr, "Error: env stage error: release started in stage %d\n", s->current_stage);
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
	s->start_release_after -= stage_len;
	buf_i += stage_len;
    }
    return s->current_stage;
}

float adsr_sample(ADSRState *s, bool *is_finished)
{
    float env = 1.0;
    switch(s->current_stage) {
    case ADSR_UNINIT:
	env = 0.0;
	break;
    case ADSR_A:
	env = ((double)s->params->a - s->env_remaining) / s->params->a;
	break;
    case ADSR_D:
	env = 1.0 - (double)(s->params->d - s->env_remaining) * (1.0f - s->params->s) / (double)s->params->d;
	/* env = 1.0f + (float)s->env_remaining * (1.0f - s->params->s) / (float)s->params->d; */
	break;
    case ADSR_S:
	env = s->params->s;
	break;
    case ADSR_R:
	/* env = (double)s->env_remaining * s->release_start_env / (float)s->params->d; */
	/* fprintf(stderr, "->START ENV? %f\n", s->release_start_env); */
	/* if (s->release_start_env == 0) exit(1); */
	env = (double)s->env_remaining * s->release_start_env / (double)s->params->r;
	break;
    case ADSR_OVERRUN:
	env = 0.0;
	break;
    default:
	break;       
    }

    /* if (Id % 100 == 0) { */
    /* if (Id < 300 || Id % 300 == 0) { */
    /* 	fprintf(stderr, "Stage: %d, envr: %d, env: %f\n", s->current_stage, s->env_remaining, env); */
    /* } */
    /* Id++; */


    if (s->current_stage < ADSR_R) {
	/* fprintf(stderr, "\t\tSETTING release start env in stage %d\n", s->current_stage); */
	s->release_start_env = env;
	/* if (s->release_start_env == 0.0f) exit(1); */
    }

    if(s->current_stage != ADSR_S)
	s->env_remaining--;
    
    /* RESET STAGES based on env_remaining */
    if (s->env_remaining == 0) {
	s->current_stage++;
	switch(s->current_stage) {
	case ADSR_D:
	    s->env_remaining = s->params->d;
	    break;
	case ADSR_R:
	    s->env_remaining = s->params->r;
	    break;
	case ADSR_OVERRUN:
	    *is_finished = true;
	    break;
	default:
	    s->env_remaining = -1;
	    break;
	}
    }
    if (s->params->ramp_exp > 1e-4)
	env = pow(env, s->params->ramp_exp);
    return env;
}

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


