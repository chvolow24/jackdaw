#ifndef JDAW_ROUTE_H
#define JDAW_ROUTE_H

#include <stdbool.h>
#include "textbox.h"

typedef struct track Track;
typedef struct timeline Timeline;

struct route_tl_gui {
    Textbox *out_tb;
};

typedef struct audio_route {
    Track *dst;
    Track *src;
    float amp;
    /* bool pre_fader; */
    struct route_tl_gui tl_gui;
    
} AudioRoute;


void track_add_audio_route(Track *track, Track *dst, float init_amp);

void audio_route_destroy(AudioRoute *rt);
void audio_route_delete(AudioRoute *rt);

/*------ track deletion ----------------------------------------------*/

void audio_route_track_deleted(Track *track);
void audio_route_track_undeleted(Track *track);


void timeline_resort_tracks_proc_order(Timeline *tl);

#endif
