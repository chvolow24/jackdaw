#include <stdbool.h>
#include "SDL.h"
#include "theme.h"

extern bool dark_mode;



SDL_Color get_color(JDAW_Color *color)
{
    if (dark_mode) {
        return color->dark;
    } else {
        return color->light;
    }
}