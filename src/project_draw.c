/*****************************************************************************************************************
  Jackdaw | a stripped-down, keyboard-focused Digital Audio Workstation | built on SDL (https://libsdl.org/)
******************************************************************************************************************

  Copyright (C) 2023 Charlie Volow
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software iso
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
    project_draw.c

    * draw project-related objects (e.g.timeline, track, clipref)
    * main draw function called in project_loop.c
 *****************************************************************************************************************/


#include "color.h"
#include "dsp.h"
#include "geometry.h"
#include "input.h"
#include "layout.h"
#include "modal.h"
#include "page.h"
#include "panel.h"
#include "project.h"
#include "symbol.h"
#include "textbox.h"
#include "timeline.h"
#include "waveform.h"

/* #define BACKGROUND_ACTIVE */

extern Window *main_win;
extern Project *proj;

SDL_Color track_bckgrnd = {120, 130, 150, 255};
SDL_Color source_mode_bckgrnd = {0, 20, 40, 255};
/* SDL_Color track_bckgrnd_active = {170, 130, 130, 255}; */
SDL_Color track_bckgrnd_active = {190, 190, 180, 255};
/* SDL_Color track_bckgrnd_active = {220, 210, 170, 255}; */
SDL_Color console_bckgrnd = {140, 140, 140, 255};
/* SDL_Color console_bckgrnd_selector = {210, 180, 100, 255}; */
/* SDL_Color console_bckgrnd_selector = {230, 190, 100, 255}; */
SDL_Color console_bckgrnd_selector = {210, 190, 140, 255};
SDL_Color timeline_bckgrnd =  {50, 52, 55, 255};
SDL_Color console_column_bckgrnd = {45, 50, 55, 255};
SDL_Color timeline_marked_bckgrnd = {255, 255, 255, 30};
SDL_Color ruler_bckgrnd = {10, 10, 10, 255};
/* SDL_Color control_bar_bckgrnd = {20, 20, 20, 255}; */
SDL_Color control_bar_bckgrnd = {22, 28, 34, 255};
SDL_Color track_selector_color = {100, 190, 255, 255};

SDL_Color grey_mask = {30, 30, 30, 190};

SDL_Color clip_ref_bckgrnd = {20, 200, 120, 200};
SDL_Color clip_ref_grabbed_bckgrnd = {50, 230, 150, 230};
SDL_Color clip_ref_home_bckgrnd = {90, 180, 245, 200};
SDL_Color clip_ref_home_grabbed_bckgrnd = {120, 210, 255, 230};

extern SDL_Color color_global_black;
extern SDL_Color color_global_white;
extern SDL_Color color_global_grey;
extern SDL_Color color_global_yellow;
extern SDL_Color color_global_red;

extern SDL_Color timeline_label_txt_color;


extern Symbol *SYMBOL_TABLE[];

/* static void draw_waveform(ClipRef *cr) */
/* { */
/*     SDL_SetRenderDrawColor(main_win->rend, 0, 0, 0, 255); */
/*     uint8_t num_channels = cr->clip->channels; */
/*     float *channels[num_channels]; */
/*     uint32_t cr_len_sframes = clipref_len(cr); */

