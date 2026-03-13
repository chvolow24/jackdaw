/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

#include <stdlib.h>
#include "audio_clip.h"
#include "clipref.h"
#include "session.h"

#define DEFAULT_REFS_ALLOC_LEN 2

/* mandatory after alloc */
void clip_init(Clip *clip)
{
    pthread_mutex_init(&clip->buf_realloc_lock, NULL);
    clip->refs_alloc_len = DEFAULT_REFS_ALLOC_LEN;
    clip->refs = calloc(clip->refs_alloc_len, sizeof(ClipRef *));
}

Clip *clip_create(AudioConn *conn, Track *target)
{
    Session *session = session_get();
    if (session->proj.num_clips == MAX_PROJ_CLIPS) {
	return NULL;
    }
    Clip *clip = calloc(1, sizeof(Clip));

    if (conn) {
	clip->recorded_from = conn;
	if (conn->type == DEVICE) {
	    clip->channels = conn->channel_cfg.R_src >= 0 ? 2 : 1; /* If R src specified, clip is stereo; else mono */
	} else {
	    clip->channels = 2;
	}
    }
    if (target) {
	clip->target = target;
    }

    clip_init(clip);
    if (!target && conn) {
	snprintf(clip->name, sizeof(clip->name), "%s_rec_%d", conn->name, session->proj.num_clips); /* TODO: Fix this */
    } else if (target) {
	snprintf(clip->name, sizeof(clip->name), "%s take %d", target->name, target->num_takes + 1);
	target->num_takes++;
    } else {
	snprintf(clip->name, sizeof(clip->name), "anonymous");
    }
    /* clip->channels = proj->channels; */
    session->proj.clips[session->proj.num_clips] = clip;
    session->proj.num_clips++;
    return clip;
}


void clip_destroy_no_displace(Clip *clip)
{
    pthread_mutex_destroy(&clip->buf_realloc_lock);
    for (uint16_t i=0; i<clip->num_refs; i++) {
	ClipRef *cr = clip->refs[i];
	clipref_destroy(cr, false);
    }
    Session *session = session_get();
    if (clip == session->source_mode.src_clip) {
	session->source_mode.src_clip = NULL;
    }
    
    if (clip->L) free(clip->L);
    if (clip->R) free(clip->R);

    for (uint8_t i=0; i<session->source_mode.num_dropped; i++) {
	Clip **dropped_clip = &(session->source_mode.saved_drops[i].clip);
	if (*dropped_clip == clip) {
	    *dropped_clip = NULL;
	}
    }
    free(clip->refs);
    free(clip);
}

void clip_destroy(Clip *clip)
{
    pthread_mutex_destroy(&clip->buf_realloc_lock);
    /* fprintf(stdout, "CLIP DESTROY %s, num refs: %d\n", clip->name,  clip->num_refs); */
    /* fprintf(stdout, "DESTROYING CLIP %p, num: %d\n", clip, proj->num_clips); */
    for (uint16_t i=0; i<clip->num_refs; i++) {
	/* fprintf(stdout, "About to destroy clipref\n"); */
	ClipRef *cr = clip->refs[i];
	clipref_destroy(cr, false);
    }
    Session *session = session_get();
    if (clip == session->source_mode.src_clip) {
	session->source_mode.src_clip = NULL;
    }
    bool displace = false;
    /* int num_displaced = 0; */
    Project *proj = &session->proj;
    for (uint16_t i=0; i<proj->num_clips; i++) {
	if (proj->clips[i] == clip) {
	    /* fprintf(stdout, "\tFOUND clip at pos %d\n", i); */
	    displace = true;
	} else if (displace && i > 0) {
	    /* fprintf(stdout, "\tmoving clip %p from pos %d to %d\n", proj->clips[i], i, i-1); */
	    proj->clips[i-1] = proj->clips[i];
	    /* num_displaced++; */
	}
    }
    /* fprintf(stdout, "\t->num displaced: %d\n", num_displaced); */
    proj->num_clips--;
    proj->active_clip_index = proj->num_clips;
    if (clip->L) free(clip->L);
    if (clip->R) free(clip->R);

    for (uint8_t i=0; i<session->source_mode.num_dropped; i++) {
	Clip **dropped_clip = &(session->source_mode.saved_drops[i].clip);
	if (*dropped_clip == clip) {
	    *dropped_clip = NULL;
	}
    }
    free(clip->refs);
    free(clip);
}

