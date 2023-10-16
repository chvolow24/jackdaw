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

/*vvv Unportable vvv*/
#include <pthread.h>
#include <unistd.h>
#include <pwd.h>
#include <dirent.h>
#include <signal.h>
/*^^^^^^^^^^^^^^^^^^*/

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
#include "timeline.h"
#include "dot_jdaw.h"
#include "dsp.h"


#define abs_min(a,b) (abs(a) < abs(b) ? a : b)
#define lesser_of(a,b) (a < b ? a : b)
#define greater_of(a,b) (a > b ? a : b)

/* GLOBALS */
Project *proj = NULL;
uint8_t scale_factor = 1;
bool dark_mode = true;
char *home_dir;
bool sys_byteorder_le;

extern int write_buffpos;

extern JDAW_Color bckgrnd_color;
extern JDAW_Color tl_bckgrnd;
extern JDAW_Color txt_soft;
extern JDAW_Color txt_main;

extern JDAW_Color lightgrey;
extern JDAW_Color black;
extern JDAW_Color white;
extern JDAW_Color grey;

static void get_native_byte_order()
{
    #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    sys_byteorder_le = true;
    #else
    sys_byteorder_le = false;
    #endif
}

static int segfault_counter = 0;

static void mem_err_handler()
{
    FILE *f = fopen("error.log", "w");
    segfault_counter++;
    if (segfault_counter > 10) {
        fclose(f);
        exit(1);
    }
    fprintf(stderr, "\nSEGMENTATION FAULT\n");
    log_project_state(f);
    fclose(f);
    exit(1);
}

static void sigint_handler()
{
    FILE *f = fopen("project_state.log", "w");
    fprintf(stderr, "SIG INT: logging project state.\n");
    log_project_state(f);
    fclose(f);
    exit(1);
}

static void signal_init(void)
{

    struct sigaction memErrHandler;
    struct sigaction sigIntHandler;
    memErrHandler.sa_handler = mem_err_handler;
    sigIntHandler.sa_handler = sigint_handler;
    sigemptyset(&memErrHandler.sa_mask);
    sigemptyset(&sigIntHandler.sa_mask);
    memErrHandler.sa_flags = 0;
    sigIntHandler.sa_flags = 0;
    // sigaction(SIGINT, &sigIntHandler, NULL);
    sigaction(SIGSEGV, &memErrHandler, NULL);
    sigaction(SIGTRAP, &memErrHandler, NULL);
    sigaction(SIGABRT, &memErrHandler, NULL);
    sigaction(SIGBUS, &memErrHandler, NULL);
    sigaction(SIGINT, &sigIntHandler, NULL);


}

/* Initialize SDL Video and TTF */
static void init_graphics()
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

/* Start timeline playback */
static void playback()
{
    if (proj->play_speed != 0 && !(proj->playing)) {
        start_device_playback();
        proj->playing = true;
    }
}

/* Get the current mouse position and ensure coordinates are scaled appropriately */
static void get_mouse_state(SDL_Point *mouse_p) 
{
    SDL_GetMouseState(&(mouse_p->x), &(mouse_p->y));
    mouse_p->x *= scale_factor;
    mouse_p->y *= scale_factor;
}

/* Triage mouseclicks that occur within a Jackdaw Project */
static void triage_project_mouseclick(SDL_Point *mouse_p, bool cmd_ctrl_down)
{
    // fprintf(stderr, "In main triage mouseclick %d, %d\n", (*mouse_p).x, (*mouse_p).y);
    // fprintf(stderr, "\t-> tlrect: %d %d %d %d\n", proj->tl->rect.x, proj->tl->rect.y, proj->tl->rect.w, proj->tl->rect.h);
    // fprintf(stderr, "\t-> audiorect: %d %d %d %d\n", proj->tl->audio_rect.x, proj->tl->audio_rect.y, proj->tl->audio_rect.w, proj->tl->audio_rect.h);

    if (SDL_PointInRect(mouse_p, &(proj->tl->rect))) {
        if (SDL_PointInRect(mouse_p, &(proj->tl->audio_rect))) {
            proj->tl->play_position = get_abs_tl_x(mouse_p->x);
            fprintf(stderr, "reset pos! %d\n", proj->tl->play_position);
        }
        for (int i=0; i<proj->tl->num_tracks; i++) {
            Track *track;
            if ((track = proj->tl->tracks[i])) {
                if (SDL_PointInRect(mouse_p, &(track->rect))) {
                    if (SDL_PointInRect(mouse_p, &(track->name_box->container))) {
                        fprintf(stderr, "\t-> mouse in namebox\n");
                        track->name_box->onclick(track->name_box, (void *)track);
                    } else if (SDL_PointInRect(mouse_p, &(track->input_name_box->container))) {
                        select_track_input_menu((void *)track);
                    } else if (cmd_ctrl_down) {
                        for (uint8_t c=0; c<track->num_clips; c++) {
                            Clip* clip = track->clips[c];
                            if (SDL_PointInRect(mouse_p, &clip->rect)) {
                                grab_ungrab_clip(clip);
                            }
                        }
                    }
                }
            }
        }
    }
}

