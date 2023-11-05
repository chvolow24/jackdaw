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
    draw.c

    * Drawing functions
        -> incl. "draw_project," which is called on every iteration of the animation loop.
 *****************************************************************************************************************/

#include <float.h>
#include <math.h>
#include <string.h>
#include "SDL.h"
#include "SDL_render.h"
#include "theme.h"
#include "project.h"
#include "text.h"
#include "theme.h"
#include "audio.h"
#include "gui.h"
#include "timeline.h"

extern Project *proj;
extern uint8_t scale_factor;
extern bool dark_mode;

JDAW_Color red = {{255, 0, 0, 255},{255, 0, 0, 255}};
JDAW_Color green = {{0, 255, 0, 255},{0, 255, 0, 255}};
JDAW_Color blue = {{0, 0, 255, 255},{0, 0, 255, 255}};
JDAW_Color white = {{255, 255, 255, 255},{255, 255, 255, 255}};
JDAW_Color grey = {{128, 128, 128, 255}, {128, 128, 128, 255}};
JDAW_Color lightgrey = {{180, 180, 180, 255}, {180, 180, 180, 255}};
JDAW_Color lightergrey = {{240, 240, 240, 255}, {240, 240, 240, 255}};
JDAW_Color lightblue = {{101, 204, 255, 255}, {101, 204, 255, 255}};
JDAW_Color black = {{0, 0, 0, 255},{0, 0, 0, 255}};
JDAW_Color menu_bckgrnd = {{25, 25, 25, 230}, {25, 25, 25, 230}};
JDAW_Color bckgrnd_color = {{255, 240, 200, 255}, {22, 28, 34, 255}};
JDAW_Color track_bckgrnd = {{120, 130, 150, 255}, {120, 130, 150, 255}};
JDAW_Color track_bckgrnd_active = {{170, 180, 200, 255}, {170, 180, 200, 255}};
JDAW_Color txt_soft = {{50, 50, 50, 255}, {200, 200, 200, 255}};
JDAW_Color txt_main = {{10, 10, 10, 255}, {240, 240, 240, 255}};
JDAW_Color tl_bckgrnd = {{240, 235, 235, 255}, {50, 52, 55, 255}};
JDAW_Color play_head = {{0, 0, 0, 255}, {255, 255, 255, 255}};
JDAW_Color console_bckgrnd = {{140, 140, 140, 255}, {140, 140, 140, 255}};
JDAW_Color console_bckgrnd_active = {{200, 200, 200, 255}, {200, 200, 200, 255}};
JDAW_Color clear = {{0, 0, 0, 0}, {0, 0, 0, 0}};


JDAW_Color clip_bckgrnd = {{90, 180, 245, 255}, {90, 180, 245, 255}};
JDAW_Color clip_bckgrnd_grabbed = {{111, 214, 255, 255}, {111, 214, 255, 255}};

JDAW_Color default_textbox_text_color = {{0, 0, 0, 255}, {0, 0, 0, 255}};
JDAW_Color default_textbox_border_color = {{0, 0, 0, 255}, {0, 0, 0, 255}};
JDAW_Color default_textbox_fill_color = {{240, 240, 240, 255}, {240, 240, 240, 255}};

JDAW_Color menulist_bckgrnd = (JDAW_Color) {{40, 40, 40, 248},{40, 40, 40, 248}};
JDAW_Color menulist_inner_brdr = (JDAW_Color) {{130, 130, 130, 250},{130, 130, 130, 250}};
JDAW_Color menulist_outer_brdr = {{10, 10, 10, 220},{10, 10, 10, 220}};
JDAW_Color menulist_item_hvr = {{12, 107, 249, 220}, {12, 107, 249, 220}};

JDAW_Color fslider_border = {{12, 107, 249, 250}, {12, 107, 249, 250}};
JDAW_Color fslider_bar = {{12, 107, 249, 250}, {12, 107, 249, 250}};
JDAW_Color fslider_bckgrnd = {{40, 40, 40, 248},{40, 40, 40, 248}};

JDAW_Color marked_bckgrnd = {{90, 180, 245, 80}, {90, 180, 245, 80}};

JDAW_Color muted_bckgrnd = {{255, 0, 0, 100}, {255, 0, 0, 100}};
JDAW_Color unmuted_bckgrnd = {{200, 200, 200, 100}, {200, 200, 200, 100}};
JDAW_Color solo_bckgrnd = {{255, 200, 0, 130}, {255, 200, 0, 130}};

