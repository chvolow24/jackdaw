
#include <stdio.h>
#include <stdbool.h>
#include "SDL.h"
#include "draw.h"
#include "layout.h"
#include "lt_params.h"
#include "openfile.h"
#include "text.h"
#include "text.h"
#include "window.h"
#include "parse_xml.h"
#include "layout_xml.h"


#define OPEN_SANS_PATH "../../assets/ttf/OpenSans-Regular.ttf"


void init_SDL()
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "\nError initializing SDL: %s\n", SDL_GetError());
        exit(1);
    }
    SDL_SetHint(SDL_HINT_MOUSE_TOUCH_EVENTS, "1");


}

void init_SDL_ttf()
{
    if (TTF_Init() != 0) {
        fprintf(stderr, "\nError: TTF_Init failed: %s", TTF_GetError());
        exit(1);
    }

}





int main() 
{
    init_SDL();
    init_SDL_ttf();

    Window *win = window_create(500, 500, "Tests");

    bool quit = false;
    
    while (!quit) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                quit = true;
            }
	}

        SDL_Delay(1);
    }
}
