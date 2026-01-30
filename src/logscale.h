#ifndef JDAW_LOGSCALE_H
#define JDAW_LOGSCALE_H

#include <stdbool.h>
#include "SDL.h"

typedef struct logscale {
    SDL_Rect *container;
    double min_scaled;
    double max_scaled;
    double base;
} Logscale;


#include "logscale.h"


/* 'min/max_scaled' define the x axis range within 'container.' E.g. 40:nyquist for frequency plotting */
void logscale_init(Logscale *ls, SDL_Rect *container, double min_scaled, double max_scaled);

/* Maps "linear" in range 0:1 to value in range ls->min_scaled:ls->max_scaled */
double logscale_from_linear(Logscale *ls, double linear);

/* Maps value in range ls->min_scaled:ls->max_scaled to linear range 0:1 */
double logscale_to_linear(Logscale *ls, double scaled);

/* Get x position of a value, relative to container rect */
int logscale_x_rel(Logscale *ls, double scaled);

/* (Vertical usage) get the y position of a value, relative to the container rect */
int logscale_y_rel(Logscale *ls, double scaled);

/* Get absolute draw x position of a value */
int logscale_x_abs(Logscale *ls, double scaled);

/* (Vertical usage) get absolute draw y position of a value */
int logscale_y_abs(Logscale *ls, double scaled);

/* Get a value from x relative to container */
double logscale_val_from_x_rel(Logscale *ls, int x);

/* (Vertical usage) get a value from y relative to container */
double logscale_val_from_y_rel(Logscale *ls, int y);

/* Get a value from absolute draw x */
double logscale_val_from_x_abs(Logscale *ls, int x);

/* (Vertical usage) get a value from absolute draw y */
double logscale_val_from_y_abs(Logscale *ls, int y);


#endif
