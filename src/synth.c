#include <portmidi.h>
#include <porttime.h>
#include "synth.h"
#include "session.h"

extern double MTOF[];

Synth *synth_create()
{
    Session *session = session_get();
    Synth *s = calloc(1, sizeof(Synth));
    s->num_oscs = 1;
    /* s->osc_types[0] = WS_SINE; */
    for (int i=0; i<SYNTH_NUM_VOICES; i++) {
	s->voices[i].oscs[0].type = WS_SAW;
	s->voices[i].oscs[0].amp = 0.2;

	
	s->voices[i].synth = s;
	s->voices[i].available = true;
	/* s->voices[i].amp_env_stage =  */
    }
    s->amp_env.a = 96 * 1;
    s->amp_env.d = 96 * 20;
    s->amp_env.s = 0.5;
    s->amp_env.r = 96 * 200;
    s->monitor = true;
    /* session->midi_io.synths[session->midi_io.num_synths] = s; */
    return s;
}

/* int synth_create_virtual_device(Synth *s) */
/* { */
/*     if (call_count > 0) { */
/* 	fprintf(stderr, "CALL COUNT!\n"); */
/* 	exit(1); */
/*     } */
/*     call_count++; */
/*     static const char *device_name = "Jackdaw Synth"; */

/*     PmError err = Pm_CreateVirtualInput( */
/* 	device_name, */
/* 	MIDI_INTERFACE_NAME, */
/* 	NULL); */
/*     if (err  < 0) { */
/* 	fprintf(stderr, "Error creating virtual device: %s\n", Pm_GetErrorText(err)); */
/* 	exit(1); */
/*     } */
/*     /\* if (err == pmInterfaceNotSupported) { *\/ */
/*     /\* 	fprintf(stderr, "Error: Interface \"%s\" not supported\n", MIDI_INTERFACE_NAME); *\/ */
/*     /\* 	return -1; *\/ */
/*     /\* } else if (err == pmInvalidDeviceId) { *\/ */
/*     /\* 	fprintf(stderr, "Error: device name \"%s\" invalid or already in use\n", device_name); *\/ */
/*     /\* 	return -1; *\/ */
/*     /\* } else  *\/ */
/*     /\* PmDeviceID id = err; *\/ */
/*     /\* s->device.id = id; *\/ */
/*     /\* s->device.info = Pm_GetDeviceInfo(id); *\/ */
    
/*     /\* err = Pm_OpenInput(&s->device.stream, s->device.id, NULL, PM_EVENT_BUF_NUM_EVENTS, NULL, NULL); *\/ */
/*     if (err != pmNoError) { */
/* 	fprintf(stderr, "ERROR OPENING DEVICE: %s\n", Pm_GetErrorText(err)); */
/* 	exit(1); */
/*     } */
    
/*     return id; */
/* } */

static float osc_sample(Osc *osc, int channel, int num_channels)
{
    float sample;
    switch(osc->type) {
    case WS_SINE:
	sample = sin(TAU * osc->phase[channel]);
	break;
    case WS_SQUARE:
	sample = osc->phase[channel] < 0.5 ? -1.0 : 1.0;
	break;
    case WS_TRI:
	sample =
	    osc->phase[channel] <= 0.25 ?
	        osc->phase[channel] * 4.0f :
	    osc->phase[channel] <= 0.5f ?
	        (0.5f - osc->phase[channel]) * 4.0 :
	    osc->phase[channel] <= 0.75f ?
	        (osc->phase[channel] - 0.5f) * -4.0 :
	        (1.0f - osc->phase[channel]) * -4.0;
        break;
    case WS_SAW:
	sample = 2.0f * osc->phase[channel] - 1.0;
	break;
    default:
	sample = 0.0;
    }
    osc->phase[channel] += osc->sample_phase_incr;
    if (osc->phase[channel] > 1.0) {
	osc->phase[channel] -= 1.0;
    }
    return sample * osc->amp;
}


