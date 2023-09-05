#include "SDL2/SDL.h"
#include "SDL2/SDL_ttf.h"

void init_SDL_ttf()
{
    if (TTF_Init() != 0) {
        fprintf(stderr, "\nError: TTF_Init failed: %s", TTF_GetError());
        exit(1);
    }
}

TTF_Font* open_font(const char* path, int size)
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

void write_text(SDL_Renderer *rend, SDL_Rect *rect, TTF_Font* font, SDL_Color* color, const char *text)
{

    SDL_Surface *surface;
    SDL_Texture *texture;
    //TODO: Implement TTF_SizeText() resizing of destination rect
    surface = TTF_RenderText_Blended(font, text, *color);
    if (!surface) {
        fprintf(stderr, "\nError: TTF_RenderText_Blended failed: %s", TTF_GetError());
        exit(1);
    }

    texture = SDL_CreateTextureFromSurface(rend, surface);
    if (!texture) {
        fprintf(stderr, "\nError: SDL_CreateTextureFromSurface failed: %s", SDL_GetError());
        exit(1);
    }

    SDL_RenderCopy(rend, texture, NULL, rect);

    // // Present the renderer
    // SDL_RenderPresent(rend);

    // Clean up resources
    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);
}