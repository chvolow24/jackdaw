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
#include <stdbool.h>
#include <stdio.h>
#include "SDL_render.h"
#include "project.h"
#include "timeline.h"
#include "gui.h"
#include "draw.h"
#include "transition.h"
// #include "theme.h"

extern uint8_t scale_factor;

#define BOUNDARY_PROX_LIMIT (12 * scale_factor)
#define MAX_BOUNDARIES 10
#define abs_lesser_of(a,b) (abs(a) < abs(b) ? (struct p){a,true} : (struct p){b,false})

extern Project *proj;
extern JDAW_Color white;
extern JDAW_Color black;
extern JDAW_Color clear;

struct p {
    int lesser_distance;
    bool left;
};

static uint8_t get_clip_boundaries(ClipBoundary **boundaries)
{
    uint8_t num_boundaries = 0;
    // ClipBoundary **boundaries = malloc(sizeof(ClipBoundary *) * MAX_BOUNDARIES);
    if (!boundaries) {
        fprintf(stderr, "Error: failed to allocate space for boundaries.\n");
        return 0;
    }

    int playhead_x = get_tl_draw_x(proj->tl->play_position);
    Track *track = NULL;
    for (uint8_t i=0; i<proj->tl->num_active_tracks; i++) {
        track = proj->tl->tracks[proj->tl->active_track_indices[i]];
        Clip *clip = NULL;
        for (uint8_t j=0; j<track->num_clips; j++) {
            clip = track->clips[j];
            struct p check = abs_lesser_of(playhead_x - clip->rect.x, playhead_x - (clip->rect.x + clip->rect.w));
            if (abs(check.lesser_distance) <= BOUNDARY_PROX_LIMIT) {
                ClipBoundary *bound = malloc(sizeof(ClipBoundary));
                bound->clip = clip;
                bound->left = check.left;
                boundaries[num_boundaries] = bound;
                num_boundaries ++;
                if (num_boundaries == MAX_BOUNDARIES) {
                    break;
                }
            }
        }
        if (num_boundaries == MAX_BOUNDARIES) {
            break;
        }
    }
    return num_boundaries;

}

typedef struct transition_loop_arg {
    ClipBoundary **boundaries;
    uint8_t num_boundaries;
    Textbox *entry;
} TransitionLoopArg;

void add_transition_from_tl()
{
    ClipBoundary **boundaries = malloc(sizeof(ClipBoundary*) * MAX_BOUNDARIES);
    uint8_t num_boundaries = get_clip_boundaries(boundaries);
    uint32_t boundary_length_sframes;
    if (num_boundaries > 0) {
        fprintf(stderr, "Found %d boundaries.\n", num_boundaries);
        for (uint8_t i=0; i<num_boundaries; i++) {
            fprintf(stderr, "\tClip named %s --- left? %d\n", boundaries[i]->clip->name, boundaries[i]->left);
        }
        char value_str[50];
        value_str[0] = '\0';
        Textbox *entry = create_textbox(0, 0, 10, 10, proj->jwin->bold_fonts[3], value_str, &white, &clear, &clear, NULL, NULL, NULL, NULL, 0, true, false, BOTTOM_LEFT);
        struct transition_loop_arg arg = {boundaries, num_boundaries, entry};
        edit_textbox(entry, draw_transition_modal, (void *)(&arg));
        // transition_loop_length = get_transition_length_loop(boundaries, num_boundaries);
        boundary_length_sframes = atof(entry->value) * proj->sample_rate;
        if (boundary_length_sframes == 0) {
            fprintf(stderr, "Unable to get float value from boundary length entry.\n");
            //TODO: actual error handling
        }
     }


    for (uint8_t i=0; i<num_boundaries; i++) {
        ClipBoundary *boundary = boundaries[i];
        uint32_t *ramp_ptr = boundary->left ? &(boundary->clip->start_ramp_len) : &(boundary->clip->end_ramp_len);
        *ramp_ptr = boundary_length_sframes;
        free(boundary);
    }
    free(boundaries);
}

void add_clip_transition(Clip *clip, bool left)
{
    ClipBoundary *boundary = malloc(sizeof(ClipBoundary));
    boundary->left = left;
    boundary->clip = clip;
    uint32_t boundary_length_sframes;
    char value_str[50];
    value_str[0] = '\0';
    Textbox *entry = create_textbox(0, 0, 10, 10, proj->jwin->bold_fonts[3], value_str, &white, &clear, &clear, NULL, NULL, NULL, NULL, 0, true, false, BOTTOM_LEFT);
    struct transition_loop_arg arg = {&boundary, 1, entry};
    edit_textbox(entry, draw_transition_modal, (void *)(&arg));
    // transition_loop_length = get_transition_length_loop(boundaries, num_boundaries);
    boundary_length_sframes = atof(entry->value) * proj->sample_rate;
    if (boundary_length_sframes == 0) {
        fprintf(stderr, "Unable to get float value from boundary length entry.\n");
        //TODO: actual error handling
    }

    uint32_t *ramp_ptr = boundary->left ? &(boundary->clip->start_ramp_len) : &(boundary->clip->end_ramp_len);
    *ramp_ptr = boundary_length_sframes;
    free(boundary);
}