/* Draw a circle quadrant. Quad 0 = upper right, 1 = upper left, 2 = lower left, 3 = lower right */
void draw_quadrant(SDL_Renderer *rend, int xinit, int yinit, int r, const register uint8_t quad)
{
    int x = 0;
    int y = r;
    int d = 1-r;
    while (x <= y) {
        switch(quad) {
            case 0:
                SDL_RenderDrawPoint(rend, xinit + x, yinit - y);
                SDL_RenderDrawPoint(rend, xinit + y, yinit  - x);
                break;
            case 1:
                SDL_RenderDrawPoint(rend, xinit - x, yinit - y);
                SDL_RenderDrawPoint(rend, xinit - y, yinit - x);
                break;
            case 2:
                SDL_RenderDrawPoint(rend, xinit - x, yinit + y);
                SDL_RenderDrawPoint(rend, xinit - y, yinit + x);
                break;
            case 3:
                SDL_RenderDrawPoint(rend, xinit + x, yinit + y);
                SDL_RenderDrawPoint(rend, xinit + y, yinit + x);
                break;
        }
        if (d>0) {
            /* Select SE coordinate */
            d += (x<<1) - (y<<1) + 5;
            y--;
        } else {
            d += (x<<1) + 3;
        }
        x++;
    }
}

/* Draw and fill a quadrant. quad 0 = upper right, 1 = upper left, 2 = lower left, 3 = lower right*/
void fill_quadrant(SDL_Renderer *rend, int xinit, int yinit, int r, const register uint8_t quad)
{
    int x = 0;
    int y = r;
    int d = 1-r;
    int fill_x = 0;

    while (x <= y) {
        if (d>0) {
            switch(quad) {
                case 0:
                    SDL_RenderDrawLine(rend, xinit, yinit - y, xinit + x, yinit - y);
                    SDL_RenderDrawLine(rend, xinit + y, yinit, xinit + y, yinit  - x);
                    break;
                case 1:
                    SDL_RenderDrawLine(rend, xinit, yinit - y, xinit - x, yinit - y);
                    SDL_RenderDrawLine(rend, xinit - y, yinit, xinit - y, yinit - x);
                    break;
                case 2:
                    SDL_RenderDrawLine(rend, xinit, yinit + y, xinit - x, yinit + y);
                    SDL_RenderDrawLine(rend, xinit - y, yinit, xinit - y, yinit + x);
                    break;
                case 3:
                    SDL_RenderDrawLine(rend, xinit, yinit + y, xinit + x, yinit + y);
                    SDL_RenderDrawLine(rend, xinit + y, yinit, xinit + y, yinit + x);
                    break;
            }
            /* Select SE coordinate */
            d += (x<<1) - (y<<1) + 5;
            y--;
        } else {
            d += (x<<1) + 3;
        }
        x++;
    }
    fill_x = x;

    SDL_Rect fill = {xinit, yinit, fill_x, fill_x};
    switch (quad) {
        case 0:
            fill.y -= fill_x -1;
            break;
        case 1:
            fill.x -= fill_x - 1;
            fill.y -= fill_x - 1;
            break;
        case 2:
            fill.x -= fill_x - 1;
            break;       
        case 3:
            break;
        default:
            break;
    }
    SDL_RenderFillRect(rend, &fill);
}

void draw_rounded_rect(SDL_Renderer *rend, SDL_Rect *rect, int r)
{
    int left_x = rect->x + r;
    int right_x = rect->x + rect->w -r;
    int upper_y = rect->y + r;
    int lower_y = rect->y + rect->h - r;
    SDL_Point ul = {left_x, upper_y};
    SDL_Point ur = {right_x, upper_y};
    SDL_Point ll = {left_x, lower_y};
    SDL_Point lr = {right_x, lower_y};
    draw_quadrant(rend, ur.x, ur.y, r, 0);
    draw_quadrant(rend, ul.x, ul.y, r, 1);
    draw_quadrant(rend, ll.x, ll.y, r, 2);
    draw_quadrant(rend, lr.x, lr.y, r, 3);
    /* top */
    SDL_RenderDrawLine(rend, left_x, rect->y, right_x, rect->y);
    /* bottom */
    SDL_RenderDrawLine(rend, left_x, lower_y + r, right_x, lower_y + r);
    /* left */
    SDL_RenderDrawLine(rend, rect->x, upper_y, rect->x, lower_y);
    /* right */
    SDL_RenderDrawLine(rend, right_x + r, upper_y, right_x + r, lower_y);
    // int d = r<<1;
    // SDL_Rect top = {left_x, rect->y, rect->w - d, r};
    // SDL_Rect bottom = {left_x, lower_y, rect->w - d, r + 1};
    // SDL_Rect left = {rect->x, upper_y, r, rect->h - d};
    // SDL_Rect right = {right_x, upper_y, r + 1, rect->h -d};
    // SDL_Rect middle = {left_x, upper_y, rect->w - d, rect->h - d};
    // SDL_RenderFillRect(rend, &top);
    // SDL_RenderFillRect(rend, &bottom);
    // SDL_RenderFillRect(rend, &right);
    // SDL_RenderFillRect(rend, &left);
    // SDL_RenderFillRect(rend, &middle);    
}