/* Present a window to allow user to create or open a project. Return project name */
static char *new_project_loop(JDAWWindow *jwin) 
{
    fprintf(stderr, "New project loop\n");
    MenulistItem *mlarray[255];
    DIR *homedir = opendir(home_dir);
    int i=0;
    struct dirent *rdir;
    while ((rdir = readdir(homedir)) != NULL)
    {
        mlarray[i] = malloc(sizeof(MenulistItem));
        // fprintf(stderr, "%s: %hhu\n", rdir->d_name, rdir->d_type);
        if (rdir->d_type == DT_DIR && rdir->d_name[0] != '.') {
            // strcpy(mlarray[i]->label, )
            // fprintf(stderr, "\tOK\n");
            // char *mlin = malloc(255);
            // mlarray[i]->label = mlin;
            // fprintf(stderr, "Copying %s to mlarray[%d]->label\n", rdir->d_name, i);
            strcpy(mlarray[i]->label, rdir->d_name);
            mlarray[i]->available = true;
            i++;
        }
    }
    fprintf(stderr, "Create menulist\n");
    TextboxList *testlist = create_menulist(
        jwin, 400, 5, jwin->fonts[2], mlarray, i, NULL, NULL
    );
    fprintf(stderr, "END menulist\n");

    position_textbox_list(testlist, 100, 50);

    SDL_Point mouse_p;
    bool quit = false;
    while (!quit) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT || (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_CLOSE)) {
                quit = true;
            }
        }
        get_mouse_state(&mouse_p);
        menulist_hover(jwin, &mouse_p);
        draw_jwin_background(jwin);
        draw_jwin_menus(jwin);
        SDL_RenderPresent(jwin->rend);
        SDL_Delay(1);
    }
    i--;
    while (i>0) {
        free(mlarray[i]);
        i--;
    }
    destroy_jwin(jwin);
    return "";
}

static void start_recording()
{
    write_buffpos = 0;
    Track *track = NULL;
    // AudioDevice *dev_list[proj->tl->num_active_tracks];
    // uint8_t num_active_devices = 0;
    fprintf(stderr, "START RECORDING. active tracks: %d\n", proj->tl->num_active_tracks);
    for (uint8_t i=0; i<proj->tl->num_active_tracks; i++) {
        track = proj->tl->tracks[proj->tl->active_track_indices[i]];
        track->input->active = true;
        fprintf(stderr, "set device %s to active\n", track->input->name);
        Clip *clip = create_clip(track, 0, proj->tl->play_position);
        fprintf(stderr, "Creating clip @abspos: %d\n", proj->tl->play_position);
        add_active_clip(clip);
        // uint8_t j=0;
        // while (j < num_active_devices && clip->input != dev_list[j]) {
        //     j++;
        // }
        // if (j == num_active_devices) {
        //     dev_list[j] = clip->input;
        //     num_active_devices++;
        // }
    }
    for (uint8_t i=0; i<proj->num_record_devices; i++) {
        AudioDevice *dev = NULL;
        if ((dev = proj->record_devices[i]) && dev->active) {
            fprintf(stderr, "Device: %s, active? %d\n", dev->name, dev->active);
            start_device_recording(dev);
        }
    }
    proj->play_speed = 1;
}

