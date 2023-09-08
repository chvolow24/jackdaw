#include <float.h>
#include <math.h>
#include "SDL2/SDL.h"
#include "SDL2/SDL_render.h"
#include "theme.h"
#include "project.h"
#include "text.h"
#include "theme.h"

#define DEFAULT_TL_WIDTH_SAMPLES 220500


extern TTF_Font *open_sans_12;
extern TTF_Font *open_sans_var_12;
extern TTF_Font *courier_new_12;
extern JDAW_Color bckgrnd_color;
extern JDAW_Color txt_soft;
extern JDAW_Color tl_bckgrnd;
extern JDAW_Color track_bckgrnd;
extern JDAW_Color play_head;

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
        // SDL_Rect bar = {xinit, yinit - y,  ,1}
        // SDL_RenderFillRect(rend, &bar);

        // SDL_Rect fill = {xinit, yinit, xinit + r, yinit +_r}

        // SDL_RenderDrawPoint(rend, x + xinit, y + yinit);
        // SDL_RenderDrawPoint(rend, y + xinit, x + yinit);
        // SDL_RenderDrawPoint(rend, x + xinit, (y * -1) + yinit);
        // SDL_RenderDrawPoint(rend, y + xinit, (x * -1) + yinit);
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

void draw_hamburger(Project * proj)
{
    set_rend_color(proj, &txt_soft);
    SDL_Rect winbox;
    SDL_GetWindowSize(proj->win, &(winbox.w), &(winbox.h));
    SDL_Rect hmbrgr_1 = relative_rect(&winbox, 0.95, 0.04, 0.02, 0.004);
    SDL_Rect hmbrgr_2 = relative_rect(&winbox, 0.95, 0.048, 0.02, 0.004);
    SDL_Rect hmbrgr_3 = relative_rect(&winbox, 0.95, 0.056, 0.02, 0.004);
    SDL_RenderDrawRect(proj->rend, &hmbrgr_1);
    SDL_RenderFillRect(proj->rend, &hmbrgr_1);
    SDL_RenderFillRect(proj->rend, &hmbrgr_2);
    SDL_RenderFillRect(proj->rend, &hmbrgr_3);
}

void draw_track(Track *track)
{

}

void draw_timeline(Timeline *tl)
{
    Track* track;
    for (int i=0; i < tl->num_tracks; i++) {
        if ((track = (*(tl->tracks + i)))) {
            draw_track(track);
        }
    }
}

void draw_project(Project *proj)
{
    const static char *bottom_text = "Jackdaw | by Charlie Volow";
    SDL_Rect winbox;
    SDL_GetWindowSize(proj->win, &(winbox.w), &(winbox.h));
    set_rend_color(proj, &bckgrnd_color);
    SDL_RenderClear(proj->rend);
    draw_hamburger(proj);

    /* Draw the timeline */
    set_rend_color(proj, &tl_bckgrnd);
    SDL_Rect tl_box = relative_rect(&winbox, 0.05, 0.1, 0.9, 0.86);
    draw_rounded_rect(proj->rend, &tl_box, 5);


    Track *track;
    Clip *clip;
    int track_x = tl_box.x + 5;
    int track_y = tl_box.y + 5;
    int track_w = tl_box.w - 10;
    int track_spacing = 5;
    set_rend_color(proj, &track_bckgrnd);
    for (int i=0; i < proj->tl->num_tracks; i++) {
        if ((track = (*(proj->tl->tracks + i)))) {
            SDL_Rect trackbox = {track_x, track_y, track_w, track->height};
            draw_rounded_rect(proj->rend, &trackbox, 5);
            for (int j=0; j<track->num_clips; j++) {
                if ((clip = (*(track->clips + j)))) {
                    SDL_Rect clipbox = {
                        track_x + track_w * clip->absolute_position / DEFAULT_TL_WIDTH_SAMPLES, 
                        track_y + 4, 
                        track_w * clip->length / DEFAULT_TL_WIDTH_SAMPLES,
                        trackbox.h - 8
                    };
                    SDL_SetRenderDrawColor(proj->rend, 0, 255, 0, 255);
                    int wav_x = clipbox.x;
                    int wav_y = clipbox.y + clipbox.h / 2;

                    if (clip->done) {
                        int sample = (int)((clip->samples)[0]);
                        int next_sample;
                        for (int i=0; i<clip->length-1; i++) {
                            next_sample = (int)((clip->samples)[i+1]);
                            if (next_sample != 0) {
                                fprintf(stderr, "next sample: %d;", next_sample);
                                fprintf(stderr, " y offset: %d\n", next_sample * clipbox.h / 100);

                            }
                            SDL_RenderDrawLine(proj->rend, wav_x, wav_y + (sample * clipbox.h / 100), wav_x + 1, wav_y + (next_sample * (clipbox.h / 32767)));
                            sample = next_sample;
                            wav_x++;
                            // SDL_RenderDrawLine
                        }
                    }

                    SDL_RenderDrawRect(proj->rend, &clipbox);
                    // SDL_RenderFillRect(proj->rend, &clipbox);
                    set_rend_color(proj, &track_bckgrnd);

                }
            }
            track_y += track->height + track_spacing;

        }
    }
    set_rend_color(proj, &play_head);
    int play_head_x = (tl_box.w * proj->tl->play_position / DEFAULT_TL_WIDTH_SAMPLES) + tl_box.x;
    SDL_RenderDrawLine(proj->rend, play_head_x, tl_box.y, play_head_x, tl_box.y + tl_box.h);


    int title_w = 0;
    int title_h = 0;
    TTF_SizeText(open_sans_12, bottom_text, &title_w, &title_h);
    SDL_Rect titlebox = relative_rect(&winbox, 0.5, 1, 0.5, 0.1);
    titlebox.y -= title_h + 2;
    titlebox.x -= title_w / 2 + 5;
    write_text(proj->rend, &titlebox, open_sans_12, &txt_soft, bottom_text, true);
    SDL_RenderPresent(proj->rend);

}