void fill_rounded_rect(SDL_Renderer *rend, SDL_Rect *rect, int r)
{
    int left_x = rect->x + r;
    int right_x = rect->x + rect->w - r;
    int upper_y = rect->y + r;
    int lower_y = rect->y + rect->h - r;
    SDL_Point ul = {left_x, upper_y};
    SDL_Point ur = {right_x, upper_y};
    SDL_Point ll = {left_x, lower_y};
    SDL_Point lr = {right_x, lower_y};
    fill_quadrant(rend, ur.x, ur.y, r, 0);
    fill_quadrant(rend, ul.x, ul.y, r, 1);
    fill_quadrant(rend, ll.x, ll.y, r, 2);
    fill_quadrant(rend, lr.x, lr.y, r, 3);
    int d = r<<1; // decision criterion
    SDL_Rect top = {left_x + 1, rect->y, rect->w - d - 1, r};
    SDL_Rect bottom = {left_x + 1, lower_y, rect->w - d - 1, r + 1};
    SDL_Rect left = {rect->x, upper_y + 1, r, rect->h - d - 1};
    SDL_Rect right = {right_x, upper_y + 1, r + 1, rect->h - d - 1};
    SDL_Rect middle = {left_x, upper_y, rect->w - d, rect->h - d};
    SDL_RenderFillRect(rend, &top);
    SDL_RenderFillRect(rend, &bottom);
    SDL_RenderFillRect(rend, &right);
    SDL_RenderFillRect(rend, &left);
    SDL_RenderFillRect(rend, &middle);
}

/* Set the render color based on project display mode */
void set_rend_color(SDL_Renderer *rend, JDAW_Color *color_class) 
{
    SDL_Color color;
    if (dark_mode) {
        color = color_class->dark;
    } else {
        color = color_class->light;
    }
    SDL_SetRenderDrawColor(rend, color.r, color.g, color.b, color.a);
}


/* Draw an empty circle */
void draw_circle(SDL_Renderer *rend, int xinit, int yinit, int r)
{
    int x = 0;
    int y = r;
    int d = 1-r;
    while (x <= y) {
        SDL_RenderDrawPoint(rend, x + xinit, y + yinit);
        SDL_RenderDrawPoint(rend, y + xinit, x + yinit);
        SDL_RenderDrawPoint(rend, (x * -1) + xinit, y + yinit);
        SDL_RenderDrawPoint(rend, (x * -1) + xinit, (y * -1) + yinit);
        SDL_RenderDrawPoint(rend, x + xinit, (y * -1) + yinit);
        SDL_RenderDrawPoint(rend, (y * -1) + xinit, x + yinit);
        SDL_RenderDrawPoint(rend, (y * -1) + xinit, (x * -1) + yinit);
        SDL_RenderDrawPoint(rend, y + xinit, (x * -1) + yinit);
        if (d>0) {
            /* Select SE coordinate */
            d += (x<<1) - (y<<1) + 5;
            y--;
        } else {
            /* Select E coordinate */
            d += (x<<1) + 3;
        }
        x++;
    }
}

void draw_textbox(SDL_Renderer *rend, Textbox *tb)
{
    set_rend_color(rend, tb->bckgrnd_color);
    if (tb->radius == 0) {
        SDL_RenderFillRect(rend, &(tb->container));
        set_rend_color(rend, tb->border_color);
        SDL_RenderDrawRect(rend, &(tb->container));
    } else {
        fill_rounded_rect(rend, &(tb->container), tb->radius);
        set_rend_color(rend, tb->border_color);
        draw_rounded_rect(rend, &(tb->container), tb->radius);
        set_rend_color(rend, tb->border_color);
        draw_rounded_rect(rend, &(tb->container), tb->radius);
    }
    if (tb->available) {
        write_text(rend, &(tb->txt_container), tb->font, tb->txt_color, tb->display_value, true);
    } else {
        write_text(rend, &(tb->txt_container), tb->font, &grey, tb->display_value, true);
    }

    if (tb->show_cursor) {
        if (tb->cursor_countdown == 0) {
            tb->cursor_countdown = CURSOR_COUNTDOWN;
        } else if (tb->cursor_countdown > CURSOR_COUNTDOWN / 2) {
            char newstr[255];
            strncpy(newstr, tb->display_value, tb->cursor_pos);
            newstr[tb->cursor_pos] = '\0';
            int w;
            TTF_SizeUTF8(tb->font, newstr, &w, NULL);
            set_rend_color(rend, &bckgrnd_color);
            int x = tb->txt_container.x + w;
            for (int i=0; i<CURSOR_WIDTH; i++) {
                SDL_RenderDrawLine(rend, x, tb->txt_container.y, x, tb->txt_container.y + tb->txt_container.h);
                x++;
            }
        }
        tb->cursor_countdown -= 1;
    }
}

