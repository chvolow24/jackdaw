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
    main.c

    * Initialize resources
    * Handle events in the main loop. 
        Ancillary event loops are defined elsewhere for things like textbox editing (gui.c)
 *****************************************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <pwd.h>

#include "SDL.h"
#include "SDL_events.h"
#include "SDL_scancode.h"
#include "SDL_ttf.h"
#include "audio.h"
#include "theme.h"
#include "text.h"
#include "project.h"
#include "gui.h"
#include "wav.h"
#include "draw.h"

#define abs_min(a,b) (abs(a) < abs(b) ? a : b)
#define lesser_of(a,b) (a < b ? a : b)
#define greater_of(a,b) (a > b ? a : b)

#define TL_SHIFT_STEP (50 * scale_factor)
#define TL_SCROLL_STEP_H (10 * scale_factor)
#define TL_SCROLL_STEP_V (10 * scale_factor)

#define SFPP_STEP 1.2
#define CATCHUP_STEP (600 * scale_factor)

/* GLOBALS */
Project *proj = NULL;
uint8_t scale_factor = 1;
// TTF_Font *fonts[11];
// TTF_Font *bold_fonts[11];
bool dark_mode = true;
char *home_dir;

extern JDAW_Color bckgrnd_color;
extern JDAW_Color tl_bckgrnd;
extern JDAW_Color txt_soft;
extern JDAW_Color txt_main;

extern JDAW_Color lightgrey;
extern JDAW_Color black;
extern JDAW_Color white;


void set_dpi_scale_factor(JDAWWindow *jwin)
{
    int rw = 0, rh = 0, ww = 0, wh = 0;
    SDL_GetWindowSize(jwin->win, &ww, &wh);
    SDL_GetRendererOutputSize(jwin->rend, &rw, &rh);
    jwin->scale_factor = (float)rw / (float)ww;
    if (jwin->scale_factor != (float)rh / (float)wh) {
        fprintf(stderr, "Error! Scale factor w != h.\n");
    }
}

void init_graphics()
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "\nError initializing SDL: %s\n", SDL_GetError());
        exit(1);
    }

    if (TTF_Init() != 0) {
        fprintf(stderr, "\nError initializing SDL_ttf: %s", SDL_GetError());
        exit(1);
    }
}


void playback()
{
    if (proj->play_speed != 0) {
        start_playback();
        proj->playing = true;
    }
}

static void get_mouse_state(SDL_Point *mouse_p) 
{
    SDL_GetMouseState(&(mouse_p->x), &(mouse_p->y));
    mouse_p->x *= scale_factor;
    mouse_p->y *= scale_factor;
}

static void triage_mouseclick(SDL_Point *mouse_p)
{
    if (SDL_PointInRect(mouse_p, &(proj->tl->rect))) {
        if (SDL_PointInRect(mouse_p, &(proj->tl->audio_rect))) {
            proj->tl->play_position = get_abs_tl_x(mouse_p->x);
        }
        for (int i=0; i<proj->tl->num_tracks; i++) {
            Track *track;
            if ((track = proj->tl->tracks[i])) {
                if (SDL_PointInRect(mouse_p, &(track->rect))) {
                    if (SDL_PointInRect(mouse_p, &(track->name_box->container))) {
                        track->name_box->onclick((void *)track);
                    }
                }
            }
        }
    }
}

void click_item(void *tb_v)
{
    Textbox *tb = (Textbox *)tb_v;
    tb->value = "New value";
}

/* Present a window to allow user to create or open a project. Return project name */
char *new_project_loop(JDAWWindow *jwin) 
{
    fprintf(stderr, "Jwin? %p\n", jwin);

    char *strarray[3];
    strarray[0] = "Hello, there";
    strarray[1] = "Another item";
    strarray[2] = "Third item!";
    fprintf(stderr, "Got here. jwin->fonts[2] %p\n", jwin->fonts);
    TextboxList *testlist = create_textbox_list_from_strings(
        0, 
        10, 
        jwin->fonts[2], 
        strarray, 
        3, 
        &black, 
        &black, 
        &lightgrey,
        click_item, 
        NULL, 
        NULL, 
        0
    );
    position_textbox_list(testlist, 100, 50);

    bool quit = false;
    while (!quit) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT || (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_CLOSE)) {
                quit = true;
            }
        }
        set_rend_color(jwin->rend, &bckgrnd_color);
        SDL_RenderClear(jwin->rend);
        draw_textbox_list(jwin->rend, testlist);
        SDL_RenderPresent(jwin->rend);
        SDL_Delay(1);
    }
    destroy_jwin(jwin);
}


