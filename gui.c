#include <float.h>
#include <math.h>
#include "SDL.h"
#include "theme.h"
#include "project.h"
#include "text.h"
#include "theme.h"
#include "audio.h"
#include "gui.h"

#define PLAYHEAD_TRI_H 20
#define TRACK_SPACING 10
#define PADDING (4 * proj->scale_factor)
#define MAX_SFPP 8000

#define TRACK_CONSOLE_WIDTH 280

#define TRACK_CONSOLE (Dim) {ABS, 0}, (Dim) {ABS, 0}, (Dim) {ABS, TRACK_CONSOLE_WIDTH}, (Dim) {REL, 100}
#define TRACK_NAME_BOX (Dim) {REL, 1}, (Dim) {REL, 1}, (Dim) {REL, 75}, (Dim) {ABS, 32}
#define CLIP_NAME_RECT (Dim) {ABS, 10}, (Dim) {REL, 3}, (Dim) {ABS, 100}, (Dim) {ABS, 20}
// #define TITLE_RECT (Dim) {ABS, 0}, (Dim) {ABS, 0}, (Dim) {REL, 100}, (Dim) {ABS, 20}
// #define TITLE_TXT_RECT (Dim) {REL, 40}, (Dim) {ABS, 0}, (Dim) {REL, 60}, (Dim) {ABS, 100}

#define lesser_of(a,b) (a < b ? a : b)
#define greater_of(a,b) (a > b ? a : b)



JDAW_Color red = {{255, 0, 0, 255},{255, 0, 0, 255}};
JDAW_Color green = {{0, 255, 0, 255},{0, 255, 0, 255}};
JDAW_Color blue = {{0, 0, 255, 255},{0, 0, 255, 255}};
JDAW_Color white = {{255, 255, 255, 255},{255, 255, 255, 255}};
JDAW_Color grey = {{128, 128, 128, 255}, {128, 128, 128, 255}};
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

uint32_t get_abs_tl_x(int draw_x)
{
    return (draw_x - proj->tl->audio_rect.x) * proj->tl->sample_frames_per_pixel + proj->tl->offset; 
}

int get_tl_draw_x(uint32_t abs_x) 
{
    if (proj->tl->sample_frames_per_pixel != 0) {
        return proj->tl->audio_rect.x + ((int) abs_x - (int) proj->tl->offset) / (int) proj->tl->sample_frames_per_pixel;
    } else {
        fprintf(stderr, "Error: proj tl sfpp value 0\n");
    }
}

int get_tl_draw_w(uint32_t abs_w) 
{
    if (proj->tl->sample_frames_per_pixel != 0) {
        return abs_w / proj->tl->sample_frames_per_pixel;
    } else {
        fprintf(stderr, "Error: proj tl sfpp value 0\n");
        return 0;
    }
}

int32_t get_tl_abs_w(int draw_w) 
{
    return draw_w * proj->tl->sample_frames_per_pixel;
}