void draw_menu_item(SDL_Renderer *rend, Textbox *tb)
{
    // set_rend_color(rend, tb->bckgrnd_color);
    // if (tb->radius == 0) {
    //     SDL_RenderFillRect(rend, &(tb->container));
    //     set_rend_color(rend, tb->border_color);
    //     SDL_RenderDrawRect(rend, &(tb->container));
    // } else {
    //     fill_rounded_rect(rend, &(tb->container), tb->radius);
    //     //TODO: rounded rect border
    // }
    if (tb->mouse_hover) {
        set_rend_color(rend, &menulist_item_hvr);
        fill_rounded_rect(rend, &tb->container, MENU_LIST_R);
    }
    if (tb->available) {
        write_text(rend, &(tb->txt_container), tb->font, &white, tb->value, true);
    } else {
        write_text(rend, &(tb->txt_container), tb->font, &grey, tb->value, true);
    }

    if (tb->show_cursor) {
        if (tb->cursor_countdown == 0) {
            tb->cursor_countdown = CURSOR_COUNTDOWN;
        } else if (tb->cursor_countdown > CURSOR_COUNTDOWN / 2) {
            char newstr[255];
            strncpy(newstr, tb->value, tb->cursor_pos);
            newstr[tb->cursor_pos] = '\0';
            int w;
            TTF_SizeUTF8(tb->font, newstr, &w, NULL);
            set_rend_color(rend, &bckgrnd_color);
            int x = tb->txt_container.x + w;
            for (int i=0; i<CURSOR_WIDTH; i++) {
                SDL_RenderDrawLine(rend, x, tb->txt_container.y, x, tb->txt_container.y + tb->txt_container.h);
                x++;
            }
        }
        tb->cursor_countdown -= 1;
    }
}
static void draw_menu_list(SDL_Renderer *rend, TextboxList *tbl)
{
    // int padding = 3 * scale_factor;
    SDL_Rect innerrect = (SDL_Rect) {tbl->container.x + 1, tbl->container.y + 1, tbl->container.w - 2, tbl->container.h - 2};
    set_rend_color(rend, &menulist_bckgrnd);
    fill_rounded_rect(rend, &(tbl->container), MENU_LIST_R);
    set_rend_color(rend, &menulist_outer_brdr);
    draw_rounded_rect(rend, &(tbl->container), MENU_LIST_R);
    set_rend_color(rend, &menulist_inner_brdr);
    draw_rounded_rect(rend, &innerrect, MENU_LIST_R - 1);
    for (uint8_t i=0; i<tbl->num_textboxes; i++) {
        draw_menu_item(rend, tbl->textboxes[i]);
    }
}

static void draw_textbox_list(SDL_Renderer *rend, TextboxList *tbl)
{
    // fprintf(stderr, "container: %d, %d, %d, %d\n", tbl->container.x, tbl->container.y, tbl->container.w, tbl->container.h);
    set_rend_color(rend, tbl->bckgrnd_color);
    if (tbl->radius == 0) {
        SDL_RenderFillRect(rend, &(tbl->container));
        set_rend_color(rend, tbl->border_color);
        SDL_RenderDrawRect(rend, &(tbl->container));
        // fprintf(stderr, "Container I'm drawing: %d, %d, %d, %d", tbl->container.x, tbl->container.y, tbl->container.w, tbl->container.h);
    } else {
        fill_rounded_rect(rend, &(tbl->container), tbl->radius);
        //TODO: Empty rounded rect border
    }
    for (uint8_t i=0; i<tbl->num_textboxes; i++) {
        draw_textbox(rend, tbl->textboxes[i]);
    }
}



