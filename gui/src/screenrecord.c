#include "SDL.h"
#include "window.h"

extern Window *main_win;

/********* Screenshot ********/
#define PREFERRED_PIXELFORMAT SDL_PIXELFORMAT_RGBA32
#define SCREENSHOT_MODULUS 1



/* Takes a bmp screenshot and saves to the 'images' subdirectory, with index i included in filename. */
void screenshot(int i, SDL_Renderer* rend)
{
  char filename[30];
  snprintf(filename, 30, "gifframes/screenshot%3d.bmp", i);
  SDL_Surface *sshot = SDL_CreateRGBSurfaceWithFormat(0, main_win->w_pix, main_win->h_pix, 32, PREFERRED_PIXELFORMAT);
  SDL_RenderReadPixels(rend, NULL, PREFERRED_PIXELFORMAT, sshot->pixels, sshot->pitch);
  SDL_SaveBMP(sshot, filename);
  SDL_FreeSurface(sshot);
}

void screenshot_loop()
{
    static int counter=0;
    static int frame=0;
    if (counter % SCREENSHOT_MODULUS == 0) {
	screenshot(frame, main_win->rend);
	frame++;
    }
    counter++;
    if (counter == SCREENSHOT_MODULUS * 100) {
	counter = 0;
    }
    counter%=SCREENSHOT_MODULUS;

}


// gif incantation:

// convert -delay 16 -loop 0 -resize 80% gifframes/*.bmp animation.gif
   