void clip_split_stereo_to_mono(Clip *to_split, Clip **new_L, Clip **new_R)
{
    *new_L = clip_create(to_split->recorded_from, to_split->target);
    *new_R = clip_create(to_split->recorded_from, to_split->target);

    (*new_L)->recording = false;
    (*new_R)->recording = false;
    (*new_L)->channels = 1;
    (*new_R)->channels = 1;
    
    (*new_L)->L = malloc(to_split->len_sframes * sizeof(float));
    (*new_R)->L = malloc(to_split->len_sframes * sizeof(float));
    
    memcpy((*new_L)->L, to_split->L, to_split->len_sframes * sizeof(float));
    memcpy((*new_R)->L, to_split->R, to_split->len_sframes * sizeof(float));


    (*new_L)->len_sframes = to_split->len_sframes;
    (*new_R)->len_sframes = to_split->len_sframes;
    Session *session = session_get();
    session->proj.active_clip_index+=2;
}

/* Assumes realloc has been done */
static void clip_waveform_append(Clip *clip, int32_t start_in_clip, int32_t len_sframes)
{
    int32_t ck64_i = start_in_clip / 64;
    float ck512_Lmin = 1.0f;
    float ck512_Lmax = -1.0f;
    float ck512_Rmin = 1.0f;
    float ck512_Rmax = -1.0f;
    int32_t ck512_i = ck64_i / 8;
    int32_t mod = ck64_i % 8;
    if (mod != 0) {
	for (int32_t i=ck64_i - mod; i<ck64_i; i++) {
	    WaveformChunk Lck = clip->waveform.ck64[0][i];
	    ck512_Lmin = fminf(ck512_Lmin, Lck.min);
	    ck512_Lmax = fmaxf(ck512_Lmax, Lck.max);
	    if (clip->R) {
		WaveformChunk Rck = clip->waveform.ck64[1][i];
		ck512_Rmin = fminf(ck512_Rmin, Rck.min);
		ck512_Rmax = fmaxf(ck512_Rmax, Rck.max);
	    }
	}
    }
    int32_t ck_len;
    while ((ck_len = len_sframes - start_in_clip) > 0) {
	ck_len = 64 < ck_len ? 64 : ck_len;
	WaveformChunk *Lck = clip->waveform.ck64[0] + ck64_i;
	float min = 1.0;
	float max = -1.0;
	for (int32_t i=0; i<ck_len; i++) {
	    min = fminf(clip->L[start_in_clip + i], min);
	    max = fmaxf(clip->L[start_in_clip + i], max);
	}
	Lck->min = min;
	Lck->max = max;

	ck512_Lmin = fminf(min, ck512_Lmin);
	ck512_Lmax = fmaxf(max, ck512_Lmax);
	
	if (clip->channels > 1) {	
	    WaveformChunk *Rck = clip->waveform.ck64[1] + ck64_i;
	    min = 1.0;
	    max = -1.0;
	    for (int32_t i=0; i<ck_len; i++) {
		min = fminf(clip->R[start_in_clip + i], min);
		max = fmaxf(clip->R[start_in_clip + i], max);
	    }
	    Rck->min = min;
	    Rck->max = max;
	    ck512_Rmin = fminf(min, ck512_Rmin);
	    ck512_Rmax = fmaxf(max, ck512_Rmax);
	}
	ck64_i++;
	start_in_clip += ck_len;
	if (ck64_i % 8 == 0) {
	    WaveformChunk *Lck512 = clip->waveform.ck512[0] + ck512_i;
	    Lck512->min = ck512_Lmin;
	    Lck512->max = ck512_Lmax;
	    if (clip->R) {
		WaveformChunk *Rck512 = clip->waveform.ck512[1] + ck512_i;
		Rck512->min = ck512_Rmin;
		Rck512->max = ck512_Rmax;
	    }
	    ck512_Lmin = 1.0;
	    ck512_Lmax = -1.0;
	    ck512_Rmin = 1.0;
	    ck512_Rmax = -1.0;
	    ck512_i++;
	}
    }
}