/*     if (!cr->clip->L) { */
/* 	return; */
/*     } */
/*     channels[0] = cr->clip->L + cr->in_mark_sframes; */
/*     if (num_channels > 1) { */
/* 	channels[1] = cr->clip->R + cr->in_mark_sframes; */
/*     } */
/*     waveform_draw_all_channels(channels, num_channels, cr_len_sframes, &cr->layout->rect); */
/*     return; */

    
/*     /\* old code below *\/ */
/*     /\* int32_t cr_len = clipref_len(cr); *\/ */
/*     /\* if (cr->clip->channels == 1) { *\/ */
/*     /\*     int wav_x = cr->rect.x; *\/ */
/*     /\*     int wav_y = cr->rect.y + cr->rect.h / 2; *\/ */
/*     /\*     SDL_SetRenderDrawColor(main_win->rend, 5, 5, 60, 255); *\/ */
/*     /\*     float sample = 0; *\/ */
/*     /\*     int last_sample_y = wav_y; *\/ */
/*     /\*     int sample_y = wav_y; *\/ */
/*     /\*     for (int i=0; i<cr_len-1; i+=cr->track->tl->sample_frames_per_pixel) { *\/ */
/*     /\*         if (wav_x > proj->audio_rect->x) { *\/ */
/*     /\*             if (wav_x >= proj->audio_rect->x + proj->audio_rect->w) { *\/ */
/*     /\*                 break; *\/ */
/*     /\*             } *\/ */
/*     /\*             sample = cr->clip->L[i + cr->in_mark_sframes]; *\/ */
/*     /\*             sample_y = wav_y - sample * cr->rect.h / 2; *\/ */
/*     /\*             // SDL_RenderDrawLine(proj->jwin->rend, wav_x, wav_y, wav_x, sample_y); *\/ */
/*     /\*             SDL_RenderDrawLine(main_win->rend, wav_x, last_sample_y, wav_x + 1, sample_y); *\/ */
/*     /\*             last_sample_y = sample_y; *\/ */
/*     /\*         } *\/ */
/*     /\*         wav_x++; *\/ */
/*     /\*     } *\/ */
/*     /\* } else if (cr->clip->channels == 2) { *\/ */
/*     /\*     int wav_x = cr->rect.x; *\/ */
/*     /\*     int wav_y_l = cr->rect.y + cr->rect.h / 4; *\/ */
/*     /\*     int wav_y_r = cr->rect.y + 3 * cr->rect.h / 4; *\/ */
/*     /\* 	SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(color_global_black)); *\/ */
/*     /\*     SDL_SetRenderDrawColor(main_win->rend, 5, 5, 60, 255); *\/ */
/*     /\*     float sample_l = 0; *\/ */
/*     /\*     float sample_r = 0; *\/ */
/*     /\*     int last_sample_y_l = wav_y_l; *\/ */
/*     /\*     int last_sample_y_r = wav_y_r; *\/ */
/*     /\*     int sample_y_l = wav_y_l; *\/ */
/*     /\*     int sample_y_r = wav_y_r; *\/ */

/*     /\*     int i=0; *\/ */
/*     /\*     while (i<cr_len) { *\/ */
/*     /\*         if (wav_x > proj->audio_rect->x && wav_x < proj->audio_rect->x + proj->audio_rect->w) { *\/ */
/*     /\*             sample_l = cr->clip->L[i + cr->in_mark_sframes]; *\/ */
/*     /\*             sample_r = cr->clip->R[i + cr->in_mark_sframes]; *\/ */
/*     /\*             sample_y_l = wav_y_l + sample_l * cr->rect.h / 4; *\/ */
/*     /\*             sample_y_r = wav_y_r + sample_r * cr->rect.h / 4; *\/ */
/*     /\*             SDL_RenderDrawLine(main_win->rend, wav_x, last_sample_y_l, wav_x + 1, sample_y_l); *\/ */
/*     /\*             SDL_RenderDrawLine(main_win->rend, wav_x, last_sample_y_r, wav_x + 1, sample_y_r); *\/ */

/*     /\*             last_sample_y_l = sample_y_l; *\/ */
/*     /\*             last_sample_y_r = sample_y_r; *\/ */
/*     /\*             i+= cr->track->tl->sample_frames_per_pixel; *\/ */
                
/*     /\*         } else { *\/ */
/*     /\*             i += cr->track->tl->sample_frames_per_pixel; *\/ */
/*     /\*         } *\/ */
/*     /\*         wav_x++; *\/ */
/*     /\*     } *\/ */
/*     /\* } *\/ */
/* } */