void draw_track(Track* track, int *track_y) {
    int track_x = track->tl->rect.x + PADDING;
    int track_w = track->tl->rect.w;
    // int samples_per_pixel = DEFAULT_TL_WIDTH / track_w;
    SDL_Rect trackbox = {track_x, *track_y, track_w, track->rect.h};
    set_rend_color(proj, &lightgrey);
    SDL_RenderFillRect(proj->rend, &trackbox);

    Clip* clip;
    for (int j=0; j<track->num_clips; j++) {
        if ((clip = (*(track->clips + j)))) {
            int clip_x = get_tl_draw_x(clip->absolute_position);
            int clip_w = get_tl_draw_w(clip->length);
            SDL_Rect cliprect = {
                clip_x,
                *track_y + 4, 
                clip_w,
                track->tl->audio_rect.h - 4
            };
            int wav_x = cliprect.x;
            int wav_y = cliprect.y + cliprect.h / 2;
            set_rend_color(proj, &lightblue);
            SDL_RenderFillRect(proj->rend, &cliprect);
            SDL_Rect clipnamerect = get_rect(cliprect, CLIP_NAME_RECT);
            write_text(proj->rend, &clipnamerect, proj->fonts[1], &grey, clip->name, true);
            set_rend_color(proj, &black);
            for (int i=0; i<2; i++) {
                SDL_RenderDrawRect(proj->rend, &cliprect);
                cliprect.x += 1;
                cliprect.y += 1;
                cliprect.w -= 2;
                cliprect.h -= 2;
            }
            set_rend_color(proj, &white);
            for (int i=0; i<4; i++) {
                SDL_RenderDrawRect(proj->rend, &cliprect);
                cliprect.x += 1;
                cliprect.y += 1;
                cliprect.w -= 2;
                cliprect.h -= 2;
            }

            SDL_SetRenderDrawColor(proj->rend, 5, 5, 60, 255);
            if (clip->done) {
                int16_t sample = (int)((clip->samples)[0]);
                int16_t next_sample;
                for (int i=0; i<clip->length-1; i+=track->tl->sample_frames_per_pixel) {
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
    SDL_Rect consolerect = get_rect(trackbox, TRACK_CONSOLE);
    set_rend_color(proj, &grey);
    SDL_RenderFillRect(proj->rend, &consolerect);
    SDL_Rect namerect = get_rect(consolerect, TRACK_NAME_BOX);
    set_rend_color(proj, &txt_soft);
    SDL_RenderFillRect(proj->rend, &namerect);
    set_rend_color(proj, &black);
    SDL_RenderDrawRect(proj->rend, &namerect);
    namerect.x += PADDING;
    write_text(proj->rend, &namerect, proj->fonts[1], &black, track->name, true);
    // draw_rounded_rect(proj->rend, &namerect, 14);
    SDL_Rect colorbar = (SDL_Rect) {track_x + consolerect.w, *track_y, 10, track->rect.h};
    track->tl->audio_rect = (SDL_Rect) {colorbar.x + colorbar.w, colorbar.y, trackbox.w - consolerect.w, trackbox.h};
    set_rend_color(proj, track->color);
    SDL_RenderFillRect(proj->rend, &colorbar);
    *track_y += track->rect.h + TRACK_SPACING;
}

void translate_tl(int translate_by_w)
{
    int32_t new_offset = proj->tl->offset + get_tl_abs_w(translate_by_w);
    if (new_offset < 0) {
        proj->tl->offset = 0;
    } else {
        proj->tl->offset = new_offset;
    }
}

void rescale_timeline(double scale_factor, uint32_t center_abs_pos) 
{
    if (scale_factor == 0) {
        fprintf(stderr, "Warning! Scale factor 0 in rescale_timeline\n");
        return;
    }
    int init_draw_pos = get_tl_draw_x(center_abs_pos);
    double new_sfpp = proj->tl->sample_frames_per_pixel / scale_factor;

    if (new_sfpp < 2 || new_sfpp > MAX_SFPP) {
        return;
    }
    proj->tl->sample_frames_per_pixel = new_sfpp;

    int new_draw_pos = get_tl_draw_x(center_abs_pos);
    int offset_draw_delta = new_draw_pos - init_draw_pos;
    if (offset_draw_delta < (-1 * get_tl_draw_w(proj->tl->offset))) {
        proj->tl->offset = 0;
    } else {
        proj->tl->offset += (get_tl_abs_w(offset_draw_delta));
    }
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
    // proj->tl->rect = relative_rect(&(proj->winrect), 0.05, 0.1, 0.9, 0.86);
    SDL_RenderFillRect(proj->rend, &(proj->tl->rect));
    // draw_rounded_rect(proj->rend, &(proj->tl->rect), STD_RAD);


    Track *track;
    int track_y = proj->tl->rect.y + PADDING;

    for (int i=0; i < proj->tl->num_tracks; i++) {
        if ((track = (*(proj->tl->tracks + i)))) {
            draw_track(track, &track_y);
        }
    }
    set_rend_color(proj, &white);

    /* Draw play head line */
    int tri_y = proj->tl->rect.y;
    if (proj->tl->play_position >= proj->tl->offset) {
        int play_head_x = get_tl_draw_x(proj->tl->play_position);
        SDL_RenderDrawLine(proj->rend, play_head_x, proj->tl->rect.y, play_head_x, proj->tl->rect.y + proj->tl->rect.h);

        /* Draw play head triangle */
        int tri_x1 = play_head_x;
        int tri_x2 = play_head_x;
        int tri_y = proj->tl->rect.y;
        for (int i=0; i<PLAYHEAD_TRI_H; i++) {
            SDL_RenderDrawLine(proj->rend, tri_x1, tri_y, tri_x2, tri_y);
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
            SDL_RenderDrawLine(proj->rend, in_x, tri_y, i_tri_x2, tri_y);
            tri_y -= 1;
            i_tri_x2 += 1;    
        }            
    }

    /* draw mark out */
    if (proj->tl->out_mark >= proj->tl->offset) {
        int out_x = get_tl_draw_x(proj->tl->out_mark);
        int o_tri_x1 = out_x;
        tri_y = proj->tl->rect.y;
        for (int i=0; i<PLAYHEAD_TRI_H; i++) {
            SDL_RenderDrawLine(proj->rend, o_tri_x1, tri_y, out_x, tri_y);
            tri_y -= 1;
            o_tri_x1 -= 1;
        }
    }


    int title_w = 0;
    int title_h = 0;
    TTF_SizeText(proj->fonts[1], bottom_text, &title_w, &title_h);
    SDL_Rect title_rect = {0, proj->winrect.h - 20 * proj->scale_factor, proj->winrect.w, 20 * proj->scale_factor};
    set_rend_color(proj, &bckgrnd_color);
    SDL_RenderFillRect(proj->rend, &title_rect);
    SDL_Rect title_text_rect = {(proj->winrect.w - title_w) / 2, title_rect.y, title_w, title_h};
    // titlebox.y -= title_h + 2 + 10;
    // titlebox.x -= title_w / 2 + 10;
    write_text(proj->rend, &title_text_rect, proj->fonts[1], &txt_soft, bottom_text, true);

    SDL_Rect mask_left = {0, 0, proj->tl->rect.x, proj->winrect.h};
    SDL_RenderFillRect(proj->rend, &mask_left);
    SDL_Rect mask_left_2 = {proj->tl->rect.x, proj->tl->rect.y, PADDING, proj->tl->rect.h};
    set_rend_color(proj, &tl_bckgrnd);
    SDL_RenderFillRect(proj->rend, &mask_left_2);


    // draw_device_list(proj->playback_devices, proj->num_playback_devices, 10, 500, 5);
    // draw_device_list(proj->record_devices, proj->num_record_devices, 500, 500, 5);
    SDL_RenderPresent(proj->rend);
}
