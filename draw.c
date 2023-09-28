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


JDAW_Color default_textbox_text_color = {{0, 0, 0, 255}, {0, 0, 0, 255}};
JDAW_Color default_textbox_border_color = {{0, 0, 0, 255}, {0, 0, 0, 255}};
JDAW_Color default_textbox_fill_color = {{240, 240, 240, 255}, {240, 240, 240, 255}};

JDAW_Color menulist_bckgrnd = (JDAW_Color) {{40, 40, 40, 248},{40, 40, 40, 248}};
JDAW_Color menulist_inner_brdr = (JDAW_Color) {{10, 10, 10, 250},{10, 10, 10, 250}};
JDAW_Color menulist_outer_brdr = {{130, 130, 130, 250},{130, 130, 130, 250}};
JDAW_Color menulist_item_hvr = {{12, 107, 249, 250}, {12, 107, 249, 250}};


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
        if (d>0) {
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
    int right_x = rect->x + rect->w -r;
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
    int d = r<<1;
    SDL_Rect top = {left_x, rect->y, rect->w - d, r};
    SDL_Rect bottom = {left_x, lower_y, rect->w - d, r + 1};
    SDL_Rect left = {rect->x, upper_y, r, rect->h - d};
    SDL_Rect right = {right_x, upper_y, r + 1, rect->h -d};
    SDL_Rect middle = {left_x, upper_y, rect->w - d, rect->h - d};
    SDL_RenderFillRect(rend, &top);
    SDL_RenderFillRect(rend, &bottom);
    SDL_RenderFillRect(rend, &right);
    SDL_RenderFillRect(rend, &left);
    SDL_RenderFillRect(rend, &middle);
    // SDL_Rect rects[] = {ul_sq, ur_sq, ll_sq, lr_sq};
    // // SDL_RenderFillRects(rend, rects, 4);
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
        //TODO: rounded rect border
    }
    write_text(rend, &(tb->txt_container), tb->font, tb->txt_color, tb->display_value, true);

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
    write_text(rend, &(tb->txt_container), tb->font, &white, tb->value, true);

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
void draw_menu_list(SDL_Renderer *rend, TextboxList *tbl)
{
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

void draw_textbox_list(SDL_Renderer *rend, TextboxList *tbl)
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

void draw_hamburger(Project * proj)
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
void draw_clip(Clip *clip)
{
    // fprintf(stderr, "start draw CLIP\n");
    // int clip_x = get_tl_draw_x(clip->absolute_position);
    // if (i<4) {
    //     fprintf(stderr, "draw clip, abs pos: %d, draw_x: %d\n", clip->absolute_position, clip_x);
    //     i++;
    // }
    // int clip_w = get_tl_draw_w(clip->length);
    // fprintf(stderr, "\t->retrieved clip dimensions\n");

    // SDL_Rect cliprect = {
    //     clip_x,
    //     clip->track->rect.y + 4, //TODO: fix these
    //     clip_w,
    //     clip->track->rect.h - 4
    // };
    // fprintf(stderr, "\t->set clip rect\n");

    int wav_x = clip->rect.x;
    int wav_y = clip->rect.y + clip->rect.h / 2;
    set_rend_color(proj->jwin->rend, &lightblue);
    SDL_RenderFillRect(proj->jwin->rend, &(clip->rect));
    SDL_Rect clipnamerect = get_rect(clip->rect, CLIP_NAME_RECT);
    write_text(proj->jwin->rend, &clipnamerect, proj->jwin->bold_fonts[1], &grey, clip->name, true);
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
        for (int i=0; i<CLIP_BORDER_W; i++) {
            SDL_RenderDrawRect(proj->jwin->rend, &cliprect_temp);
            cliprect_temp.x -= 1;
            cliprect_temp.y -= 1;
            cliprect_temp.w += 2;
            cliprect_temp.h += 2;        
        }
    }

    SDL_SetRenderDrawColor(proj->jwin->rend, 5, 5, 60, 255);
    // fprintf(stderr, "\t->draw wav\n");
    if (clip->done) {
        int16_t sample = (int)((clip->samples)[0]);
        int16_t next_sample;
        for (int i=0; i<clip->length-1; i+=clip->track->tl->sample_frames_per_pixel) {
            // fprintf(stderr, "\t\t->Clip %p, sample %d\n", clip, i);
            next_sample = (clip->samples)[i];
            SDL_RenderDrawLine(proj->jwin->rend, wav_x, wav_y + (sample / 50), wav_x + 1, wav_y + (next_sample / 50));
            sample = next_sample;
            wav_x++;
        }
    }
    // fprintf(stderr, "\t->end draw CLIP\n");

}