static void draw_fslider(JDAWWindow *jwin, FSlider *fslider)
{
    set_rend_color(jwin->rend, &fslider_border);
    SDL_RenderDrawRect(jwin->rend, &(fslider->rect));
    set_rend_color(jwin->rend, &fslider_bckgrnd);
    SDL_RenderFillRect(jwin->rend, &(fslider->rect));
    set_rend_color(jwin->rend, &fslider_bar);
    SDL_RenderFillRect(jwin->rend, &(fslider->bar_rect));
}

void draw_hamburger(Project *proj)
{
    set_rend_color(proj->jwin->rend, &txt_soft);
    SDL_GL_GetDrawableSize(proj->jwin->win, &(proj->jwin->w), &(proj->jwin->h));
    // TODO: fix relative recting
    // SDL_Rect hmbrgr_1 = relative_rect(&(proj->winrect), 0.95, 0.04, 0.02, 0.004);
    // SDL_Rect hmbrgr_2 = relative_rect(&(proj->winrect), 0.95, 0.048, 0.02, 0.004);
    // SDL_Rect hmbrgr_3 = relative_rect(&(proj->winrect), 0.95, 0.056, 0.02, 0.004);
    // SDL_RenderDrawRect(proj->jwin->rend, &hmbrgr_1);
    // SDL_RenderFillRect(proj->jwin->rend, &hmbrgr_1);
    // SDL_RenderFillRect(proj->jwin->rend, &hmbrgr_2);
    // SDL_RenderFillRect(proj->jwin->rend, &hmbrgr_3);
}


void draw_waveform(Clip *clip)
{
    if (clip->channels == 1) {
        int wav_x = clip->rect.x;
        int wav_y = clip->rect.y + clip->rect.h / 2;
        SDL_SetRenderDrawColor(proj->jwin->rend, 5, 5, 60, 255);
        int16_t sample = 0;
        int last_sample_y = wav_y;
        int sample_y = wav_y;
        for (int i=0; i<clip->len_sframes-1; i+=clip->track->tl->sample_frames_per_pixel) {
            if (wav_x > proj->tl->audio_rect.x) {
                if (wav_x >= proj->tl->audio_rect.x + proj->tl->audio_rect.w) {
                    break;
                }
                sample = (clip->post_proc)[i];
                sample_y = wav_y + sample * clip->rect.h / (2 * INT16_MAX);
                // SDL_RenderDrawLine(proj->jwin->rend, wav_x, wav_y, wav_x, sample_y);
                SDL_RenderDrawLine(proj->jwin->rend, wav_x, last_sample_y, wav_x + 1, sample_y);
                last_sample_y = sample_y;
            }
            wav_x++;
        }
    } else if (clip->channels == 2) {
        int wav_x = clip->rect.x;
        int wav_y_l = clip->rect.y + clip->rect.h / 4;
        int wav_y_r = clip->rect.y + 3 * clip->rect.h / 4;
        set_rend_color(proj->jwin->rend, &black);
        // int clip_mid_y = clip->rect.y + clip->rect.h / 2;
        // SDL_RenderDrawLine(proj->jwin->rend, clip->rect.x, clip_mid_y, clip->rect.x + clip->rect.w, clip_mid_y);
        // clip_mid_y -= 1;
        // SDL_RenderDrawLine(proj->jwin->rend, clip->rect.x, clip_mid_y, clip->rect.x + clip->rect.w, clip_mid_y);
        // clip_mid_y += 2;
        // SDL_RenderDrawLine(proj->jwin->rend, clip->rect.x, clip_mid_y, clip->rect.x + clip->rect.w, clip_mid_y);
        SDL_SetRenderDrawColor(proj->jwin->rend, 5, 5, 60, 255);
        int16_t sample_l = 0;
        int16_t sample_r = 0;
        int last_sample_y_l = wav_y_l;
        int last_sample_y_r = wav_y_r;
        int sample_y_l = wav_y_l;
        int sample_y_r = wav_y_r;

        int i=0;
        while (i<clip->len_sframes * clip->channels) {
        // for (int i=0; i<clip->len_sframes * clip->channels; i+=clip->track->tl->sample_frames_per_pixel * clip->channels) {
            if (wav_x > proj->tl->audio_rect.x && wav_x < proj->tl->audio_rect.x + proj->tl->audio_rect.w) {
                sample_l = (clip->post_proc)[i];
                sample_r = (clip->post_proc[i+1]);
                int j=0;
                // while (j<proj->tl->sample_frames_per_pixel) {
                //     if (abs((clip->post_proc)[i]) > abs(sample_l) && (clip->post_proc)[i] / abs((clip->post_proc)[i]) == sample_l < 0 ? -1 : 1) {
                //         sample_l = (clip->post_proc)[i];
                //     }
                //     if (abs((clip->post_proc)[i+1]) > abs(sample_l)) {
                //         sample_r = (clip->post_proc)[i+1];
                //     }
                //     i+=2;
                //     j++;
                // }
                sample_y_l = wav_y_l + sample_l * clip->rect.h / (4 * INT16_MAX);
                sample_y_r = wav_y_r + sample_r * clip->rect.h / (4 * INT16_MAX);
                // SDL_RenderDrawLine(proj->jwin->rend, wav_x, wav_y_l, wav_x, sample_y_l);
                // SDL_RenderDrawLine(proj->jwin->rend, wav_x, wav_y_r, wav_x, sample_y_r);
                SDL_RenderDrawLine(proj->jwin->rend, wav_x, last_sample_y_l, wav_x + 1, sample_y_l);
                SDL_RenderDrawLine(proj->jwin->rend, wav_x, last_sample_y_r, wav_x + 1, sample_y_r);

                last_sample_y_l = sample_y_l;
                last_sample_y_r = sample_y_r;
                i+= proj->tl->sample_frames_per_pixel * 2;
                
            } else {
                i += proj->tl->sample_frames_per_pixel * 2;
            }
            wav_x++;
        }
    }
}

