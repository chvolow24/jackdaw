#include <stdbool.h>
#include "SDL2/SDL.h"
#include "SDL2/SDL_ttf.h"
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
int write_text(
    SDL_Renderer *rend, 
    SDL_Rect *rect, 
    TTF_Font* font, 
    JDAW_Color* color, 
    const char *text, 
    bool allow_resize
)
{
    int w, h;
    if (TTF_SizeText(font, text,  &w, &h) !=0) {
        fprintf(stderr, "Error: could not size text size (SDLErr:  %s)", SDL_GetError());
    }

    SDL_Rect new_rect = {rect->x, rect->y, w, h};
    SDL_Surface *surface;
    SDL_Texture *texture;
    SDL_Color sdl_col = get_color(color);
    surface = TTF_RenderUTF8_Blended(font, text, (SDL_Color){255, 255, 255, 255});
    if (!surface) {
        fprintf(stderr, "\nError: TTF_RenderText_Blended failed: %s", TTF_GetError());
        exit(1);
    }
    texture = SDL_CreateTextureFromSurface(rend, surface);

    SDL_QueryTexture(texture, NULL, NULL, &(new_rect.w), &(new_rect.h));
    if (!texture) {
        fprintf(stderr, "\nError: SDL_CreateTextureFromSurface failed: %s", TTF_GetError());
        exit(1);
    }

    SDL_RenderCopy(rend, texture, NULL, &new_rect);

    // int SDL_QueryTexture(SDL_Texture * texture,
    //                  Uint32 * format, int *access,
    //                  int *w, int *h);
    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);
}