/*****************************************************************************************************************
  Jackdaw | https://jackdaw-audio.net/ | a free, keyboard-focused DAW | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023-2025 Charlie Volow
  
  Jackdaw is licensed under the GNU General Public License.

*****************************************************************************************************************/

#ifndef JDAW_TEXT_H
#define JDAW_TEXT_H

#include "SDL.h"
#include "SDL_ttf.h"
#include "theme.h"


#ifndef INSTALL_DIR
#define INSTALL_DIR ""
#endif

#define FREE_SANS INSTALL_DIR "/assets/ttf/free_sans.ttf"
#define OPEN_SANS INSTALL_DIR "/assets/ttf/OpenSans-Regular.ttf"
#define OPEN_SANS_BOLD INSTALL_DIR "/assets/ttf//OpenSans-Bold.ttf"
#define OPEN_SANS_VAR INSTALL_DIR "/assets/ttf/OpenSans-Variable.ttf"
#define COURIER_NEW INSTALL_DIR "/assets/ttf/CourierNew.ttf"
#define DROID_SANS_MONO INSTALL_DIR "/assets/ttf/DroidSansMono.ttf"
#define STD_FONT_SIZES {10, 12, 14, 16, 18, 24, 30, 36, 48, 60, 72}
#define STD_FONT_ARRLEN 11

// typedef struct textbox {
//     SDL_Rect *container;
//     char *text;
//     TTF_Font *font;
// } Textbox;

void init_SDL_ttf();
TTF_Font* open_font_(const char* path, int size);
void init_fonts(TTF_Font **font_array, const char *path);
void close_fonts(TTF_Font **font_array);
void write_text(
    SDL_Renderer *rend, 
    SDL_Rect *rect, 
    TTF_Font* font, 
    JDAW_Color* color, 
    const char *text, 
    bool allow_resize
);

#endif