static void clipref_draw_waveform(ClipRef *cr)
{
    if (cr->waveform_redraw && cr->waveform_texture) {
	SDL_DestroyTexture(cr->waveform_texture);
	cr->waveform_texture = NULL;
	cr->waveform_redraw = false;
    }
    int32_t cr_len = clipref_len(cr);
    int32_t start_pos = 0;
    int32_t end_pos = cr_len;
    double sfpp = cr->track->tl->sample_frames_per_pixel;
    SDL_Rect onscreen_rect = cr->layout->rect;
    if (onscreen_rect.x > main_win->w_pix) return;
    if (onscreen_rect.x + onscreen_rect.w < 0) return;
    if (onscreen_rect.x < 0) {
	start_pos = sfpp * -1 * onscreen_rect.x;
	if (start_pos < 0 || start_pos > clipref_len(cr)) {
	    return;
	    fprintf(stderr, "ERROR: start pos is %d\n", start_pos);
	    fprintf(stderr, "vs len: %d\n", start_pos - cr_len);
	    fprintf(stderr, "Clipref: %s\n", cr->name);
	    /* exit(1); */
	}
	onscreen_rect.w += onscreen_rect.x;
	onscreen_rect.x = 0;
    }
    if (onscreen_rect.x + onscreen_rect.w > main_win->w_pix) {
	
	if (end_pos <= start_pos || end_pos > cr_len) {
	    fprintf(stderr, "ERROR: end pos is %d\n", end_pos);
	    exit(1);
	}
	onscreen_rect.w = main_win->w_pix - onscreen_rect.x;
	end_pos = start_pos + sfpp * onscreen_rect.w;
	
    }
    if (onscreen_rect.w <= 0) return;
    if (!cr->waveform_texture) {
	SDL_Texture *saved_targ = SDL_GetRenderTarget(main_win->rend);
	/* SDL_Rect onscreen_rect = cr->layout->rect; */

	cr->waveform_texture = SDL_CreateTexture(main_win->rend, 0, SDL_TEXTUREACCESS_TARGET, onscreen_rect.w, onscreen_rect.h);
	if (!cr->waveform_texture) {
	    fprintf(stderr, "Error: unable to create waveform texture. %s\n", SDL_GetError());
	    fprintf(stderr, "Attempted to create with dims: %d, %d\n", onscreen_rect.w, onscreen_rect.h);
	    exit(1);
	}
	SDL_SetTextureBlendMode(cr->waveform_texture, SDL_BLENDMODE_BLEND);
	SDL_SetRenderTarget(main_win->rend, cr->waveform_texture);
	SDL_SetRenderDrawColor(main_win->rend, 0, 0, 0, 0);
	SDL_RenderClear(main_win->rend);
	
	/* Do waveform draw here */
	SDL_SetRenderDrawColor(main_win->rend, 0, 0, 0, 255);
	uint8_t num_channels = cr->clip->channels;
	float *channels[num_channels];
	/* uint32_t cr_len_sframes = clipref_len(cr); */
	if (!cr->clip->L) {
	    return;
	}
	channels[0] = cr->clip->L + cr->in_mark_sframes + start_pos;
	if (num_channels > 1) {
	    channels[1] = cr->clip->R + cr->in_mark_sframes + start_pos;
	}
	SDL_Rect waveform_container = {0, 0, onscreen_rect.w, onscreen_rect.h};
	waveform_draw_all_channels_generic((void **)channels, JDAW_FLOAT, num_channels, end_pos - start_pos, &waveform_container, 0, onscreen_rect.w);
	SDL_SetRenderTarget(main_win->rend, saved_targ);
    }
    SDL_RenderCopy(main_win->rend, cr->waveform_texture, NULL, &onscreen_rect);
}

static void clipref_draw(ClipRef *cr)
{
    /* clipref_reset(cr); */
    if (cr->deleted) {
	return;
    }
    if (cr->grabbed && proj->dragging) {
	clipref_reset(cr, false);
    }
    /* Only check horizontal out-of-bounds; track handles vertical */
    if (cr->layout->rect.x > main_win->w_pix || cr->layout->rect.x + cr->layout->rect.w < 0) {
	return;
    }

    if (cr->home) {
	if (cr->grabbed) {
	    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(clip_ref_home_grabbed_bckgrnd));
	} else {
	    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(clip_ref_home_bckgrnd));
	}
    } else {
	if (cr->grabbed) {
	    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(clip_ref_grabbed_bckgrnd));
	} else {
	    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(clip_ref_bckgrnd));
	}
    }
    SDL_RenderFillRect(main_win->rend, &cr->layout->rect);
    if (!cr->clip->recording) {
	clipref_draw_waveform(cr);
    }

    int border = cr->grabbed ? 3 : 2;
	
    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(color_global_black));
    geom_draw_rect_thick(main_win->rend, &cr->layout->rect, border, main_win->dpi_scale_factor);
    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(color_global_white));    
    geom_draw_rect_thick(main_win->rend, &cr->layout->rect, border / 2, main_win->dpi_scale_factor);
    if (cr->label) {
	textbox_draw(cr->label);
    }
    /* layout_draw(main_win, cr->layout); */
}