void draw_clip_ramps(Clip *clip)
{
    int start_w = get_tl_draw_w(clip->start_ramp_len);
    int end_w = get_tl_draw_w(clip->end_ramp_len);
    set_rend_color(proj->jwin->rend, &white);
    /* draw start ramp */
    SDL_RenderDrawLine(proj->jwin->rend, clip->rect.x, clip->rect.y + clip->rect.h, clip->rect.x + start_w, clip->rect.y);
    /* draw end ramp */
    SDL_RenderDrawLine(proj->jwin->rend, clip->rect.x + clip->rect.w - end_w, clip->rect.y, clip->rect.x + clip->rect.w, clip->rect.y + clip->rect.h);
}

void draw_clip(Clip *clip)
{
    if (clip->grabbed) {
        set_rend_color(proj->jwin->rend, &clip_bckgrnd_grabbed);
    } else {
        set_rend_color(proj->jwin->rend, &clip_bckgrnd);
    }
    SDL_RenderFillRect(proj->jwin->rend, &(clip->rect));
    if (clip->done) {
        draw_clip_ramps(clip);
        draw_waveform(clip);
    }
    // SDL_Rect clipnamerect = get_rect(clip->rect, CLIP_NAME_RECT); //TODO: remove this computation from draw
    draw_textbox(proj->jwin->rend, clip->namebox);
    // write_text(proj->jwin->rend, &clipnamerect, proj->jwin->bold_fonts[1], &grey, clip->name, true);
    set_rend_color(proj->jwin->rend, &black);
    // fprintf(stderr, "\t->drawing border\n");
    SDL_Rect cliprect_temp = clip->rect;
    for (int i=0; i<CLIP_BORDER_W; i++) {
        SDL_RenderDrawRect(proj->jwin->rend, &cliprect_temp);
        cliprect_temp.x += 1;
        cliprect_temp.y += 1;
        cliprect_temp.w -= 2;
        cliprect_temp.h -= 2;
    }
    set_rend_color(proj->jwin->rend, &white);
    for (int i=0; i<CLIP_BORDER_W; i++) {
        SDL_RenderDrawRect(proj->jwin->rend, &cliprect_temp);
        cliprect_temp.x += 1;
        cliprect_temp.y += 1;
        cliprect_temp.w -= 2;
        cliprect_temp.h -= 2;
    }
    cliprect_temp = clip->rect;
    cliprect_temp.x -= 2;
    cliprect_temp.y -= 2;
    cliprect_temp.w += 4;
    cliprect_temp.h += 4;
    // fprintf(stderr, "\t->if grabbed\n");

    if (clip->grabbed) {
        for (int i=0; i<CLIP_BORDER_W / 2; i++) {
            SDL_RenderDrawRect(proj->jwin->rend, &cliprect_temp);
            cliprect_temp.x -= 1;
            cliprect_temp.y -= 1;
            cliprect_temp.w += 2;
            cliprect_temp.h += 2;        
        }
    }
}

