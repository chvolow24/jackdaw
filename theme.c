#include <stdbool.h>
#include "SDL2/SDL.h"
#include "theme.h"

extern bool dark_mode;

JDAW_Color bckgrnd_color = {{255, 240, 200, 255}, {22, 28, 34, 255}};
JDAW_Color txt_soft = {{50, 50, 50, 255}, {200, 200, 200, 255}};
JDAW_Color txt_main = {{10, 10, 10, 255}, {240, 240, 240, 255}};

JDAW_Color tl_bckgrnd = {{240, 235, 235, 255}, {50, 52, 55, 255}};
JDAW_Color track_bckgrnd = {{168, 168, 162, 255}, {83, 98, 127, 255}};

JDAW_Color play_head = {{0, 0, 0, 255}, {255, 255, 255, 255}};

SDL_Color get_color(JDAW_Color *color)
{
    if (dark_mode) {
        return color->dark;
    } else {
        return color->light;
    }
}