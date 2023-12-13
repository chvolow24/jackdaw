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
    mixdown.c

    * Get sampels from tracks/clips for playback or export
 *****************************************************************************************************************/

#include "project.h"

extern Project *proj;
/* Query track clips and return audio sample representing a given point in the timeline. 
channel is 0 for left or mono, 1 for right */
static double get_track_sample(Track *track, uint8_t channel, int32_t tl_pos_sframes)
{
    if (track->muted || track->solo_muted) {
        return 0;
    }

    double sample = 0;


    for (int i=0; i < track->num_clips; i++) {
        Clip *clip = (track->clips)[i];
        if (!(clip->done)) {
            continue;
        }
        double *clip_buf = (channel == 0) ? clip->L : clip->R;
        int32_t pos_in_clip_sframes = tl_pos_sframes - clip->abs_pos_sframes;
        // fprintf(stderr, "Pos in clip: %d\n", pos_in_clip_sframes);
        if (pos_in_clip_sframes >= 0 && pos_in_clip_sframes < clip->len_sframes) {
            if (pos_in_clip_sframes < clip->start_ramp_len) {
                double ramp_value = (double) pos_in_clip_sframes / clip->start_ramp_len;
                // ramp_value *= ramp_value;
                // ramp_value *= ramp_value;
                // ramp_value *= ramp_value;
                // fprintf(stderr, "(START RAMP) Pos in clip: %d; scale: %f\n", pos_in_clip, ramp_value);
                // fprintf(stderr, "\t Sample pre & post: %d, %d\n", (clip->post_proc)[pos_in_clip], (int16_t) ((clip->post_proc)[pos_in_clip] * ramp_value));
                sample += ramp_value * clip_buf[pos_in_clip_sframes];
            } else if (pos_in_clip_sframes > clip->len_sframes - clip->end_ramp_len) {
                double ramp_value = (double) (clip->len_sframes - pos_in_clip_sframes) / clip->end_ramp_len;
                // ramp_value *= ramp_value;
                // ramp_value *= ramp_value;
                // ramp_value *= ramp_value;
                // fprintf(stderr, "(END RAMP) Pos in clip: %d; scale: %f\n", pos_in_clip, ramp_value);
                // fprintf(stderr, "\t Sample pre & post: %d, %d\n", (clip->post_proc)[pos_in_clip], (int16_t) ((clip->post_proc)[pos_in_clip] * ramp_value));
                // sample += (int16_t) ((clip->post_proc)[pos_in_clip] * ramp_value);
                sample += ramp_value * clip_buf[pos_in_clip_sframes];
            } else {
                // if (clip->L == clip_buf) {
                //     fprintf(stderr, "Getting track L sample at %d: %f\n", pos_in_clip_sframes, clip_buf[pos_in_clip_sframes]);
                // } else {
                //     fprintf(stderr, "Getting track R sample at %d: %f\n", pos_in_clip_sframes, clip_buf[pos_in_clip_sframes]);

                // }
                sample += clip_buf[pos_in_clip_sframes];
            }
        }
    }

    return sample;
}

/* 
Sum track samples over a chunk of timeline and return an array of samples. from_mark_in indicates that samples
should be collected from the in mark rather than from the play head.
*/
double *get_mixdown_chunk(Timeline* tl, uint8_t channel, uint32_t len_sframes, bool from_mark_in)
{
    uint32_t chunk_len_bytes = sizeof(double) * len_sframes;
    double *mixdown = malloc(chunk_len_bytes);
    memset(mixdown, '\0', chunk_len_bytes);
    if (!mixdown) {
        fprintf(stderr, "\nError: could not allocate mixdown chunk.");
        exit(1);
    }

    int i=0;
    float j=0;
    uint32_t start_pos_sframes = from_mark_in ? proj->tl->in_mark_sframes : proj->tl->play_pos_sframes;
    while (i < len_sframes) {
        for (int t=0; t<tl->num_tracks; t++) {
            mixdown[i] += get_track_sample((tl->tracks)[t], channel, start_pos_sframes + (int32_t)j);
        }
        j += from_mark_in ? 1 : proj->play_speed;
        i++;
    }

    //TODO: need to move play position



    // move_play_position(len_samples * proj->play_speed / proj->channels);
    // proj->tl->play_position += len_samples * proj->play_speed / proj->channels;
    return mixdown;
}