static void draw_track(Track * track) 
{    
    if (track->active) {
        set_rend_color(proj->jwin->rend, &track_bckgrnd_active);
    } else {
        set_rend_color(proj->jwin->rend, &track_bckgrnd);
    }
    SDL_RenderFillRect(proj->jwin->rend, &track->rect);

    Clip* clip;
    for (int j=0; j<track->num_clips; j++) {
        if ((clip = (*(track->clips + j)))) {
            draw_clip(clip);
        }
    }

    if (track->active) {
        set_rend_color(proj->jwin->rend, &console_bckgrnd_active);
    } else {
        set_rend_color(proj->jwin->rend, &console_bckgrnd);
    }
    SDL_RenderFillRect(proj->jwin->rend, &(track->console_rect));

    draw_textbox(proj->jwin->rend, track->name_box);
    draw_textbox(proj->jwin->rend, track->input_label_box);
    draw_textbox(proj->jwin->rend, track->input_name_box);
    draw_textbox(proj->jwin->rend, track->vol_label_box);
    draw_textbox(proj->jwin->rend, track->pan_label_box);
    draw_textbox(proj->jwin->rend, track->mute_button_box);
    draw_textbox(proj->jwin->rend, track->solo_button_box);
    draw_fslider(proj->jwin, track->vol_ctrl);
    draw_fslider(proj->jwin, track->pan_ctrl);

    SDL_Rect colorbar = (SDL_Rect) {track->rect.x + track->console_rect.w, track->rect.y, COLOR_BAR_W, track->rect.h};
    set_rend_color(proj->jwin->rend, track->color);
    SDL_RenderFillRect(proj->jwin->rend, &colorbar);
}

void draw_jwin_background(JDAWWindow *jwin)
{
    set_rend_color(jwin->rend, &bckgrnd_color);
    SDL_RenderClear(jwin->rend);
}

void draw_jwin_menus(JDAWWindow *jwin)
{
    for (uint8_t i=0; i<jwin->num_active_menus; i++) {
        draw_menu_list(jwin->rend, jwin->active_menus[i]);
    }
}

void draw_tc()
{
    draw_textbox(proj->jwin->rend, proj->tl->timecode_tb);

}

void draw_ruler()
{
    if (!proj) {
        fprintf(stderr, "Error: request to draw ruler with no active project.\n");
        exit(1);
    }
    set_rend_color(proj->jwin->rend, &black);
    SDL_RenderFillRect(proj->jwin->rend, &(proj->tl->ruler_rect));
    set_rend_color(proj->jwin->rend, &white);

    int x = first_second_tick_x();
    int sw = get_second_w();

    while (x < proj->tl->rect.x + proj->tl->rect.w) {
        if (x > proj->tl->audio_rect.x) {
            SDL_RenderDrawLine(proj->jwin->rend, x, proj->tl->rect.y, x, proj->tl->rect.y + 20);
        }
        x += sw;
        if (x > proj->tl->audio_rect.x + proj->tl->audio_rect.w) {
            break;
        }
    }    
}