static void stop_recording()
{
    fprintf(stderr, "Enter stop_recording\n");
    proj->play_speed = 0;
    Track *track = NULL;
    for (uint8_t i=0; i<proj->tl->num_active_tracks; i++) {
        track = proj->tl->tracks[proj->tl->active_track_indices[i]];
        stop_device_recording(track->input);
    }

    Clip *clip = NULL;
    for (uint8_t i=0; i<proj->num_active_clips; i++) {
        fprintf(stderr, "Active clip index %d\n", i);
        if (!(clip = proj->active_clips[i])) {
            fprintf(stderr, "Error: active clip not found at index %d\n", i);
            exit(1);
        }
        fprintf(stderr, "\t->found clip. Copying buf\n");
        copy_buff_to_clip(clip); //TODO: consider whether this needs to be multi-threaded.
        uint32_t abs_pos_saved = clip->abs_pos_sframes;
        reposition_clip(clip, clip->abs_pos_sframes - proj->tl->record_offset);
        fprintf(stderr, "Repoisitioning clip from %d to %d\n", abs_pos_saved, clip->abs_pos_sframes);
        // pthread_t t;
        // pthread_create(&t, NULL, copy_buff_to_clip, proj->active_clips[i]);
    }
    for (uint8_t i=0; i<proj->num_record_devices; i++) {
        AudioDevice* dev = proj->record_devices[i];
        if (dev->active) {
            memset(dev->rec_buffer, '\0', BUFFLEN / 2);
            dev->write_buffpos_sframes = 0;
            dev->active = false;
        }
    }
    proj->tl->record_offset = 0;
    fprintf(stderr, "Clearing active clips\n");
    clear_active_clips();
}
    // fprintf(stderr, "Enter stop_recording\n");
    // SDL_PauseAudioDevice(dev->id, 1);
    // pthread_t t;
    // pthread_create(&t, NULL, copy_buff_to_clip, clip);
    // fprintf(stderr, "\t->exit stop_recording\n");

static void start_or_stop_recording()
{
    if (proj->recording) {
        stop_recording();
        proj->recording = false;
    } else {
        start_recording();
        proj->recording = true;
    }
}