static void synth_voice_add_buf(SynthVoice *v, float *buf, int32_t len, int channel)
{
    if (v->available) return;
    /* fprintf(stderr, "\tvoice %p has data\n", v); */
    for (int i=0; i<v->synth->num_oscs; i++) {
	Osc *osc = v->oscs + i;
	/* fprintf(stderr, "\t\tosc %d, start rel %d end rel %d\n", i, v->note_start_rel, v->note_end_rel); */
	/* int32_t start_rel = v->note_start_rel; */
	/* int32_t end_rel = v->note_end_rel; */
	for (int i=0; i<len; i++) {
	    float sample = osc_sample(osc, channel, 2);
	    float env = 1.0f;
	    if (v->note_end_rel[channel] == 0) {
		v->amp_env_stage[channel] = 3;
		v->release_start_env[channel] = v->last_env[channel];
		v->env_remaining[channel] = v->synth->amp_env.r;
	    }
	    
	    if (v->env_remaining[channel] == 0) {
		v->amp_env_stage[channel]++;
		switch (v->amp_env_stage[channel]) {
		case 1:
		    v->env_remaining[channel] = v->synth->amp_env.d;
		    break;
		case 2:
		    v->env_remaining[channel] = -1;
		    env = v->synth->amp_env.s;
		    break;
		case 3:
		    v->env_remaining[channel] = v->synth->amp_env.r;
		    break;
		case 4:
		    v->available = true;
		    return; /* We've passed ADSR */
		}		
	    }
	    switch(v->amp_env_stage[channel]) {
	    case 0:
		env = (float)(v->synth->amp_env.a - v->env_remaining[channel]) / v->synth->amp_env.a;
		break;
	    case 1:
		/* env = v->synth->amp_env.s + (float)(v->env_remaining[channel]) / v->synth->amp_env.d; */
		env = v->synth->amp_env.s + ((float)v->env_remaining[channel] / v->synth->amp_env.d) * (1.0f - v->synth->amp_env.s);
		break;
	    case 2:
		env = v->synth->amp_env.s;
		break;
	    case 3:
		env = v->release_start_env[channel] - v->release_start_env[channel] * (float)(v->synth->amp_env.r - v->env_remaining[channel]) / v->synth->amp_env.r;
		break;
	    }
	    sample *= env * (float)v->velocity / 127.f;
 	    buf[i] += sample;
	    buf[i] = tanh(buf[i]);
	    v->last_env[channel] = env;
	    v->env_remaining[channel]--;
	    v->note_end_rel[channel]--;
	    v->note_start_rel[channel]--;
	    /* if (channel == 0) { */
	    /* 	fprintf(stderr, "Env stage %d, remaining %d\n", v->amp_env_stage[channel], v->env_remaining[channel]); */
	    /* } */
	}
	    /* if (start_rel < v->synth->amp_env.a) { */
	    /* 	/\* fprintf(stderr, "\t\t\t\tAttack!\n"); *\/ */
	    /* 	sample *= (float)start_rel / v->synth->amp_env.a; */
	    /* } else if (start_rel < v->synth->amp_env.a + v->synth->amp_env.d) { */
	    /* 	/\* fprintf(stderr, "\t\t\t\tDecay!\n"); *\/ */
	    /* 	sample *= v->synth->amp_env.s + (float)(start_rel - v->synth->amp_env.a) / v->synth->amp_env.d; */
	    /* } else if (end_rel < 0) { */
	    /* 	sample *= v->synth->amp_env.s; */
	    /* 	/\* fprintf(stderr, "\t\t\t\tSustain! * %f\n", v->synth->amp_env.s); *\/ */
	    /* } else if (end_rel < v->synth->amp_env.r) { */
	    /* 	/\* fprintf(stderr, "Num %d - %d\n", v->synth->amp_env.r, end_rel); *\/ */
	    /* 	sample *= v->synth->amp_env.s * (float)(v->synth->amp_env.r - end_rel) / v->synth->amp_env.r; */
	    /* 	/\* fprintf(stderr, "\t\t\t\tRelease!\n"); *\/ */
	    /* } else { */
	    /* 	v->available = true; */
	    /* 	/\* fprintf(stderr, "Passed ADSR!\n"); *\/ */
	    /* 	return; /\* We've passed ADSR *\/ */

	    /* } */
	    /* buf[i] += sample; */
	    /* start_rel++; */
	    /* end_rel++; */
	/* } */

	/* v->note_start_rel += len; */
	/* if (channel == 1) { */
	/*     v->note_start_rel += len; */
	/*     v->note_end_rel += len; */
	/* } */
    }
}