static void track_draw(Track *track)
{
    if (track->deleted) {
	return;
    }
    if (track->inner_layout->rect.y + track->inner_layout->rect.h < proj->audio_rect->y || track->inner_layout->rect.y > main_win->layout->rect.h) {
	goto automations_draw;
    }
    if (track->active) {
	SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(track_bckgrnd_active));
    } else {
	SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(track_bckgrnd));
    }
    SDL_RenderFillRect(main_win->rend, &track->inner_layout->rect);

    /* SDL_RenderSetClipRect(main_win->rend, &tl->layout->rect); */
    for (uint16_t i=0; i<track->num_clips; i++) {
	clipref_draw(track->clips[i]);
    }
    /* Left mask */
    SDL_Rect l_mask = {0, track->layout->rect.y, track->console_rect->x, track->layout->rect.h};
    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(timeline_bckgrnd));
    SDL_RenderFillRect(main_win->rend, &l_mask);
    /* SDL_RenderSetClipRect(main_win->rend, &main_win->layout->rect); */

    if (track->tl->track_selector == track->tl_rank) {
	SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(console_bckgrnd_selector));
	SDL_RenderFillRect(main_win->rend, track->console_rect);
	SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(control_bar_bckgrnd));
	SDL_RenderDrawRect(main_win->rend, track->console_rect);
    } else {
	SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(console_bckgrnd));
	SDL_RenderFillRect(main_win->rend, track->console_rect);
    }


    /* Draw the colorbar */
    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(track->color));
    SDL_RenderFillRect(main_win->rend, track->colorbar);
    
    textbox_draw(track->tb_input_label);
    textbox_draw(track->tb_vol_label);
    textbox_draw(track->tb_pan_label);
    textbox_draw(track->tb_input_name);
    textbox_draw(track->tb_mute_button);
    textbox_draw(track->tb_solo_button);
    textbox_draw(track->tb_name);

    if (track->tl->track_selector == track->tl_rank) {
	SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(color_global_black));
	geom_draw_rect_thick(main_win->rend, &track->inner_layout->rect, 3, main_win->dpi_scale_factor);
	SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(track_selector_color));
	geom_draw_rect_thick(main_win->rend, &track->inner_layout->rect, 1, main_win->dpi_scale_factor);
    }

    slider_draw(track->vol_ctrl);
    slider_draw(track->pan_ctrl);
    symbol_button_draw(track->automation_dropdown);
automations_draw:
    for (uint8_t i=0; i<track->num_automations; i++) {
	Automation *a = track->automations[i];
	if (a->shown && a->layout->rect.y + a->layout->rect.h > proj->audio_rect->y && a->layout->rect.y < proj->audio_rect->y + proj->audio_rect->h) {
	    automation_draw(a);
	    if (i == track->selected_automation) {
		SDL_Rect auto_console_bar = {
		    a->layout->rect.x + 5 * main_win->dpi_scale_factor,
		    a->layout->rect.y + 3 * main_win->dpi_scale_factor,
		    a->console_rect->x - a->layout->rect.x - 8 * main_win->dpi_scale_factor,
		    a->layout->rect.h - 6 * main_win->dpi_scale_factor
		};
		SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(console_bckgrnd_selector));
		SDL_RenderFillRect(main_win->rend, &auto_console_bar);
		SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(color_global_black));
		SDL_Rect layout_rect_large = a->layout->rect;
		layout_rect_large.y -= 3 * main_win->dpi_scale_factor;
		layout_rect_large.h += 6 * main_win->dpi_scale_factor;
		/* geom_draw_rect_thick(main_win->rend, &a->layout->rect, 3, main_win->dpi_scale_factor); */
		geom_draw_rect_thick(main_win->rend, &layout_rect_large, 3, main_win->dpi_scale_factor);
		SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(track_selector_color));
		/* geom_draw_rect_thick(main_win->rend, &a->layout->rect, 1, main_win->dpi_scale_factor); */
		geom_draw_rect_thick(main_win->rend, &layout_rect_large, 1, main_win->dpi_scale_factor);
	    }

	}
    }
}

