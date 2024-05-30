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
    waveform.c

    * algorithm to draw waveforms inside rectangles
    * incl. multi-channel audio
 *****************************************************************************************************************/

#include "window.h"
#include "project.h"

#define SFPP_THRESHOLD 15
#define SFPP_SAFE 200

extern Project *proj;
extern Window *main_win;

static void waveform_draw_channel(float *channel, uint32_t buflen, int start_x, int w, int amp_h_max, int center_y)
{
    float sfpp = (double) buflen / w;
    
    if (sfpp <= 0) {
	fprintf(stderr, "Error in waveform_draw_channel: sfpp<=0\n");
	return;
    }

    if (sfpp > SFPP_THRESHOLD) {
	float avg_amp_neg, avg_amp_pos;
	int x = start_x;
	/* int amp_y = center_y; */
	float sample_i = 0.0f;
	while (x < start_x + w && sample_i + sfpp < buflen) {
	    /* Early exit conditions (offscreen) */
	    if (x < 0) {
		sample_i+=sfpp*(0-x);
		x=0;
		continue;
	    } else if (x > main_win->w) {
		break;
	    }
	    
	    /* Get avg amplitude value */
	    avg_amp_neg = 0;
	    avg_amp_pos = 0;
	    int sfpp_safe = (int)sfpp < SFPP_SAFE ? (int)sfpp : SFPP_SAFE;
	    for (int i=0; i<sfpp_safe; i++) {
		if (sample_i + i >= buflen) {
		    break;
		}
		float sample;
		if ((sample = channel[(int)sample_i + i]) < 0) {
		    avg_amp_neg += sample;
		} else {
		    avg_amp_pos += sample;
		}
	    }
	    avg_amp_neg /= sfpp_safe;
	    avg_amp_pos /= sfpp_safe;
	    /* for (int i=0; i<(int)sfpp; i++) { */
	    /* 	if (sample_i + i >= buflen) { */
	    /* 	    break; */
	    /* 	} */
	    /* 	float sample; */
	    /* 	/\* fprintf(stdout, "wav i=%d\n", i); *\/ */
	    /* 	if ((sample = channel[(int)sample_i + i]) < 0) { */
	    /* 	    avg_amp_neg += sample; */
	    /* 	} else { */
	    /* 	    avg_amp_pos += sample; */
	    /* 	} */
	    /* 	/\* avg_amp += fabs(channel[(int)sample_i + i]); *\/ */
	    /* } */
	    /* avg_amp_neg /= sfpp; */
	    /* avg_amp_pos /= sfpp; */
	    if (avg_amp_neg < -1.0f || avg_amp_pos > 1.0f) {
		SDL_SetRenderDrawColor(main_win->rend, 255, 0, 0, 255);
		SDL_RenderDrawLine(main_win->rend, x, center_y - amp_h_max, x, center_y + amp_h_max);
		SDL_SetRenderDrawColor(main_win->rend, 0, 0, 0, 255);
	    } else {
		int y_offset_pos = avg_amp_pos * amp_h_max;
		int y_offset_neg = avg_amp_neg * amp_h_max;
		SDL_RenderDrawLine(main_win->rend, x, center_y + y_offset_neg, x, center_y + y_offset_pos);
	    }
	    sample_i+=sfpp;
	    x++;
	}
    } else {
	float avg_amp = 0;
	int x = start_x;
	/* int sample_y = center_y; */
	int last_sample_y = center_y;
	float sample_i = 0.0f;
	while (x < start_x + w && sample_i + sfpp < buflen) {
	    avg_amp = 0;
	    if (x < 0) {
		sample_i+=sfpp;
		x++;
		continue;
	    } else if (x > main_win->w) {
		break;
	    }
	    if (sfpp > 1) {
		for (int i=0; i<(int)sfpp; i++) {
		    if (sample_i + i >= buflen) {
			break;
		    }
		    avg_amp += channel[(int)sample_i + i];
		    /* fprintf(stdout, "\t->avg amp + %f\n", channel[(int)(sample_i) + i]); */
		}
		avg_amp /= sfpp;
	    } else {
		avg_amp = channel[(int)sample_i];
	    }
	    int sample_y = center_y + avg_amp * amp_h_max;
	    if (fabs(avg_amp) > 1.0f) {
		SDL_SetRenderDrawColor(main_win->rend, 255, 0, 0, 255);
		SDL_RenderDrawLine(main_win->rend, x-1, center_y - amp_h_max, x-1, center_y + amp_h_max);
		SDL_RenderDrawLine(main_win->rend, x, center_y - amp_h_max, x, center_y + amp_h_max);
		SDL_SetRenderDrawColor(main_win->rend, 0, 0, 0, 255);
		last_sample_y = avg_amp > 0 ? center_y + amp_h_max : center_y - amp_h_max;
	    } else {
		/* if (channel == proj->output_L && sample_i < 10) { */
		/*     fprintf(stdout, "\t->sfpp: %f, avgamp: %f, y_off: %d\n", sfpp, avg_amp, (int)(avg_amp * amp_h_max)); */
		/* } */
		SDL_RenderDrawLine(main_win->rend, x-1, last_sample_y, x, sample_y);
		last_sample_y = sample_y;
	    }
	    sample_i+=sfpp;
	    x++;
	}
	
    }
    

}

void waveform_draw_all_channels(float **channels, uint8_t num_channels, uint32_t buflen, SDL_Rect *rect)
{
    int channel_h = rect->h / num_channels;
    int center_y = rect->y + channel_h / 2;
    for (uint8_t i=0; i<num_channels; i++) {
	/* fprintf(stdout, "DRAW ALL CHANNELS: channels? %p, %p\n", channels[0], channels[1]); */
	/* fprintf(stdout, "Drawing channel w %d, x %d, buflen %ul\n", rect->w, rect->x, buflen); */
	waveform_draw_channel(channels[i], buflen, rect->x, rect->w, channel_h / 2, center_y);
	/* if (proj && channels == &proj->output_L) { */
	/*     fprintf(stdout, "rect h? %d, maxamp h? %d\n", rect->h, channel_h / 2); */
	/* } */
	center_y += channel_h;
    }

}
