#include <float.h>
#include <math.h>
#include "SDL.h"
#include "theme.h"
#include "project.h"
#include "text.h"
#include "theme.h"
#include "audio.h"
#include "gui.h"

#define DEFAULT_TL_WIDTH_SAMPLES 220500
#define TRACK_SPACING 10

#define TRACK_CONSOLE (Dim) {ABS, 0}, (Dim) {ABS, 0}, (Dim) {ABS, 280}, (Dim) {REL, 100}


JDAW_Color red = {{255, 0, 0, 255},{255, 0, 0, 255}};
JDAW_Color green = {{0, 255, 0, 255},{0, 255, 0, 255}};
JDAW_Color blue = {{0, 0, 255, 255},{0, 0, 255, 255}};
JDAW_Color white = {{255, 255, 255, 255},{255, 255, 255, 255}};
JDAW_Color lightgrey = {{180, 180, 180, 255}, {180, 180, 180, 255}};
JDAW_Color lightblue = {{101, 204, 255, 255}, {101, 204, 255, 255}};
JDAW_Color black = {{0, 0, 0, 255},{0, 0, 0, 255}};
JDAW_Color menu_bckgrnd = {{25, 25, 25, 230}, {25, 25, 25, 230}};
JDAW_Color bckgrnd_color = {{255, 240, 200, 255}, {22, 28, 34, 255}};
JDAW_Color txt_soft = {{50, 50, 50, 255}, {200, 200, 200, 255}};
JDAW_Color txt_main = {{10, 10, 10, 255}, {240, 240, 240, 255}};
JDAW_Color tl_bckgrnd = {{240, 235, 235, 255}, {50, 52, 55, 255}};
JDAW_Color play_head = {{0, 0, 0, 255}, {255, 255, 255, 255}};


//TODO: Replace project arguments with reference to global var;
extern Project *proj;
uint8_t scale_factor;

/* Get SDL_Rect with size and position relative to parent window */
SDL_Rect relative_rect(SDL_Rect *win_rect, float x_rel, float y_rel, float w_rel, float h_rel)
{
    SDL_Rect ret;
    ret.x = x_rel * win_rect->w;
    ret.y = y_rel * win_rect->h;
    ret.w = w_rel * win_rect->w;
    ret.h = h_rel * win_rect->h;
    return ret;
}

SDL_Rect get_rect(SDL_Rect parent_rect, Dim x, Dim y, Dim w, Dim h) {
    SDL_Rect ret;
    switch (x.dimType) {
        case ABS:
            ret.x = parent_rect.x + x.value;
            break;
        case REL:
            ret.x = parent_rect.x + x.value * parent_rect.w / 100;
            break;
    }
    switch (y.dimType) {
        case ABS:
            ret.y = parent_rect.y + y.value;
            break;
        case REL:
            ret.y = parent_rect.y + y.value * parent_rect.h / 100;
            break;
    }
    switch (w.dimType) {
        case ABS:
            ret.w = w.value;
            break;
        case REL:
            ret.w = w.value * parent_rect.w / 100;
            break;
    }
    switch (h.dimType) {
        case ABS:
            ret.h = h.value;
            break;
        case REL:
            ret.h = h.value * parent_rect.h / 100;
            break;
    }
    return ret;
}

