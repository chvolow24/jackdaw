#include "color.h"
const struct colors colors = {
    .white = {255, 255, 255, 255},
    .black = {0, 0, 0, 255},
    .clear = {0, 0, 0, 0},
    .grey = {127, 127, 127, 255},
    .light_grey = {220, 220, 220, 255},
    .dark_grey = {64, 64, 64, 255},
    .red = {255, 0, 0, 255},
    .green = {0, 255, 0, 255},
    .blue = {0, 0, 255, 255},
    .yellow = {255, 255, 0, 255},
    .burnt_umber = {138, 51, 36, 255},
    .sea_green = {59, 122, 87, 255},
    .amber = {255, 191, 0, 255},

    .quickref_button_blue = {35, 45, 55, 255},
    .quickref_button_pressed = {10, 20, 30, 255},
    .play_green = {0, 140, 50, 255},
    .undo_purple = {230, 30, 200, 255},
    .cerulean = {100, 190, 255, 255},
    .cerulean_pale = {100, 190, 255, 100},
    .dark_brown = {20, 20, 0, 255},
    .alert_orange = {255, 99, 0, 255},
    .mute_red = {240, 80, 80, 255},
    .solo_yellow = {255, 200, 50, 255},
    .mute_solo_grey = {210, 210, 210, 255},

    .tl_background_grey =  {50, 52, 55, 255},
    .control_bar_background_grey = {22, 28, 34, 255},
    .dropdown_green = {20, 100, 40, 255},
    .x_red = {200, 0, 0, 255},
    .min_yellow = {200, 90, 0, 255},
    .click_track = {10, 30, 25, 255},

    /* .midi_clip_pink = {237,204,232,255}, */
    .midi_clip_pink = {242, 188, 223, 230},
    /* .midi_clip_pink = {224, 142, 74, 255}, */
    .midi_clip_pink_grabbed = {255, 218, 243, 255},

    
    .freq_L = {130, 255, 255, 255},
    .freq_R = {255, 255, 130, 220},
};

void color_diff_set(ColorDiff *diff, SDL_Color a, SDL_Color b)
{
    diff->r = (int)a.r - b.r;
    diff->g = (int)a.g - b.g;
    diff->b = (int)a.b - b.b;
    diff->a = (int)a.a - b.a;
}

void color_diff_apply(const ColorDiff *diff, SDL_Color orig, double prop, SDL_Color *dst)
{
    ColorDiff ret;
    ret.r = orig.r + prop * diff->r;
    ret.g = orig.g + prop * diff->g;
    ret.b = orig.b + prop * diff->b;
    ret.a = orig.a + prop * diff->a;

    dst->r = ret.r < 0 ? 0 : ret.r > 255 ? 255 : ret.r;
    dst->g = ret.g < 0 ? 0 : ret.g > 255 ? 255 : ret.g;
    dst->b = ret.b < 0 ? 0 : ret.b > 255 ? 255 : ret.b;
    dst->a = ret.a < 0 ? 0 : ret.a > 255 ? 255 : ret.a;
}

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
