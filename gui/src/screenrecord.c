#include "SDL.h"
#include "window.h"

extern Window *main_win;

/********* Screenshot ********/

int screenshot_index = 0;

/* Takes a bmp screenshot and saves to the 'images' subdirectory, with index i included in filename. */
void screenshot(int i, SDL_Renderer* rend)
{
  char filename[30];
  sprintf(filename, "gifframes/screenshot%3d.bmp", i);
  printf("\nSaved %s", filename);
  SDL_Surface *sshot = SDL_CreateRGBSurface(0, main_win->w, main_win->h, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000);
  SDL_RenderReadPixels(rend, NULL, SDL_PIXELFORMAT_ARGB8888, sshot->pixels, sshot->pitch);
  SDL_SaveBMP(sshot, filename);
  SDL_FreeSurface(sshot);
}

/******************************/
