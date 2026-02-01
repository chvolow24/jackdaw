#include "timeview.h"

int32_t timeview_get_pos_sframes(TimeView *tv, int draw_x)
{
    /* Timeline *tl = ACTIVE_TL; */
    return (draw_x - tv->rect->x) * tv->sample_frames_per_pixel + tv->offset_left_sframes;
}

double timeview_get_draw_x_precise(TimeView *tv, int32_t pos_sframes)
{
    return (double)tv->rect->x + ((double)pos_sframes - (double)tv->offset_left_sframes) / (double)tv->sample_frames_per_pixel;
}
int timeview_get_draw_x(TimeView *tv, int32_t pos_sframes)
{
    return (int) round(timeview_get_draw_x_precise(tv, pos_sframes));
}

double timeview_get_draw_w_precise(TimeView *tv, int32_t w_sframes)
{
    return (double) w_sframes / tv->sample_frames_per_pixel;
}

int timeview_get_draw_w(TimeView *tv, int32_t w_sframes)
{
    return (int) round(timeview_get_draw_w_precise(tv, w_sframes));
}

double timeview_get_w_sframes_precise(TimeView *tv, int draw_w_pix)
{
    return (double)draw_w_pix * tv->sample_frames_per_pixel;
}

int32_t timeview_get_w_sframes(TimeView *tv, int draw_w_pix)
{
    return (int32_t)round(timeview_get_w_sframes_precise(tv, draw_w_pix));
}
int32_t timeview_rightmost_pos(TimeView *tv)
{
    return (int32_t)((double)tv->offset_left_sframes + timeview_get_w_sframes_precise(tv, tv->rect->w));
}

static void timeview_rectify_scroll(TimeView *tv)
{
    if (tv->restrict_view) {
	int64_t overshoot_right = 0;
	/* Like crossing the street */
	if (tv->offset_left_sframes < tv->view_min) {
	    tv->offset_left_sframes = tv->view_min;
	}
	if ((overshoot_right = (int64_t)timeview_rightmost_pos(tv) - tv->view_max) > 0) {
	    tv->offset_left_sframes -= overshoot_right;
	}
	if (tv->offset_left_sframes < tv->view_min) {
	    tv->offset_left_sframes = tv->view_min;
	    tv->sample_frames_per_pixel = tv->max_sfpp;
	}
    }
}

void timeview_scroll_sframes(TimeView *tv, int32_t scroll_by)
{
    tv->offset_left_sframes += scroll_by;
    timeview_rectify_scroll(tv);
}

void timeview_scroll_horiz(TimeView *tv, int scroll_by)
{
    /* Timeline *tl = ACTIVE_TL; */
    int32_t new_offset = tv->offset_left_sframes + timeview_get_w_sframes(tv, scroll_by);
    tv->offset_left_sframes = new_offset;
    timeview_rectify_scroll(tv);
    /* if (tv->restrict_view) { */
    /* 	int32_t overshoot_right = 0; */
    /* 	if (tv->offset_left_sframes < tv->view_min) { */
    /* 	    tv->offset_left_sframes = tv->view_min; */
    /* 	} */
    /* 	if ((overshoot_right = timeview_rightmost_pos(tv) - tv->view_max) > 0) { */
    /* 	    tv->offset_left_sframes -= overshoot_right; */
    /* 	} */
    /* 	fprintf(stderr, "Rightmost: %d, view max: %d\n", timeview_rightmost_pos(tv), tv->view_max); */
    /* 	fprintf(stderr, "\t\tovershoot right?? %d\n", overshoot_right); */
    /* } */


    /* fprintf(stderr, "SCROLL HORIZ new offset: %d\n", new_offset); */
}



void timeview_rescale(TimeView *tv, double sfpp_scale_factor, bool on_mouse, SDL_Point mousep)
{
    if (sfpp_scale_factor == 1.0f) return;
    int32_t center_abs_pos = 0;
    if (on_mouse) {
	center_abs_pos = timeview_get_pos_sframes(tv, mousep.x);
    } else if (tv->play_pos) {
	center_abs_pos = *(tv->play_pos);
    }
    if (sfpp_scale_factor == 0) {
        fprintf(stderr, "Warning! Scale factor 0 in rescale_timeview\n");
        return;
    }
    int init_draw_pos = timeview_get_draw_x(tv, center_abs_pos);
    double new_sfpp = tv->sample_frames_per_pixel / sfpp_scale_factor;
    if (new_sfpp < 1.0) {
        return;
    } else if (tv->restrict_view && new_sfpp > tv->max_sfpp) {
	tv->sample_frames_per_pixel = tv->max_sfpp;
	timeview_rectify_scroll(tv);
	return;
	/* tv->offset_left_sframes = 0; */
	/* return; */
    }
    if (new_sfpp == tv->sample_frames_per_pixel) {
        tv->sample_frames_per_pixel += sfpp_scale_factor <= 1.0f ? 1 : -1;
	if (tv->sample_frames_per_pixel < 1) tv->sample_frames_per_pixel = 1;
    } else {
        tv->sample_frames_per_pixel = new_sfpp;
    }

    int new_draw_pos = timeview_get_draw_x(tv, center_abs_pos);
    int offset_draw_delta = new_draw_pos - init_draw_pos;
    tv->offset_left_sframes += (timeview_get_w_sframes(tv, offset_draw_delta));
    timeview_rectify_scroll(tv);
}

void timeview_catchup(TimeView *tv)
{
    /* Timeline *tl = ACTIVE_TL; */
    if (!tv->play_pos) return;
    /* uint32_t move_by_sframes; */
    int catchup_w = tv->rect->w / 2;
    /* while (catchup_w > session->gui.audio_rect->w / 2 && catchup_w > 10) { */
    /* 	catchup_w /= 2; */
    /* } */
    int playhead_x = timeview_get_draw_x(tv, *tv->play_pos);
    if (playhead_x > tv->rect->x + tv->rect->w) {
	tv->offset_left_sframes = *tv->play_pos - timeview_get_w_sframes(tv, tv->rect->w - catchup_w);
    }
    else if (playhead_x < tv->rect->x) {
	tv->offset_left_sframes = *tv->play_pos - timeview_get_w_sframes(tv, catchup_w);
    }
    timeview_rectify_scroll(tv);
}

void timeview_init(TimeView *tv, SDL_Rect *rect, double sfpp, int32_t offset_left, int32_t *play_pos, int32_t *in, int32_t *out)
{
    tv->rect = rect;
    tv->sample_frames_per_pixel = sfpp;
    tv->offset_left_sframes = offset_left;
    tv->play_pos = play_pos;
    tv->in_mark = in;
    tv->out_mark = out;    
}