void draw_project(Project *proj)
{

    const static char *bottom_text = "Jackdaw | by Charlie Volow";
    set_rend_color(proj->jwin->rend, &bckgrnd_color);
    SDL_RenderClear(proj->jwin->rend);


    draw_hamburger(proj);

    /* Draw the timeline */
    set_rend_color(proj->jwin->rend, &tl_bckgrnd);
    SDL_RenderFillRect(proj->jwin->rend, &(proj->tl->rect));

    Track *track;

    for (int i=0; i < proj->tl->num_tracks; i++) {
        if ((track = (*(proj->tl->tracks + i)))) {
            /* Only draw if track is onscreen */
            if (track->rect.y + track->rect.h > proj->tl->rect.y && track->rect.y < proj->tl->rect.y + proj->tl->rect.h) {
                draw_track(track);
            }
            
        }
    }

    set_rend_color(proj->jwin->rend, &tl_bckgrnd);
    SDL_RenderFillRect(proj->jwin->rend, &(proj->tl->ruler_tc_container_rect));
    draw_ruler();
    draw_tc();
    set_rend_color(proj->jwin->rend, &bckgrnd_color);
    SDL_Rect top_mask = {0, 0, proj->jwin->w, proj->tl->rect.y};
    SDL_RenderFillRect(proj->jwin->rend, &top_mask);
    SDL_Rect mask_left = {0, 0, proj->tl->rect.x, proj->jwin->h};
    SDL_RenderFillRect(proj->jwin->rend, &mask_left);
    SDL_Rect mask_left_2 = {proj->tl->rect.x, proj->tl->rect.y, PADDING, proj->tl->rect.h};
    set_rend_color(proj->jwin->rend, &tl_bckgrnd);
    SDL_RenderFillRect(proj->jwin->rend, &mask_left_2);
    // fprintf(stderr, "\t->end draw\n");

    // SDL_SetRenderDrawColor(proj->jwin->rend, 255, 0, 0, 255);
    // SDL_RenderDrawRect(proj->jwin->rend, &(proj->ctrl_rect));
    // SDL_SetRenderDrawColor(proj->jwin->rend, 0, 255, 0, 255);
    // SDL_RenderDrawRect(proj->jwin->rend, &(proj->ctrl_rect_col_a));
    draw_textbox(proj->jwin->rend, proj->audio_out_label);
    draw_textbox(proj->jwin->rend, proj->audio_out);

    set_rend_color(proj->jwin->rend, &white);

    /* Draw t=0 */
    if (proj->tl->offset < 0) {
        set_rend_color(proj->jwin->rend, &black);
        int zero_x = get_tl_draw_x(0);
        SDL_RenderDrawLine(proj->jwin->rend, zero_x, proj->tl->audio_rect.y, zero_x, proj->tl->audio_rect.y + proj->tl->audio_rect.h);
    }

    /* Draw play head line */
    set_rend_color(proj->jwin->rend, &white);
    int tri_y = proj->tl->rect.y;
    if (proj->tl->play_position >= proj->tl->offset) {
        int play_head_x = get_tl_draw_x(proj->tl->play_position);
        SDL_RenderDrawLine(proj->jwin->rend, play_head_x, proj->tl->rect.y, play_head_x, proj->tl->rect.y + proj->tl->rect.h);

        /* Draw play head triangle */
        int tri_x1 = play_head_x;
        int tri_x2 = play_head_x;
        int tri_y = proj->tl->rect.y;
        for (int i=0; i<PLAYHEAD_TRI_H; i++) {
            SDL_RenderDrawLine(proj->jwin->rend, tri_x1, tri_y, tri_x2, tri_y);
            tri_y -= 1;
            tri_x2 += 1;
            tri_x1 -= 1;
        }
    }


    /* draw mark in */
    int in_x, out_x = -1;
    if (proj->tl->in_mark >= proj->tl->offset && proj->tl->in_mark < proj->tl->offset + get_tl_abs_w(proj->tl->audio_rect.w)) {
        in_x = get_tl_draw_x(proj->tl->in_mark);
        int i_tri_x2 = in_x;
        tri_y = proj->tl->rect.y;
        for (int i=0; i<PLAYHEAD_TRI_H; i++) {
            SDL_RenderDrawLine(proj->jwin->rend, in_x, tri_y, i_tri_x2, tri_y);
            tri_y -= 1;
            i_tri_x2 += 1;    
        }            
    } else if (proj->tl->in_mark < proj->tl->offset) {
        in_x = proj->tl->audio_rect.x;
    }

    /* draw mark out */
    if (proj->tl->out_mark > proj->tl->offset && proj->tl->out_mark < proj->tl->offset + get_tl_abs_w(proj->tl->audio_rect.w)) {
        out_x = get_tl_draw_x(proj->tl->out_mark);
        int o_tri_x1 = out_x;
        tri_y = proj->tl->rect.y;
        for (int i=0; i<PLAYHEAD_TRI_H; i++) {
            SDL_RenderDrawLine(proj->jwin->rend, o_tri_x1, tri_y, out_x, tri_y);
            tri_y -= 1;
            o_tri_x1 -= 1;
        }
    } else if (proj->tl->out_mark > proj->tl->offset + get_tl_abs_w(proj->tl->audio_rect.w)) {
        out_x = proj->tl->audio_rect.x + proj->tl->audio_rect.w;
    }
    if (in_x < out_x && out_x != 0) {
        SDL_Rect in_out = (SDL_Rect) {in_x, proj->tl->audio_rect.y, out_x - in_x, proj->tl->audio_rect.h};
        set_rend_color(proj->jwin->rend, &marked_bckgrnd);
        SDL_RenderFillRect(proj->jwin->rend, &(in_out));
    }

    /* Draw title */
    int title_w = 0;
    int title_h = 0;
    set_rend_color(proj->jwin->rend, &bckgrnd_color);
    TTF_SizeUTF8(proj->jwin->fonts[1], bottom_text, &title_w, &title_h);
    SDL_Rect title_rect = {0, proj->jwin->h - 24 * scale_factor, proj->jwin->w, 24 * scale_factor};
    SDL_RenderFillRect(proj->jwin->rend, &title_rect);
    SDL_Rect title_text_rect = {proj->jwin->w / 2 - (title_w / 2), title_rect.y, title_w, title_h};
    write_text(proj->jwin->rend, &title_text_rect, proj->jwin->fonts[1], &txt_soft, bottom_text, true);
}