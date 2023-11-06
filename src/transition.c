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
// #include "theme.h"

extern uint8_t scale_factor;

#define BOUNDARY_PROX_LIMIT (12 * scale_factor)
#define MAX_BOUNDARIES 10
#define abs_lesser_of(a,b) (abs(a) < abs(b) ? (struct p){a,true} : (struct p){b,false})

extern Project *proj;
extern JDAW_Color white;
extern JDAW_Color black;
extern JDAW_Color clear;

typedef struct clip_boundary {
    Clip *clip;
    bool left;
} ClipBoundary;

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


static void *transition_loop(void *arg)
{
    TransitionLoopArg *arg2 = (TransitionLoopArg *)arg;
    // ClipBoundary **boundaries = arg2->boundaries;
    uint8_t num_boundaries = arg2->num_boundaries;
    Textbox *entry = arg2->entry;


    // // double transition_length = 0;
    // bool quit = false;
    char explain_text[50];
    sprintf(explain_text, "Add transition to %d clip boundaries.", num_boundaries);
    Textbox *explain = create_textbox(0, 0, 10, 10, proj->jwin->bold_fonts[3], explain_text, &white, &clear, &clear, NULL, NULL, NULL, NULL, 0, true, false, BOTTOM_LEFT);
    Textbox *entry_label = create_textbox(0, 0, 10, 10, proj->jwin->bold_fonts[3], "Enter length in seconds:\t", &white, &clear, &clear, NULL, NULL, NULL, NULL, 0, true, false, BOTTOM_LEFT);
    // Textbox *entry = create_textbox(0, 0, 10, proj->jwin->bold_fonts[3], "0", &white, &clear, &clear, NULL, NULL, NULL, NULL, 0, true, false);
    position_textbox(explain, 100 * scale_factor, 200 * scale_factor);
    position_textbox(entry_label, explain->container.x, explain->container.y + explain->container.h + PADDING);
    position_textbox(entry, entry_label->container.x + entry_label->container.w, entry_label->container.y);
    
    // while (!quit) {
    //     SDL_Event e;
    //     while (SDL_PollEvent(&e)) {
    //         if (e.type == SDL_QUIT || (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_CLOSE)) {
    //             quit = true;
    //         }
    //     }
    // SDL_RenderClear(proj->jwin->rend);
    draw_project(proj);
    SDL_Rect bckgrnd = {0, 0, proj->jwin->w, proj->jwin->h};
    SDL_SetRenderDrawColor(proj->jwin->rend, 20, 20, 20, 232);
    SDL_RenderFillRect(proj->jwin->rend, &bckgrnd);
    draw_textbox(proj->jwin->rend, explain);
    draw_textbox(proj->jwin->rend, entry_label);
    draw_textbox(proj->jwin->rend, entry);
    // SDL_RenderPresent(proj->jwin->rend);
    // }
    return NULL;
}

void add_transition()
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
        edit_textbox(entry, transition_loop, (void *)(&arg));
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


