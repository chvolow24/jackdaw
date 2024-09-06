/*****************************************************************************************************************
  Jackdaw | a stripped-down, keyboard-focused Digital Audio Workstation | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023 Charlie Volow
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:
  
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
  
  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.

*****************************************************************************************************************/

/*****************************************************************************************************************
    automation.h

    * Define types related to parameter automation
    * Provide an interface for writing and reading automations
 *****************************************************************************************************************/


#ifndef JDAW_AUTOMATION_H
#define JDAW_AUTOMATION_H

#include "project.h"

typedef struct automation Automation;
typedef struct keyframe Keyframe;
typedef struct keyframe_clip KClip;

typedef enum automation_type {
    AUTO_VOL,
    AUTO_PAN,
    AUTO_FIR_FILTER_CUTOFF,
    AUTO_FIR_FILTER_BANDWIDTH,
    AUTO_DEL_TIME,
    AUTO_DEL_AMP,
    AUTO_DEL_STEREO_OFFSET
} AutomationType;

typedef struct keyframe {
    Automation *automation;
    int32_t pos;
    Value value;
    double m_fwd; /* slope (change in value per sample) */

    Keyframe *prev;
    Keyframe *next;
    
    KClip *kclip; /* Optional; used for replication of envelopes */
    int32_t pos_rel; /* Position relative to KClip start */
} Keyframe;

typedef struct automation {
    Track *track;
    AutomationType type;
    ValType val_type;
    void *target_val;
    
    bool read;
    bool write;
    
    Keyframe *first;
    Keyframe *last;
    Keyframe *current;
    /* ComponentFn read_action; */
} Automation;

typedef struct keyframe_clip {
    Keyframe *first;
    Keyframe *last;
} KClip;

typedef struct keyframe_clipref {
    KClip *kclip;
    Automation *automation;
    int32_t pos;
} KClipRef;


#endif