void project_loop()
{
    int active_track_i = 0;
    Track *active_track = NULL;
    bool mousebutton_down = false;
    bool cmd_ctrl_down = false;
    bool shift_down = false;
    bool k_down = false;
    bool l_down = false;
    bool j_down = false;
    bool quit = false;
    SDL_Point mouse_p;
    while (!quit) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT || (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_CLOSE)) {
                quit = true;
            } else if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                reset_dims(proj->jwin);
                reset_tl_rect(proj->tl);
            } else if (e.type == SDL_MOUSEBUTTONUP) {
                if (mousebutton_down) {
                    mousebutton_down = false;
                }
            } else if (e.type == SDL_MOUSEBUTTONDOWN) {
                mousebutton_down = true;
                get_mouse_state(&mouse_p);
                triage_mouseclick(&mouse_p);
                // get_mouse_state(&(mouse_p));
                // if (SDL_PointInRect(&mouse_p, &(proj->tl->audio_rect))) {
                //     proj->tl->play_position = get_abs_tl_x(mouse_p.x);
                // }
            } else if (e.type == SDL_MOUSEWHEEL) {
                get_mouse_state(&(mouse_p));
                if (SDL_PointInRect(&mouse_p, &(proj->tl->rect))) {
                    if (cmd_ctrl_down) {
                        double scale_factor = pow(SFPP_STEP, e.wheel.y);
                        rescale_timeline(scale_factor, get_abs_tl_x(mouse_p.x));
                    } else {

                        translate_tl(TL_SCROLL_STEP_H * e.wheel.x, TL_SCROLL_STEP_V * e.wheel.y);
                    }
                }
            } else if (e.type == SDL_KEYDOWN) {
                switch (e.key.keysym.scancode) {
                    // TODO: when text entry, use SDL_GetKeyName(event->key.keysym.sym) to get value and add to text field
                    case SDL_SCANCODE_LGUI:
                    case SDL_SCANCODE_LCTRL:
                        if (!cmd_ctrl_down) {
                            cmd_ctrl_down = true;
                        }
                        break;
                    case SDL_SCANCODE_LSHIFT:
                    case SDL_SCANCODE_RSHIFT:
                        if (!shift_down) {
                            shift_down = true;
                        }
                        break;
                    case SDL_SCANCODE_R:
                        if (!proj->recording) {
                            proj->recording = true;
                            proj->active_clip = create_clip((proj->tl->tracks)[active_track_i], 0, proj->tl->play_position);
                            start_recording();
                            if (!proj->playing) {
                                proj->play_speed = 1; 
                                playback();
                            }

                        } else {
                            proj->recording = false;
                            proj->playing = false;
                            stop_recording(proj->active_clip);
                            proj->play_speed = 0;
                        }
                        break;
                    case SDL_SCANCODE_L:
                        printf("Play!\n");
                        if (proj->recording) {
                            proj->recording = false;
                            stop_recording(proj->active_clip);
                        }
                        if (k_down) {
                            l_down = true;
                            proj->play_speed = 0.2;
                        } else {
                            if (proj->play_speed <= 0) {
                                proj->play_speed = 1;
                            } else if (proj->play_speed < 32) {
                                proj->play_speed *= 2;
                            }
                        }
                        break;
                    case SDL_SCANCODE_K:
                        k_down = true;
                        if (proj->recording) {
                            proj->recording = false;
                            stop_recording(proj->active_clip);
                        }
                        proj->play_speed = 0;
                        stop_playback();
                        proj->playing = false;
                        break;
                    case SDL_SCANCODE_J:
                        printf("Rewind!\n");
                        if (proj->recording) {
                            proj->play_speed = 0;
                            proj->playing = 0;
                            proj->recording = false;
                            stop_recording(proj->active_clip);
                        }
                        if (k_down) {
                            j_down = true;
                            proj->play_speed = -0.2;
                        } else if (proj->play_speed >= 0) {
                            proj->play_speed = -1;
                        } else if (proj->play_speed > -32) {
                            proj->play_speed *= 2;
                        }
                        break;
                    case SDL_SCANCODE_SEMICOLON:
                        translate_tl(TL_SHIFT_STEP, 0);
                        // proj->tl->offset += proj->tl->sample_frames_per_pixel * TL_SHIFT_STEP;
                        break;
                    case SDL_SCANCODE_H:
                        translate_tl(TL_SHIFT_STEP * -1, 0);
                        // proj->tl->offset -= lesser_of(proj->tl->offset, proj->tl->sample_frames_per_pixel * TL_SHIFT_STEP);
                        break;
                    case SDL_SCANCODE_COMMA: {
                        double scale_factor = 1 / SFPP_STEP;
                        rescale_timeline(scale_factor, proj->tl->play_position);
                        break;
                    }
                    case SDL_SCANCODE_PERIOD: {
                        double scale_factor = SFPP_STEP;
                        rescale_timeline(scale_factor, proj->tl->play_position);
                        break;
                    }
                    case SDL_SCANCODE_I:
                        if (cmd_ctrl_down) {
                            proj->tl->play_position = proj->tl->in_mark;
                        } else {
                            proj->tl->in_mark = proj->tl->play_position;
                        }
                        break;
                    case SDL_SCANCODE_O:
                        if (cmd_ctrl_down) {
                            proj->tl->play_position = proj->tl->out_mark;
                        } else {
                            proj->tl->out_mark = proj->tl->play_position;
                        }
                        break;
                    case SDL_SCANCODE_T:
                        if (cmd_ctrl_down) {
                            proj->play_speed = 0;
                            create_track(proj->tl, false);
                        }
                        break;
                    case SDL_SCANCODE_1:
                        if ((active_track = proj->tl->tracks[0])) {
                            active_track_i = 0;
                            if (active_track->active) {
                                active_track->active = false;
                            } else {
                                active_track->active = true;

                            }
                        }
                        break;
                    case SDL_SCANCODE_2:
                        if ((active_track = proj->tl->tracks[1])) {
                            active_track_i = 1;
                            if (active_track->active) {
                                active_track->active = false;
                            } else {
                                active_track->active = true;

                            }
                        }
                        break;
                    case SDL_SCANCODE_3:
                        if ((active_track = proj->tl->tracks[2])) {
                            active_track_i = 2;
                            if (active_track->active) {
                                active_track->active = false;
                            } else {
                                active_track->active = true;
                            }
                        }
                        break;
                    case SDL_SCANCODE_4:
                        if ((active_track = proj->tl->tracks[3])) {
                            active_track_i = 3;
                            if (active_track->active) {
                                active_track->active = false;
                            } else {
                                active_track->active = true;
                            }
                        }
                        break;
                    case SDL_SCANCODE_5:
                        if ((active_track = proj->tl->tracks[4])) {
                            active_track_i = 4;
                            if (active_track->active) {
                                active_track->active = false;
                            } else {
                                active_track->active = true;
                            }
                        }
                        break;
                    case SDL_SCANCODE_6:
                        if ((active_track = proj->tl->tracks[5])) {
                            active_track_i = 5;
                            if (active_track->active) {
                                active_track->active = false;
                            } else {
                                active_track->active = true;
                            }
                        }
                        break;
                    case SDL_SCANCODE_D:
                        proj->play_speed = 0;
                        if (proj->recording) {
                            proj->recording = false;
                            stop_recording(proj->active_clip);
                        }
                        Track *track_to_destroy;
                        if ((track_to_destroy = (proj->tl->tracks)[proj->tl->num_tracks - 1])) {
                            destroy_track(track_to_destroy);
                        } else {
                            fprintf(stderr, "Error: no track found to destry at index %d\n", proj->tl->num_tracks - 1);
                        }
                        break;
                    case SDL_SCANCODE_RIGHT:
                        proj->tl->play_position += 512;
                        break;
                    case SDL_SCANCODE_LEFT:
                        proj->tl->play_position -= 512;
                        break;
                    case SDL_SCANCODE_W:
                        write_mixdown_to_wav();
                        break;
                    default:
                        break;
                }
            } else if (e.type == SDL_KEYUP) {
                switch (e.key.keysym.scancode) {
                    case SDL_SCANCODE_LGUI:
                    case SDL_SCANCODE_LCTRL:
                        if (cmd_ctrl_down) {
                            cmd_ctrl_down = false;
                        }
                        break;
                    case SDL_SCANCODE_LSHIFT:
                    case SDL_SCANCODE_RSHIFT:
                        if (shift_down) {
                            shift_down = false;
                        }
                        break;
                    case SDL_SCANCODE_K:
                        if (k_down) {
                            k_down = false;
                        }
                        break;
                    case SDL_SCANCODE_J:
                        if (j_down) {
                            j_down = false;
                            proj->play_speed = 0;
                        }
                        break;
                    case SDL_SCANCODE_L:
                        if (l_down) {
                            l_down = false;
                            proj->play_speed = 0;
                        }
                        break;
                    default:
                        break;
                }
            }
        }
        // if (mousebutton_down && !proj->playing) {
        //     get_mouse_state(&mouse_p);
        //     proj->tl->play_position = get_abs_tl_x(mouse_p.x);
        // }

        if (proj->play_speed < 0 && proj->tl->offset > proj->tl->play_position) {
            translate_tl(CATCHUP_STEP * -1, 0);
        } else if (proj->play_speed > 0 && get_tl_draw_x(proj->tl->play_position) > proj->jwin->w) {
            translate_tl(CATCHUP_STEP, 0);
        }
        playback();
        draw_project(proj);
        SDL_Delay(1);
    }
}