static void osc_set_freq(Osc *osc, double freq_hz)
{
    Session *session = session_get();
    osc->freq = freq_hz;
    osc->sample_phase_incr = freq_hz / session->proj.sample_rate;    
}
static void synth_voice_set_note(SynthVoice *v, uint8_t note_val, uint8_t velocity)
{
    v->note_val = note_val;
    for (int i=0; i<v->synth->num_oscs; i++) {
	osc_set_freq(v->oscs + i, MTOF[note_val]);
    }
    v->velocity = velocity;
}

void synth_add_buf(Synth *s, float *buf, int channel, int32_t len)
{
    /* FIRST: Update synth state from available MIDI data */
    if (channel != 0) goto get_buffer;
    if (s->num_events > 0)
	fprintf(stderr, "\t->Read %d events from synth\n", s->num_events);
    for (int i=0; i<s->num_events; i++) {
	/* PmEvent e = s->device.buffer[i]; */
	PmEvent e = s->events[i];
	uint8_t status = Pm_MessageStatus(e.message);
	uint8_t note_val = Pm_MessageData1(e.message);
	uint8_t velocity = Pm_MessageData2(e.message);
	uint8_t msg_type = status >> 4;

	/* if (i == 0) start = e.timestamp; */
	/* int32_t pos_rel = ((double)e.timestamp - start) * (double)session->proj.sample_rate / 1000.0; */
	/* fprintf(stderr, "EVENT %d/%d, timestamp: %d (rel %d) pos rel %d (record start %d)\n", i, num_read, e.timestamp, current_time - e.timestamp, pos_rel, d->record_start); */
	if (velocity == 0) msg_type = 8;
	if (msg_type == 8) {
	    /* HANDLE NOTE OFF */
	    fprintf(stderr, "note off received, val %d\n", note_val);
	    for (int i=0; i<SYNTH_NUM_VOICES; i++) {
		SynthVoice *v = s->voices + i;
		if (!v->available && v->note_val == note_val) {
		    fprintf(stderr, "v%d noteval %d\n", i, v->note_val);
		    fprintf(stderr, "SETTING END REL!\n");
		    v->note_end_rel[0] = 0;
		    v->note_end_rel[1] = 0;
		}		
	    }
	} else if (msg_type == 9) {
	    for (int i=0; i<SYNTH_NUM_VOICES; i++) {
		SynthVoice *v = s->voices + i;
		if (v->available) {
		    synth_voice_set_note(v, note_val, velocity);
		    v->note_start_rel[0] = 0;
		    v->note_start_rel[1] = 0;
		    v->note_end_rel[0] = INT32_MIN;
		    v->note_end_rel[1] = INT32_MIN;
		    v->available = false;
		    v->amp_env_stage[0] = 0;
		    v->amp_env_stage[1] = 0;
		    v->env_remaining[0] = s->amp_env.a;
		    v->env_remaining[1] = s->amp_env.a;
		    v->oscs[0].phase[0] = 0.0;
		    v->oscs[0].phase[1] = 0.0;
		    fprintf(stderr, "Setting voice %d to not available !\n", i);
		    break;
		}
	    }
	}
    }
    s->num_events = 0;

get_buffer:

    /* SECOND: get buffers from voices */
    for (int i=0; i<SYNTH_NUM_VOICES; i++) {
	SynthVoice *v = s->voices + i;
	synth_voice_add_buf(v, buf, len, channel);		
    }    
}
