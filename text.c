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

/*****************************************************************************************************************
    text.c

    * Wrappers around SDL_TTF functions
    * "write_text" function provides simple interface for writing text to a rect
 *****************************************************************************************************************/

#include <stdbool.h>
#include "SDL.h"
#include "SDL_ttf.h"
#include "theme.h"
#include "text.h"
#include "project.h"

extern Project *proj;

TTF_Font *open_sans_fonts[11];

void init_SDL_ttf()
{
    if (TTF_Init() != 0) {
        fprintf(stderr, "\nError: TTF_Init failed: %s", TTF_GetError());
        exit(1);
    }
}

TTF_Font *open_font(const char* path, int size)
{
    TTF_Font *font = TTF_OpenFont(path, size);
    if (!font) {
        fprintf(stderr, "\nError: failed to open font: %s", TTF_GetError());
        exit(1);
    }
    return font;
}

/* Open a font at standard sizes, accounting for high DPI scaling */
void init_fonts(TTF_Font **font_array, const char *path, int arrlen)
{
    int standard_sizes[] = STD_FONT_SIZES;
    for (int i=0; i<arrlen; i++) {
        int size = standard_sizes[i];
        /* Account for high DPI scaling using global scale_factor */
        printf("Opening font at size: %d, scale fact: %d\n", size, proj->scale_factor);

        size *= proj->scale_factor;
        font_array[i] = open_font(path, size);
    }
}

void close_fonts(TTF_Font **font_array, int arrlen)
{
    for (int i=0; i<arrlen; i++) {
        TTF_CloseFont(font_array[i]);
    }

}

/* Write text to a SDL rect. If text fits, return 0. Else, truncate and return num truncated chars */
void write_text(
    SDL_Renderer *rend, 
    SDL_Rect *rect, 
    TTF_Font* font, 
    JDAW_Color* color, 
    const char *text, 
    bool allow_resize
)
{
    if (strlen(text) == 0) {
        return;
    }

    SDL_Rect new_rect = {rect->x, rect->y, 0, 0};
    SDL_Surface *surface;
    SDL_Texture *texture;
    SDL_Color sdl_col = get_color(color);
    surface = TTF_RenderUTF8_Blended(font, text, sdl_col);
    if (!surface) {
        fprintf(stderr, "\nError: TTF_RenderText_Blended failed: %s", TTF_GetError());
        return;
    }
    texture = SDL_CreateTextureFromSurface(rend, surface);

    SDL_QueryTexture(texture, NULL, NULL, &(new_rect.w), &(new_rect.h));
    if (!texture) {
        fprintf(stderr, "\nError: SDL_CreateTextureFromSurface failed: %s", TTF_GetError());
        return;
    }

    SDL_RenderCopy(rend, texture, NULL, &new_rect);

    // int SDL_QueryTexture(SDL_Texture * texture,
    //                  Uint32 * format, int *access,
    //                  int *w, int *h);
    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);
}