static void ruler_draw(Timeline *tl)
{
    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(ruler_bckgrnd));
    SDL_RenderFillRect(main_win->rend, tl->proj->ruler_rect);

    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(color_global_white));

    int second;
    float x = tl->proj->ruler_rect->x + timeline_first_second_tick_x(tl, &second);
    float sw = timeline_get_second_w(tl);
    int line_len;
    while (x < proj->audio_rect->x + proj->audio_rect->w) {
    /* while (x < tl->layout->rect.x + tl->layout->rect.w) { */
        if (x > proj->audio_rect->x) {
	    if (second % 60 == 0) {
		line_len = 20 * main_win->dpi_scale_factor;
	    } else if (second % 30 == 0) {
		line_len = 15 * main_win->dpi_scale_factor;
	    } else if (second % 15 == 0) {
		line_len = 10  * main_win->dpi_scale_factor;
	    } else if (second % 5 == 0) {
		line_len = 5 * main_win->dpi_scale_factor;
	    } else {
		line_len = 5;
	    }
            SDL_RenderDrawLine(main_win->rend, x, tl->layout->rect.y, x, tl->layout->rect.y + line_len);
        }
        x += sw;
	second++;
        if (x > proj->audio_rect->x + proj->audio_rect->w) {
            break;
        }
    }    

    
}

void fill_quadrant(SDL_Renderer *rend, int xinit, int yinit, int r, const register uint8_t quad);
void fill_quadrant_complement(SDL_Renderer *rend, int xinit, int yinit, int r, const register uint8_t quad);
static int timeline_draw(Timeline *tl)
{
    /* Only redraw the timeline if necessary */
    if (!tl->needs_redraw && !proj->recording && !main_win->txt_editing && !(main_win->i_state & I_STATE_MOUSE_L)) {
	/* fprintf(stderr, "SKIP!\n"); */
	return 0;
    }
    /* fprintf(stderr, "Tl redraw? %d\n", tl->needs_redraw); */
    /* static int i=0; */
    /* fprintf(stdout, "TL draw %d\n", i); */
    /* i++; */
    /* i%=200; */
    
    /* Draw the timeline background */
    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(timeline_bckgrnd));
    SDL_RenderFillRect(main_win->rend, &tl->layout->rect);

    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(console_column_bckgrnd));
    SDL_RenderFillRect(main_win->rend, proj->console_column_rect);
    /* Draw tracks */
    SDL_RenderSetClipRect(main_win->rend, &tl->layout->rect);
    for (uint8_t i=0; i<tl->num_tracks; i++) {
	track_draw(tl->tracks[i]);
    }

    SDL_RenderSetClipRect(main_win->rend, &main_win->layout->rect);
    
    /* Draw ruler */
    ruler_draw(tl);
    if (tl->timecode_tb) {
	textbox_draw(tl->timecode_tb);
    }
    /* Draw t=0 */
    if (tl->display_offset_sframes < 0) {
	SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(color_global_black));
        int zero_x = timeline_get_draw_x(tl, 0);
        SDL_RenderDrawLine(main_win->rend, zero_x, proj->audio_rect->y, zero_x, proj->audio_rect->y + proj->audio_rect->h);
    }

    /* Draw play head line */
    /* set_rend_color(rend, &white); */
    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(color_global_white));
    int ph_y = tl->proj->ruler_rect->y + PLAYHEAD_TRI_H;
    /* int tri_y = tl->proj->ruler_rect->y; */
    if (tl->play_pos_sframes >= tl->display_offset_sframes) {
        int play_head_x = timeline_get_draw_x(tl, tl->play_pos_sframes);
        SDL_RenderDrawLine(main_win->rend, play_head_x, ph_y, play_head_x, tl->proj->audio_rect->y + tl->proj->audio_rect->h);

        /* Draw play head triangle */
        int tri_x1 = play_head_x;
        int tri_x2 = play_head_x;
        /* int ph_y = tl->rect.y; */
        for (int i=0; i<PLAYHEAD_TRI_H; i++) {
            SDL_RenderDrawLine(main_win->rend, tri_x1, ph_y, tri_x2, ph_y);
            ph_y -= 1;
            tri_x2 += 1;
            tri_x1 -= 1;
        }
    }

    /* draw mark in */
    int in_x, out_x = -1;

    if (tl->in_mark_sframes >= tl->display_offset_sframes && tl->in_mark_sframes < tl->display_offset_sframes + timeline_get_abs_w_sframes(tl, proj->audio_rect->w)) {
        in_x = timeline_get_draw_x(tl, tl->in_mark_sframes);
        int i_tri_x2 = in_x;
	ph_y = tl->proj->ruler_rect->y + PLAYHEAD_TRI_H;
        /* ph_y = tl->layout->rect.y; */
        for (int i=0; i<PLAYHEAD_TRI_H; i++) {
            SDL_RenderDrawLine(main_win->rend, in_x, ph_y, i_tri_x2, ph_y);
            ph_y -= 1;
            i_tri_x2 += 1;
        }
    } else if (tl->in_mark_sframes < tl->display_offset_sframes) {
        in_x = proj->audio_rect->x;
    } else {
	in_x = tl->layout->rect.w;
    }

    /* draw mark out */
    if (tl->out_mark_sframes > tl->display_offset_sframes && tl->out_mark_sframes < tl->display_offset_sframes + timeline_get_abs_w_sframes(tl, proj->audio_rect->w)) {
        out_x = timeline_get_draw_x(tl, tl->out_mark_sframes);
        int o_tri_x1 = out_x;
	ph_y =  tl->proj->ruler_rect->y + PLAYHEAD_TRI_H;
        for (int i=0; i<PLAYHEAD_TRI_H; i++) {
            SDL_RenderDrawLine(main_win->rend, o_tri_x1, ph_y, out_x, ph_y);
            ph_y -= 1;
            o_tri_x1 -= 1;
        }
    } else if (tl->out_mark_sframes > tl->display_offset_sframes + timeline_get_abs_w_sframes(tl, proj->audio_rect->w)) {
        out_x = proj->audio_rect->x + proj->audio_rect->w;
    }

    /* draw marked region */
    if (in_x < out_x && out_x > 0 && in_x < tl->layout->rect.w) {
        SDL_Rect in_out = (SDL_Rect) {in_x, proj->audio_rect->y, out_x - in_x, proj->audio_rect->h};
	SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(timeline_marked_bckgrnd));
        SDL_RenderFillRect(main_win->rend, &(in_out));
    }

    for (int i=0; i<tl->num_tempo_tracks; i++) {
	tempo_track_draw(tl->tempo_tracks[i]);
    }

    if (proj->source_mode) {
	SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(grey_mask));
	SDL_RenderFillRect(main_win->rend, &tl->layout->rect);
    }
    /* SDL_SetRenderDrawColor(main_win->rend, 255, 0, 0, 10); */
    /* SDL_RenderFillRect(main_win->rend, &tl->track_area->rect); */
    /* layout_draw(main_win, tl->track_area); */
    tl->needs_redraw = false;
    return 1;
    /* tl->needs_redraw = false; */

}


