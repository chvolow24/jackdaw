#include "SDL.h"

struct colors {
    SDL_Color white;
    SDL_Color black;
    SDL_Color clear;
    SDL_Color grey;
    SDL_Color light_grey;
    SDL_Color red;
    SDL_Color green;
    SDL_Color blue;
    SDL_Color yellow;
    SDL_Color quickref_button_blue;
    SDL_Color quickref_button_pressed;
    SDL_Color play_green;
    SDL_Color undo_purple;
    SDL_Color cerulean;
    SDL_Color cerulean_pale;


    SDL_Color dropdown_green;
    SDL_Color x_red;
    SDL_Color min_yellow;
    SDL_Color click_track;
};

const struct colors colors = {
    .white = {255, 255, 255, 255},
    .black = {0, 0, 0, 255},
    .clear = {0, 0, 0, 0},
    .grey = {127, 127, 127, 255},
    .light_grey = {220, 220, 220, 255},
    .red = {255, 0, 0, 255},
    .green = {0, 255, 0, 255},
    .blue = {0, 0, 255, 255},
    .yellow = {255, 255, 0, 255},

    .quickref_button_blue = {35, 45, 55, 255},
    .quickref_button_pressed = {10, 20, 30, 255},
    .play_green = {0, 140, 50, 255},
    .undo_purple = {230, 30, 200, 255},
    .cerulean = {100, 190, 255, 255},
    .cerulean_pale = {100, 190, 255, 100},
    .dropdown_green = {20, 100, 40, 255},
    .x_red = {200, 0, 0, 255},
    .min_yellow = {200, 90, 0, 255},
    .click_track = {10, 30, 25, 255}

};

/* SDL_Color color_global_white = {255, 255, 255, 255}; */
/* SDL_Color color_global_black = {0, 0, 0, 255}; */
/* SDL_Color color_global_clear = {0, 0, 0, 0}; */
/* SDL_Color color_global_yellow = {255, 255, 0, 255}; */
/* SDL_Color color_global_grey = {127, 127, 127, 255}; */
/* SDL_Color color_global_light_grey = {220, 220, 220, 255}; */
/* SDL_Color color_global_red = {255, 0, 0, 255}; */
/* SDL_Color color_global_green = {0, 255, 0, 255}; */
/* SDL_Color color_global_blue = {0, 0, 255, 255}; */
/* /\* SDL_Color color_global_quickref_button_blue = {50, 90, 110, 255}; *\/ */

/* SDL_Color color_global_quickref_button_blue = {35, 45, 55, 255}; */
/* SDL_Color color_global_quickref_button_pressed = {10, 20, 30, 255}; */
/* SDL_Color color_global_play_green = {0, 140, 50, 255}; */
/* SDL_Color color_global_undo_purple = {230, 30, 200, 255}; */
/* SDL_Color color_global_cerulean = {100, 190, 255, 255}; */
/* SDL_Color color_global_cerulean_pale = {100, 190, 255, 100}; */


/* SDL_Color color_global_dropdown_green = {20, 100, 40, 255}; */
/* SDL_Color color_global_x_red = {200, 0, 0, 255}; */
/* SDL_Color color_global_min_yellow = {200, 90, 0, 255}; */

/* SDL_Color color_global_click_track = {10, 30, 25, 255}; */