char *get_home_dir()
{
    uid_t uid = getuid();
    struct passwd *pw = getpwuid(uid);
    return pw->pw_dir;
}

int main()
{
    home_dir = get_home_dir();

    //TODO: open project creation window if proj==NULL;
    init_graphics();
    init_audio();
    init_SDL_ttf();
    JDAWWindow *new_project = create_jwin("Create a new Jackdaw project", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 600, 400);
    new_project_loop(new_project);

    // create_or_open_project();
    proj = create_project("Untitled");
    // init_fonts(fonts, OPEN_SANS);
    // init_fonts(bold_fonts, OPEN_SANS_BOLD);
    // set_dpi_scale_factor(proj->win, proj->rend);
    fprintf(stderr, "project scale factor: %d\n", scale_factor);
    // proj->tl->rect = get_rect(proj->winrect, TL_RECT); // TODO: Why is this here?


    AudioDevice **playback_devices = NULL;
    AudioDevice **record_devices = NULL;
    int num_playback_devices = query_audio_devices(&playback_devices, 0);
    int num_record_devices = query_audio_devices(&record_devices, 1);
    for (int i=0; i<num_playback_devices; i++) {
        AudioDevice *dev = playback_devices[i];
        if (open_audio_device(dev, dev->spec.channels, SAMPLE_RATE, CHUNK_SIZE) == 0) {
            fprintf(stderr, "Opened audio device: %s\n\tchannels: %d\n\tformat: %s\n", dev->name, dev->spec.channels, get_audio_fmt_str(dev->spec.format));

        } else {
            // fprintf(stderr, "Error: failed to open device %s\n", dev->name);
        }
    }
    for (int i=0; i<num_record_devices; i++) {
        AudioDevice *dev = record_devices[i];
        if (open_audio_device(dev, dev->spec.channels, SAMPLE_RATE, CHUNK_SIZE) == 0) {
            fprintf(stderr, "Opened record device: %s\n\tchannels: %u\n\tformat: %s\n", dev->name, dev->spec.channels, get_audio_fmt_str(dev->spec.format));

        } else {
            // fprintf(stderr, "Error: failed to open record device %s\n", dev->name);
        }
    }
    proj->record_devices = record_devices;
    proj->playback_devices = playback_devices;
    proj->num_record_devices = num_record_devices;
    proj->num_playback_devices = num_playback_devices;
    // txt_soft_c = get_color(txt_soft);
    // txt_main_c = get_color(txt_main);


    project_loop();

    SDL_Quit();
}