static void control_bar_draw(Project *proj)
{
    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(control_bar_bckgrnd));
    SDL_RenderFillRect(main_win->rend, proj->control_bar_rect);

    panel_area_draw(proj->panels);
        
    /* DRAW HAMBURGER */
    SDL_Rect mask = *proj->hamburger;
    mask.x -= 20;
    mask.w += 40;
    mask.y -= 20;
    mask.h += 40;
    SDL_Color c = control_bar_bckgrnd;
    c.a = 0;
    int rad = (10 * main_win->dpi_scale_factor);
    while (mask.w > rad) {
	if (c.a < 225) {
	    c.a += 5;
	}
	SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(c));
	geom_fill_rounded_rect(main_win->rend, &mask, rad);
	mask.x++;
	mask.y++;
	mask.w-=2;
	mask.h-=2;
    }
    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(track_bckgrnd));
    for (int i=0; i<3; i++) {
	SDL_RenderFillRect(main_win->rend, proj->bun_patty_bun[i]);
    }
}


extern SDL_Color color_global_x_red;
extern SDL_Color color_global_dropdown_green;
extern SDL_Color color_global_min_yellow;
void project_draw()
{
    window_start_draw(main_win, NULL);
    Timeline *tl = proj->timelines[proj->active_tl_index];
    int timeline_redrawn = timeline_draw(tl);
    if (timeline_redrawn) {
	control_bar_draw(proj);
	textbox_draw(proj->timeline_label);
    }
    if (main_win->active_tab_view) {
	if (timeline_redrawn) {
	    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(grey_mask));
	    SDL_RenderFillRect(main_win->rend, &proj->layout->rect);
	}
	tab_view_draw(main_win->active_tab_view);
    }

    SDL_SetRenderDrawColor(main_win->rend, sdl_color_expand(control_bar_bckgrnd));
    SDL_RenderFillRect(main_win->rend, &proj->status_bar.layout->rect);
    textbox_draw(proj->status_bar.error);
    if (proj->dragging) {
	textbox_draw(proj->status_bar.dragstat);
    }
    textbox_draw(proj->status_bar.call);
    window_draw_modals(main_win);
    window_draw_menus(main_win);

    proj->timelines[proj->active_tl_index]->needs_redraw = false;

    window_end_draw(main_win);
}