/* Waveform ops */
void clip_init_or_update_waveform(Clip *clip)
{
    int32_t len_sframes = clip->len_sframes;
    fprintf(stderr, "AT call to update/init, init len vs clip len: %d %d\n", clip->waveform.init_len, len_sframes);
    if (clip->waveform.init_len == len_sframes) {
	return;
    }
    int32_t start_in_clip = 0;
    clip->waveform.clip = clip;
    clip->waveform.num_channels = clip->channels;
    if (clip->waveform.init_len == 0) {
	pthread_mutex_init(&clip->waveform.lock, NULL);
	pthread_mutex_lock(&clip->waveform.lock);
	clip->waveform.init_len = len_sframes;
	clip->waveform.num_ck64 = ceil((double)len_sframes / 64);    
	clip->waveform.ck64[0] = malloc(clip->waveform.num_ck64 * sizeof(WaveformChunk));
	if (clip->R) {
	    clip->waveform.ck64[1] = malloc(clip->waveform.num_ck64 * sizeof(WaveformChunk));
	}
	int32_t num_ck512 = clip->waveform.num_ck64 / 8;
	if (num_ck512) {
	    clip->waveform.ck512[0] = malloc(num_ck512 * sizeof(WaveformChunk));
	    if (clip->R) {
		clip->waveform.ck512[1] = malloc(num_ck512 * sizeof(WaveformChunk));
	    }
	}
    } else {
	/* Reinit */
	pthread_mutex_lock(&clip->waveform.lock);
	start_in_clip = clip->waveform.init_len;
	clip->waveform.init_len = len_sframes;
	clip->waveform.num_ck64 = ceil((double)len_sframes / 64);
	clip->waveform.ck64[0] = realloc(clip->waveform.ck64[0], clip->waveform.num_ck64 * sizeof(WaveformChunk));
	if (clip->R) {
	    clip->waveform.ck64[1] = realloc(clip->waveform.ck64[1], clip->waveform.num_ck64 * sizeof(WaveformChunk));
	}
	int32_t num_ck512 = clip->waveform.num_ck64 / 8;
	if (num_ck512) {
	    clip->waveform.ck512[0] = realloc(clip->waveform.ck512[0], num_ck512 * sizeof(WaveformChunk));
	    if (clip->R) {
		clip->waveform.ck512[1] = realloc(clip->waveform.ck512[1], num_ck512 * sizeof(WaveformChunk));
	    }
	}	
    }
    clip_waveform_append(clip, start_in_clip, len_sframes);
    pthread_mutex_unlock(&clip->waveform.lock);
    return;
    /* int32_t ck64_i = 0; */
    /* int32_t ck512_i = 0; */
    /* float ck512_Lmin = 1.0; */
    /* float ck512_Lmax = -1.0; */
    /* float ck512_Rmin = 1.0; */
    /* float ck512_Rmax = -1.0; */

    /* /\* int32_t start_in_clip = 0; *\/ */
    /* int32_t ck_len; */
    /* while ((ck_len = len_sframes - start_in_clip) > 0) { */
    /* 	ck_len = 64 < ck_len ? 64 : ck_len; */
    /* 	WaveformChunk *Lck = clip->waveform.ck64[0] + ck64_i; */
    /* 	float min = 1.0; */
    /* 	float max = -1.0; */
    /* 	for (int32_t i=0; i<ck_len; i++) { */
    /* 	    min = fminf(clip->L[start_in_clip + i], min); */
    /* 	    max = fmaxf(clip->L[start_in_clip + i], max); */
    /* 	} */
    /* 	Lck->min = min; */
    /* 	Lck->max = max; */

    /* 	ck512_Lmin = fminf(min, ck512_Lmin); */
    /* 	ck512_Lmax = fmaxf(max, ck512_Lmax); */
	
    /* 	if (clip->channels > 1) {	 */
    /* 	    WaveformChunk *Rck = clip->waveform.ck64[1] + ck64_i; */
    /* 	    min = 1.0; */
    /* 	    max = -1.0; */
    /* 	    for (int32_t i=0; i<ck_len; i++) { */
    /* 		min = fminf(clip->R[start_in_clip + i], min); */
    /* 		max = fmaxf(clip->R[start_in_clip + i], max); */
    /* 	    } */
    /* 	    Rck->min = min; */
    /* 	    Rck->max = max; */
    /* 	    ck512_Rmin = fminf(min, ck512_Rmin); */
    /* 	    ck512_Rmax = fmaxf(max, ck512_Rmax); */
    /* 	} */
    /* 	ck64_i++; */
    /* 	start_in_clip += ck_len; */
    /* 	if (ck64_i % 8 == 0) { */
    /* 	    WaveformChunk *Lck512 = clip->waveform.ck512[0] + ck512_i; */
    /* 	    Lck512->min = ck512_Lmin; */
    /* 	    Lck512->max = ck512_Lmax; */
    /* 	    if (clip->R) { */
    /* 		WaveformChunk *Rck512 = clip->waveform.ck512[1] + ck512_i; */
    /* 		Rck512->min = ck512_Rmin; */
    /* 		Rck512->max = ck512_Rmax; */
    /* 	    } */
    /* 	    ck512_Lmin = 1.0; */
    /* 	    ck512_Lmax = -1.0; */
    /* 	    ck512_Rmin = 1.0; */
    /* 	    ck512_Rmax = -1.0; */
    /* 	    ck512_i++; */
    /* 	} */
    /* } */
}
