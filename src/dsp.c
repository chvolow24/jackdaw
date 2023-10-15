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
    dsp.c

    * Digital signal processing
    * Clip buffers are run through functions here to populate post-proc buffer
 *****************************************************************************************************************/


#include "project.h"
#include "gui.h"

extern Project *proj;

 
void process_track_vol_and_pan(Track *track)
{
    fprintf(stderr, "Processing vol and pan for track at %p\n", track);
    Clip *clip = NULL;
    float pan, lpan, rpan, panctrlval;
    panctrlval = track->pan_ctrl->value;
    lpan = panctrlval < 0 ? 1 : 1 - panctrlval;
    rpan = panctrlval > 0 ? 1 : 1 + panctrlval;
    for (uint8_t i=0; i<track->num_clips; i++) {
        clip = track->clips[i];
        fprintf(stderr, "\t->clip index %d\n", i);
        uint8_t k=0;
        for (uint32_t j=0; j<clip->length; j++) {
            pan = j%2==0 ? lpan : rpan;
            if (k<20) {
                k++;
                fprintf(stderr, "\t\t->sample %d, pan value: %f\n", j, pan);
            }
            clip->post_proc[j] = clip->pre_proc[j] * track->vol_ctrl->value * pan;
        }
    }
    fprintf(stderr, "\t->Done with track vol and pan.\n");

}

void process_vol_and_pan()
{
    if (!proj) {
        fprintf(stderr, "Error: request to process vol and pan for nonexistent project.\n");
        return;
    }
    Track *track = NULL;
    for (uint8_t i=0; i<proj->tl->num_active_tracks; i++) {
        track = proj->tl->tracks[proj->tl->active_track_indices[i]];
        process_track_vol_and_pan(track);
    }
}