/* Draw and fill a quadrant. quad 0 = upper right, 1 = upper left, 2 = lower left, 3 = lower right*/
void draw_quadrant(SDL_Renderer *rend, int xinit, int yinit, int r, const register uint8_t quad)
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
void set_rend_color(Project *proj, JDAW_Color* color_class) 
{
    SDL_Color color;
    if (proj->dark_mode) {
        color = color_class->dark;
    } else {
        color = color_class->light;
    }
    SDL_SetRenderDrawColor(proj->rend, color.r, color.g, color.b, color.a);
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

/* Draw the hamburger menu */
void draw_hamburger(Project * proj)
{
    set_rend_color(proj, &txt_soft);
    SDL_GL_GetDrawableSize(proj->win, &((proj->winrect).w), &((proj->winrect).h));
    SDL_Rect hmbrgr_1 = relative_rect(&(proj->winrect), 0.95, 0.04, 0.02, 0.004);
    SDL_Rect hmbrgr_2 = relative_rect(&(proj->winrect), 0.95, 0.048, 0.02, 0.004);
    SDL_Rect hmbrgr_3 = relative_rect(&(proj->winrect), 0.95, 0.056, 0.02, 0.004);
    SDL_RenderDrawRect(proj->rend, &hmbrgr_1);
    SDL_RenderFillRect(proj->rend, &hmbrgr_1);
    SDL_RenderFillRect(proj->rend, &hmbrgr_2);
    SDL_RenderFillRect(proj->rend, &hmbrgr_3);
}

/* Draw a list of audio devices */

void draw_device_list(AudioDevice **dev_list, int num_devices, int x, int y, int padding) 
{
    SDL_Rect list_box = {x, y, 0, padding<<1};
    int w;
    int h;

    for (int i=0; i<num_devices; i++) {
        TTF_SizeText(proj->fonts[2], dev_list[i]->name, &w, &h);
        if (w > list_box.w) {
            list_box.w = w + (padding<<1);
        }
        list_box.h += h + padding;
    }

    set_rend_color(proj, &menu_bckgrnd);
    SDL_RenderFillRect(proj->rend, &list_box);
    set_rend_color(proj, &lightgrey);
    SDL_RenderDrawRect(proj->rend, &list_box);
    int spacer = padding<<1;
    for (int i=0; i<num_devices; i++) {
        SDL_Rect item_box = {list_box.x + (padding<<1), list_box.y + spacer, list_box.w, h};
        // set_rend_color(proj, &red);
        // SDL_RenderDrawRect(proj->rend, &item_box);
        spacer += h + padding;
        if (dev_list[i]-> open) {
            write_text(proj->rend, &item_box, proj->fonts[2], &white, dev_list[i]->name, true);
        } else {
            write_text(proj->rend, &item_box, proj->fonts[2], &red, dev_list[i]->name, true);
        }
    }

}

void draw_track(Track* track, int *track_y) {
    int track_x = track->tl->rect.x + 5;
    int track_w = track->tl->rect.w;
    int samples_per_pixel = DEFAULT_TL_WIDTH_SAMPLES / track_w;
    SDL_Rect trackbox = {track_x, *track_y, track_w, track->rect.h};
    set_rend_color(proj, &lightgrey);
    SDL_RenderFillRect(proj->rend, &trackbox);
    SDL_Rect console = get_rect(trackbox, TRACK_CONSOLE);
    set_rend_color(proj, &bckgrnd_color);
    SDL_RenderFillRect(proj->rend, &console);
    SDL_Rect colorbar = (SDL_Rect) {track_x + console.w, *track_y, 10, track->rect.h};
    set_rend_color(proj, track->color);
    SDL_RenderFillRect(proj->rend, &colorbar);
    Clip* clip;
    for (int j=0; j<track->num_clips; j++) {
        if ((clip = (*(track->clips + j)))) {
            SDL_Rect clipbox = {
                track_x + track_w * clip->absolute_position / DEFAULT_TL_WIDTH_SAMPLES, 
                *track_y + 4, 
                track_w * clip->length / DEFAULT_TL_WIDTH_SAMPLES,
                trackbox.h - 8
            };
            int wav_x = clipbox.x;
            int wav_y = clipbox.y + clipbox.h / 2;
            set_rend_color(proj, &lightblue);
            SDL_RenderFillRect(proj->rend, &clipbox);
            set_rend_color(proj, &black);
            for (int i=0; i<2; i++) {
                SDL_RenderDrawRect(proj->rend, &clipbox);
                clipbox.x += 1;
                clipbox.y += 1;
                clipbox.w -= 2;
                clipbox.h -= 2;
            }
            set_rend_color(proj, &white);
            for (int i=0; i<4; i++) {
                SDL_RenderDrawRect(proj->rend, &clipbox);
                clipbox.x += 1;
                clipbox.y += 1;
                clipbox.w -= 2;
                clipbox.h -= 2;
            }

            SDL_SetRenderDrawColor(proj->rend, 5, 5, 60, 255);
            if (clip->done) {
                int16_t sample = (int)((clip->samples)[0]);
                int16_t next_sample;
                for (int i=0; i<clip->length-1; i+= samples_per_pixel) {
                    next_sample = (clip->samples)[i];
                    SDL_RenderDrawLine(proj->rend, wav_x, wav_y + (sample / 50), wav_x + 1, wav_y + (next_sample / 50));
                    sample = next_sample;
                    wav_x++;

                    // SDL_RenderDrawLine
                }
            }
            // SDL_RenderFillRect(proj->rend, &clipbox);
            // set_rend_color(proj, &trck_bckgrnd);

        }
    }
    *track_y += track->rect.h + TRACK_SPACING;
}

void draw_project(Project *proj)
{
    SDL_SetRenderDrawBlendMode(proj->rend, SDL_BLENDMODE_BLEND);
    const static char *bottom_text = "Jackdaw | by Charlie Volow";
    SDL_GL_GetDrawableSize(proj->win, &((proj->winrect).w), &((proj->winrect).h));
    set_rend_color(proj, &bckgrnd_color);
    SDL_RenderClear(proj->rend);
    draw_hamburger(proj);

    /* Draw the timeline */
    set_rend_color(proj, &tl_bckgrnd);
    // SDL_Rect proj->tl->rect = relative_rect(&(proj->winrect), 0.05, 0.1, 0.9, 0.86);
    SDL_RenderFillRect(proj->rend, &(proj->tl->rect));
    // draw_rounded_rect(proj->rend, &(proj->tl->rect), STD_RAD);


    Track *track;
    Clip *clip;
    int track_x = proj->tl->rect.x + 5;
    int track_y = proj->tl->rect.y + 5;
    int track_w = proj->tl->rect.w - 10;

    for (int i=0; i < proj->tl->num_tracks; i++) {
        if ((track = (*(proj->tl->tracks + i)))) {
            draw_track(track, &track_y);
        }
    }
    set_rend_color(proj, &white);
    int play_head_x = (proj->tl->rect.w * proj->tl->play_position / DEFAULT_TL_WIDTH_SAMPLES) + proj->tl->rect.x;
    SDL_RenderDrawLine(proj->rend, play_head_x, proj->tl->rect.y, play_head_x, proj->tl->rect.y + proj->tl->rect.h);


    int title_w = 0;
    int title_h = 0;
    TTF_SizeText(proj->fonts[1], bottom_text, &title_w, &title_h);
    SDL_Rect titlebox = relative_rect(&(proj->winrect), 0.5, 1, 0.5, 0.1);
    titlebox.y -= title_h + 2 + 10;
    titlebox.x -= title_w / 2 + 10;
    write_text(proj->rend, &titlebox, proj->fonts[1], &txt_soft, bottom_text, true);

    // draw_device_list(proj->playback_devices, proj->num_playback_devices, 10, 500, 5);
    // draw_device_list(proj->record_devices, proj->num_record_devices, 500, 500, 5);

    SDL_RenderPresent(proj->rend);

}