static void project_loop()
{
    // int active_track_i = 0;
    // Track *active_track = NULL;
    bool mousebutton_down = false;
    bool cmd_ctrl_down = false;
    bool shift_down = false;
    bool k_down = false;
    bool l_down = false;
    bool j_down = false;
    bool equals_down = false;
    bool minus_down = false;
    bool nine_down = false;
    bool zero_down = false;
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
                /* Triage mouseclick to GUI menus first. Check project for clickables IFF no GUI item is clicked */
                if (!triage_menulist_mouseclick(proj->jwin, &mouse_p)) {
                    triage_project_mouseclick(&mouse_p, cmd_ctrl_down);
                }
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
                        start_or_stop_recording();
                        break;
                    case SDL_SCANCODE_L:
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
                        proj->play_speed = 0;
                        stop_device_playback();
                        proj->playing = false;
                        break;
                    case SDL_SCANCODE_J:
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
                    case SDL_SCANCODE_G: {
                        fprintf(stderr, "SCANCODE_G\n");
                        if (proj_grabbed_clips() != 0 && !shift_down) {
                            ungrab_clips();
                        } else {
                            fprintf(stderr, "\tSince no clip found, grabbing\n");
                            grab_clips();
                        }
                    }
                        break;
                    case SDL_SCANCODE_S: {
                        if (cmd_ctrl_down) {
                            write_jdaw_file("project.jdaw");
                            fprintf(stderr, "DONE WRITING FILE!\n");
                        }
                    }
                    case SDL_SCANCODE_1:
                        activate_or_deactivate_track(0);
                        break;
                    case SDL_SCANCODE_2:
                        activate_or_deactivate_track(1);
                        break;
                    case SDL_SCANCODE_3:
                        activate_or_deactivate_track(2);
                        break;
                    case SDL_SCANCODE_4:
                        activate_or_deactivate_track(3);
                        break;
                    case SDL_SCANCODE_5:
                        activate_or_deactivate_track(4);
                        break;
                    case SDL_SCANCODE_6:
                        activate_or_deactivate_track(5);
                        break;
                    case SDL_SCANCODE_7:
                        activate_or_deactivate_track(6);
                        break;
                    case SDL_SCANCODE_8:
                        activate_or_deactivate_track(7);
                        break;
                    case SDL_SCANCODE_9:
                        if (!shift_down) {
                            activate_or_deactivate_track(8);
                        }
                        nine_down = true;
                        break;
                    case SDL_SCANCODE_0:
                        zero_down = true;
                        break;
                    case SDL_SCANCODE_D:
                        proj->play_speed = 0;
                        Track *track_to_destroy;
                        if ((track_to_destroy = (proj->tl->tracks)[proj->tl->num_tracks - 1])) {
                            destroy_track(track_to_destroy);
                        } else {
                            fprintf(stderr, "Error: no track found to destry at index %d\n", proj->tl->num_tracks - 1);
                        }
                        break;
                    case SDL_SCANCODE_RIGHT: {
                        int32_t translate_by;
                        if (cmd_ctrl_down) {
                            translate_by = get_tl_abs_w(ARROW_TL_STEP_SMALL);
                        } else {
                            translate_by = get_tl_abs_w(ARROW_TL_STEP);
                        }
                        proj->tl->play_position += translate_by;
                        translate_grabbed_clips(translate_by);
                        proj->tl->play_position += get_tl_abs_w(ARROW_TL_STEP);
                    }
                        break;
                    case SDL_SCANCODE_LEFT: {
                        int32_t translate_by;
                        if (cmd_ctrl_down) {
                            translate_by = get_tl_abs_w(ARROW_TL_STEP_SMALL) * -1;
                        } else {
                            translate_by = get_tl_abs_w(ARROW_TL_STEP) * -1;
                        }
                        proj->tl->play_position += translate_by;
                        translate_grabbed_clips(translate_by);
                        proj->tl->play_position += get_tl_abs_w(ARROW_TL_STEP);
                    }
                        break;
                    case SDL_SCANCODE_W:
                        if (shift_down) {
                            write_mixdown_to_wav();
                        }
                        break;
                    case SDL_SCANCODE_BACKSPACE:
                    case SDL_SCANCODE_DELETE:
                        fprintf(stderr, "SCANCODE DELETE\n");
                        delete_grabbed_clips();
                        break;
                    case SDL_SCANCODE_EQUALS:
                        equals_down = true;
                        break;
                    case SDL_SCANCODE_MINUS:
                        minus_down = true;
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
                    case SDL_SCANCODE_EQUALS:
                        equals_down = false;
                        process_vol_and_pan();
                        break;
                    case SDL_SCANCODE_MINUS:
                        minus_down = false;
                        process_vol_and_pan();
                        break;
                    case SDL_SCANCODE_9:
                        nine_down = false;
                        process_vol_and_pan();
                        break;
                    case SDL_SCANCODE_0:
                        zero_down = false;
                        process_vol_and_pan();
                    default:
                        break;
                }
            }
        }

        if (mousebutton_down && !proj->playing && !proj->recording) {
            get_mouse_state(&mouse_p);
            proj->tl->play_position = get_abs_tl_x(mouse_p.x);
        }
        if (proj->play_speed < 0 && proj->tl->offset > proj->tl->play_position) {
            translate_tl(CATCHUP_STEP * -1, 0);
        } else if (proj->play_speed > 0 && get_tl_draw_x(proj->tl->play_position) > proj->jwin->w) {
            translate_tl(CATCHUP_STEP, 0);
        }
        if (shift_down && (equals_down || minus_down)) {
            Track *track = NULL;
            for (uint8_t i=0; i<proj->tl->num_active_tracks; i++) {
                track = proj->tl->tracks[proj->tl->active_track_indices[i]];
                if (!track) {
                    fprintf(stderr, "Fatal error: track not found at active track index.\n");
                    exit(1);
                }
                float adjust_by = minus_down ? -0.02 : 0.02;
                adjust_track_vol(track, adjust_by);
            }
        }
        if (shift_down && (nine_down || zero_down)) {
            Track *track = NULL;
            for (uint8_t i=0; i<proj->tl->num_active_tracks; i++) {
                track = proj->tl->tracks[proj->tl->active_track_indices[i]];
                if (!track) {
                    fprintf(stderr, "Fatal error: track not found at active track index.\n");
                    exit(1);
                }
                float adjust_by = nine_down ? -0.02 : 0.02;
                adjust_track_pan(track, adjust_by);
            }
        }
        get_mouse_state(&mouse_p);
        menulist_hover(proj->jwin, &mouse_p);
        playback();
        draw_project(proj);
        draw_jwin_menus(proj->jwin);
        SDL_RenderPresent(proj->jwin->rend);
        SDL_Delay(1);
    }
}


static char *get_home_dir()
{
    uid_t uid = getuid();
    struct passwd *pw = getpwuid(uid);
    return pw->pw_dir;
}

int main()
{
    get_native_byte_order();
    signal_init();
    home_dir = get_home_dir();

    //TODO: open project creation window if proj==NULL;
    init_graphics();
    init_audio();
    init_SDL_ttf();
    JDAWWindow *new_project = create_jwin("Create a new Jackdaw project", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 600, 400);
    new_project_loop(new_project);

    fprintf(stdout, "Opening project\n");
    if ((proj = open_jdaw_file("project.jdaw")) == NULL) {
        fprintf(stderr, "Creating new project\n");
        proj = create_project("Untitled", 2, 48000, AUDIO_S16SYS, 512);
        activate_audio_devices(proj);
    } else {
        fprintf(stderr, "Successfully opened project.\n");
    }
    fprintf(stdout, "Activating audio devices on project\n");


        fprintf(stdout, "Loop starts now\n");

    project_loop();

    SDL_Quit();
}