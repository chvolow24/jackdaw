#include "logscale.h"


/* 'min/max_scaled' define the x axis range within 'container.' E.g. 40:nyquist for frequency plotting */
void logscale_init(Logscale *ls, SDL_Rect *container, double min_scaled, double max_scaled)
{
    ls->container = container;
    ls->min_scaled = min_scaled;
    ls->max_scaled = max_scaled;
    ls->base = max_scaled / min_scaled;
}

/* Maps "linear" in range 0:1 to value in range ls->min_scaled:ls->max_scaled */
double logscale_from_linear(Logscale *ls, double linear)
{
    return pow(ls->base, linear) * ls->min_scaled;
}

/* Maps value in range ls->min_scaled:ls->max_scaled to linear range 0:1 */
double logscale_to_linear(Logscale *ls, double scaled)
{
    return log2(scaled / ls->min_scaled) / log2(ls->base);    
}

/* Get x position of a value, relative to container rect */
int logscale_x_rel(Logscale *ls, double scaled)
{
    return logscale_to_linear(ls, scaled) * ls->container->w;
}

/* (Vertical usage) get the y position of a value, relative to the container rect */
int logscale_y_rel(Logscale *ls, double scaled)
{
    return ls->container->h - logscale_to_linear(ls, scaled) * ls->container->h;
}

/* Get absolute draw x position of a value */
int logscale_x_abs(Logscale *ls, double scaled)
{
    return logscale_x_rel(ls, scaled) + ls->container->x;
}

/* (Vertical usage) get absolute draw y position of a value */
int logscale_y_abs(Logscale *ls, double scaled)
{
    return logscale_y_rel(ls, scaled) + ls->container->y;
}

/* Get a value from x relative to container */
double logscale_val_from_x_rel(Logscale *ls, int x)
{
    double xprop = (double)x / ls->container->w;
    return logscale_from_linear(ls, xprop);
}

/* (Vertical usage) get a value from y relative to container */
double logscale_val_from_y_rel(Logscale *ls, int y)
{
    double yprop = (double)(ls->container->h - y) / ls->container->h;
    return logscale_from_linear(ls, yprop);
}


/* Get a value from absolute draw x */
double logscale_val_from_x_abs(Logscale *ls, int x)
{
    return logscale_val_from_x_rel(ls, x - ls->container->x);
}

/* (Vertical usage) get a value from absolute draw y */
double logscale_val_from_y_abs(Logscale *ls, int y)
{
    return logscale_val_from_y_rel(ls, y - ls->container->y);
}

