#include <stdbool.h>
#include "SDL2/SDL.h"
#include "SDL2/SDL_ttf.h"
#include "theme.h"

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

void close_font(TTF_Font* font)
{
    TTF_CloseFont(font);
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

    int rows = (w + rect->w - 1) / rect->w;

    // Resize or truncate
    if (rows * h > rect->h) {
        if (allow_resize) {
            rect->h = rows * h;
        }
    }
    int width = rows == 1 ? w : rect->w;
    SDL_Rect new_rect = {rect->x, rect->y, width, rows * h};
    SDL_Surface *surface;
    SDL_Texture *texture;
    SDL_Color sdl_col = get_color(color);
    surface = TTF_RenderUTF8_Blended_Wrapped(font, text, sdl_col, rect->w);
    if (!surface) {
        fprintf(stderr, "\nError: TTF_RenderText_Blended failed: %s", TTF_GetError());
        exit(1);
    }
    texture = SDL_CreateTextureFromSurface(rend, surface);
    if (!texture) {
        fprintf(stderr, "\nError: SDL_CreateTextureFromSurface failed: %s", TTF_GetError());
        exit(1);
    }

    SDL_RenderCopy(rend, texture, NULL, &new_rect);

    // // Present the renderer
    // SDL_RenderPresent(rend);

    // Clean up resources
    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);
}