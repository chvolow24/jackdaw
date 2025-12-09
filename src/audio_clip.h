/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

/*****************************************************************************************************************
    audio_clip.h

    * container for all persistent linear PCM audio data
    * referenced on timeline as ClipRef (see clipref.h)
    * can be recorded directly from a audio device (audio_conn.h)
    * MIDI counterpart in midi_clip.h
*****************************************************************************************************************/

#ifndef JDAW_AUDIO_CLIP_H
#define JDAW_AUDIO_CLIP_H

#include "textbox.h"

typedef struct clip_ref ClipRef;
typedef struct track Track;
typedef struct audio_conn AudioConn;

typedef struct clip {
    char name[MAX_NAMELENGTH];
    bool deleted;
    uint8_t channels;
    uint32_t len_sframes;
    /* ClipRef *refs[MAX_CLIP_REFS]; */
    /* uint16_t num_refs; */
    
    ClipRef **refs;
    uint16_t num_refs;
    uint16_t refs_alloc_len;
    
    float *L;
    float *R;
    uint32_t write_bufpos_sframes;
    /* Recording in */
    Track *target;
    bool recording;
    AudioConn *recorded_from;
} Clip;


/* mandatory after alloc */
void clip_init(Clip *clip);

/* Higher-level than alloc->init */
Clip *clip_create(AudioConn *dev, Track *target);

void clip_destroy(Clip *clip);
void clip_destroy_no_displace(Clip *clip);
void clip_split_stereo_to_mono(Clip *to_split, Clip **new_L, Clip **new_R);

#endif