void draw_track(Track * track) 
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

    /* Draw the track information console */
    SDL_Rect consolerect = get_rect(track->rect, TRACK_CONSOLE);
    consolerect.w = track->tl->console_width;

    if (track->active) {
        set_rend_color(proj->jwin->rend, &console_bckgrnd_active);
    } else {
        set_rend_color(proj->jwin->rend, &console_bckgrnd);
    }
    SDL_RenderFillRect(proj->jwin->rend, &consolerect);

    position_textbox(track->name_box, consolerect.x + 4, consolerect.y + 4);
    draw_textbox(proj->jwin->rend, track->name_box);
    position_textbox(track->input_label_box, consolerect.x + TRACK_INTERNAL_PADDING, track->name_box->container.y + track->name_box->container.h + TRACK_INTERNAL_PADDING); //TODO: replace 4 with padding
    draw_textbox(proj->jwin->rend, track->input_label_box);
    position_textbox(track->input_name_box, track->input_label_box->container.x + track->input_label_box->container.w + TRACK_INTERNAL_PADDING, track->input_label_box->container.y);
    draw_textbox(proj->jwin->rend, track->input_name_box);
    // TODO: fix this 
    SDL_Rect colorbar = (SDL_Rect) {track->rect.x + consolerect.w, track->rect.y, COLOR_BAR_W, track->rect.h};
    // track->tl->audio_rect = (SDL_Rect) {colorbar.x + colorbar.w, colorbar.y, track->rect.w - consolerect.w, track->rect.h};
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

void draw_project(Project *proj)
{

    // fprintf(stderr, "start draw\n");
    // SDL_SetRenderDrawBlendMode(proj->jwin->rend, SDL_BLENDMODE_BLEND);
    const static char *bottom_text = "Jackdaw | by Charlie Volow";
    // SDL_GL_GetDrawableSize(proj->jwin->win, &(proj->jwin->w), &(proj->jwin->h));
    set_rend_color(proj->jwin->rend, &bckgrnd_color);
    SDL_RenderClear(proj->jwin->rend);
    draw_hamburger(proj);
    /* Draw the timeline */
    set_rend_color(proj->jwin->rend, &tl_bckgrnd);
    // proj->tl->rect = relative_rect(&(proj->winrect), 0.05, 0.1, 0.9, 0.86);
    SDL_RenderFillRect(proj->jwin->rend, &(proj->tl->rect));
    // draw_rounded_rect(proj->jwin->rend, &(proj->tl->rect), STD_RAD);


    Track *track;

    for (int i=0; i < proj->tl->num_tracks; i++) {
        if ((track = (*(proj->tl->tracks + i)))) {
            /* Only draw if track is onscreen */
            if (track->rect.y + track->rect.h > proj->tl->rect.y && track->rect.y < proj->tl->rect.y + proj->tl->rect.h) {
                draw_track(track);
            }
            
        }
    }
    set_rend_color(proj->jwin->rend, &white);

    /* Draw play head line */
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
    if (proj->tl->in_mark >= proj->tl->offset) {
        int in_x = get_tl_draw_x(proj->tl->in_mark);
        int i_tri_x2 = in_x;
        tri_y = proj->tl->rect.y;
        for (int i=0; i<PLAYHEAD_TRI_H; i++) {
            SDL_RenderDrawLine(proj->jwin->rend, in_x, tri_y, i_tri_x2, tri_y);
            tri_y -= 1;
            i_tri_x2 += 1;    
        }            
    }

    /* draw mark out */
    if (proj->tl->out_mark > proj->tl->offset) {
        int out_x = get_tl_draw_x(proj->tl->out_mark);
        int o_tri_x1 = out_x;
        tri_y = proj->tl->rect.y;
        for (int i=0; i<PLAYHEAD_TRI_H; i++) {
            SDL_RenderDrawLine(proj->jwin->rend, o_tri_x1, tri_y, out_x, tri_y);
            tri_y -= 1;
            o_tri_x1 -= 1;
        }
    }

    int title_w = 0;
    int title_h = 0;
    TTF_SizeUTF8(proj->jwin->fonts[1], bottom_text, &title_w, &title_h);
    SDL_Rect title_rect = {0, proj->jwin->h - 20 * scale_factor, proj->jwin->w, 20 * scale_factor};
    set_rend_color(proj->jwin->rend, &bckgrnd_color);
    SDL_RenderFillRect(proj->jwin->rend, &title_rect);
    SDL_Rect title_text_rect = {(proj->jwin->w - title_w) / 2, title_rect.y, title_w, title_h};
    write_text(proj->jwin->rend, &title_text_rect, proj->jwin->fonts[1], &txt_soft, bottom_text, true);
    SDL_Rect mask_left = {0, 0, proj->tl->rect.x, proj->jwin->h};
    SDL_RenderFillRect(proj->jwin->rend, &mask_left);
    SDL_Rect mask_left_2 = {proj->tl->rect.x, proj->tl->rect.y, PADDING, proj->tl->rect.h};
    set_rend_color(proj->jwin->rend, &tl_bckgrnd);
    SDL_RenderFillRect(proj->jwin->rend, &mask_left_2);
    // fprintf(stderr, "\t->end draw\n");


}