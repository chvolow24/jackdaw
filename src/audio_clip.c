/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

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
