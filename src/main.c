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
#include "transition.h"

#define abs_min(a,b) (abs(a) < abs(b) ? a : b)
#define lesser_of(a,b) (a < b ? a : b)
#define greater_of(a,b) (a > b ? a : b)

/* GLOBALS */
Project *proj = NULL;
uint8_t scale_factor = 1;
bool dark_mode = true;
char *home_dir;
bool sys_byteorder_le;
uint8_t animation_delay = 10;

// extern int write_buffpos;

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

// static int segfault_counter = 0;

// static void mem_err_handler()
// {
//     FILE *f = fopen("error.log", "w");
//     segfault_counter++;
//     if (segfault_counter > 10) {
//         fclose(f);
//         exit(1);
//     }
//     fprintf(stderr, "\nSEGMENTATION FAULT\n");
//     log_project_state(f);
//     fclose(f);
//     exit(1);
// }

// static void sigint_handler()
// {
//     FILE *f = fopen("project_state.log", "w");
//     fprintf(stderr, "SIG INT: logging project state.\n");
//     log_project_state(f);
//     fclose(f);
//     exit(1);
// }

// static void signal_init(void)
// {

//     struct sigaction memErrHandler;
//     struct sigaction sigIntHandler;
//     memErrHandler.sa_handler = mem_err_handler;
//     sigIntHandler.sa_handler = sigint_handler;
//     sigemptyset(&memErrHandler.sa_mask);
//     sigemptyset(&sigIntHandler.sa_mask);
//     memErrHandler.sa_flags = 0;
//     sigIntHandler.sa_flags = 0;
//     // sigaction(SIGINT, &sigIntHandler, NULL);
//     sigaction(SIGSEGV, &memErrHandler, NULL);
//     sigaction(SIGTRAP, &memErrHandler, NULL);
//     sigaction(SIGABRT, &memErrHandler, NULL);
//     sigaction(SIGBUS, &memErrHandler, NULL);
//     sigaction(SIGINT, &sigIntHandler, NULL);


// }

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

void stop_playback()
{
    proj->play_speed = 0;
    proj->playing = false;
    proj->recording = false;
    stop_device_playback();
}

/* Get the current mouse position and ensure coordinates are scaled appropriately */
static void get_mouse_position(SDL_Point *mouse_p) 
{
    SDL_GetMouseState(&(mouse_p->x), &(mouse_p->y));
    mouse_p->x *= scale_factor;
    mouse_p->y *= scale_factor;
}

/* Triage mouseclicks that occur within a Jackdaw Project
Return values:
    0: Nothing
    1: track clicked
    2: clip clicked
*/
static int triage_project_mouseclick(SDL_Point *mouse_p, bool cmd_ctrl_down, Track **clicked_track, Clip **clicked_clip)
{
    /* DNE! */
    //fprintf(stderr, "Mouse xy: %d,%d. audio out cont xrange: %d, %d, yrange %d, %d\n", mouse_p->x, mouse_p->y, proj->audio_out->container.x, proj->audio_out->container.x + proj->audio_out->container.w, proj->audio_out->container.y, proj->audio_out->container.y + proj->audio_out->container.w);

    Textbox *tb = NULL;
    int ret = 0;
    if (SDL_PointInRect(mouse_p, &(proj->ctrl_rect))) {
        if (SDL_PointInRect(mouse_p, &((tb = proj->audio_out)->container))) {
            tb->onclick(tb, tb->target);
            // select_audio_out_menu((void *)proj);
        }
    } else if (SDL_PointInRect(mouse_p, &(proj->tl->rect))) {
        for (int i=0; i<proj->tl->num_tracks; i++) {
            Track *track;
            if ((track = proj->tl->tracks[i])) {
                if (SDL_PointInRect(mouse_p, &(track->rect))) {
                    *clicked_track = track;
                    ret = 1;
                    // Textbox *tb = NULL;
                    if (SDL_PointInRect(mouse_p, &((tb = track->name_box)->container))) {
                        tb->onclick(tb, tb->target);
                    } else if (SDL_PointInRect(mouse_p, &((tb = track->input_name_box)->container))) {
                        tb->onclick(tb, tb->target);
                    } else if (SDL_PointInRect(mouse_p, &((tb = track->mute_button_box)->container))) {
                        tb->onclick(tb, tb->target);
                    } else if (SDL_PointInRect(mouse_p, &((tb = track->solo_button_box)->container))) {
                        tb->onclick(tb, tb->target);
                    } else {
                        for (uint8_t c=0; c<track->num_clips; c++) {
                            Clip* clip = track->clips[c];
                            ret = 2;
                            // if (SDL_PointInRect(mouse_p, &(clip->namebox->container))) { // TODO: Right click action
                            //     edit_textbox(clip->namebox, draw_project, proj);
                            // }
                            if (SDL_PointInRect(mouse_p, &(clip->rect))) {
                                *clicked_clip = clip;
                                if (cmd_ctrl_down) {
                                    grab_ungrab_clip(clip);
                                }
                            }
                        }
                    }
                }
            }
        }
        if (cmd_ctrl_down && !(*clicked_clip)) {
            ungrab_clips();
        }
    }
    return ret;
}


static void triage_project_rightclick(SDL_Point *mouse_p)
{
    if (SDL_PointInRect(mouse_p, &(proj->ctrl_rect))) {

    } else if (SDL_PointInRect(mouse_p, &(proj->tl->rect))) {
        for (int i=0; i<proj->tl->num_tracks; i++) {
            Track *track;
            if ((track = proj->tl->tracks[i])) {
                if (SDL_PointInRect(mouse_p, &(track->rect))) {
                    track_actions_menu(track, mouse_p);
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
    // write_buffpos = 0;
    Track *track = NULL;
    // AudioDevice *dev_list[proj->tl->num_active_tracks];
    // uint8_t num_active_devices = 0;
    fprintf(stderr, "START RECORDING. active tracks: %d\n", proj->tl->num_active_tracks);
    for (uint8_t i=0; i<proj->tl->num_active_tracks; i++) {
        track = proj->tl->tracks[proj->tl->active_track_indices[i]];
        if (track->input->active == false) {
            track->input->active = true;
            if (open_audio_device(proj, track->input, 2) != 0) {
                fprintf(stderr, "Unable to open device: %s\n", track->input->name);
            }
        }
// void start_device_recording(AudioDevice *dev)
// {
//     fprintf(stderr, "START RECORDING dev: %s\n", dev->name);
//     if () {
//         fprintf(stderr, "Opened device\n");
//         SDL_PauseAudioDevice(dev->id, 0);
//     } else {
//         fprintf(stderr, "Failed to open device\n");
//     }
// }
        Clip *clip = create_clip(track, 0, proj->tl->play_pos_sframes);
        fprintf(stderr, "Creating clip @abspos: %d\n", proj->tl->play_pos_sframes);
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
            // start_device_recording(dev);
            SDL_PauseAudioDevice(dev->id, 0);
        }
    }
    proj->play_speed = 1;
    proj->recording = true;
}

static void stop_recording()
{
    fprintf(stderr, "Enter stop_recording\n");
    proj->play_speed = 0;
    proj->recording = false;
    proj->playing = false;
    Track *track = NULL;
    for (uint8_t i=0; i<proj->tl->num_active_tracks; i++) {
        track = proj->tl->tracks[proj->tl->active_track_indices[i]];
        stop_device_recording(track->input);
    }

    Clip *clip = NULL;
    for (uint8_t i=0; i<proj->num_active_clips; i++) {
        if (!(clip = proj->active_clips[i])) {
            fprintf(stderr, "Error: active clip not found at index %d\n", i);
            exit(1);
        }
        copy_device_buff_to_clip(clip); //TODO: consider whether this needs to be multi-threaded.

        //TODO: reposition clip after recording

        // reposition_clip(clip, clip->abs_pos_sframes - proj->tl->record_offset);
    }
    for (uint8_t i=0; i<proj->num_record_devices; i++) {
        AudioDevice* dev = proj->record_devices[i];
        if (dev->active) {
            memset(dev->rec_buffer, '\0', dev->rec_buf_len_samples * sizeof(int16_t));
            dev->write_buffpos_samples = 0;
            dev->active = false;
        }
    }
    // proj->tl->record_offset = 0;
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
        stop_playback();
    } else {
        start_recording();
    }
}


static void project_loop()
{
    // int active_track_i = 0;
    // Track *active_track = NULL;
    bool mousebutton_down = false;
    bool mousebutton_right_down = false;
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

    // bool reprocess_on_mouse_up = false;
    SDL_Point mouse_p;
    SDL_Point mouse_p_click;
    Track *clicked_track = NULL;

    Clip *clicked_clip = NULL;
    Track *mouse_track = NULL;
    int dynamic_clicked_track_rank = -1;
    while (!quit) {
        // get_mouse_state(&mouse_p); // TODO: 

        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT || (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_CLOSE)) {
                quit = true;
            } else if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                reset_dims(proj->jwin);
                // reset_tl_rect(proj->tl);
                reset_tl_rects(proj);
                reset_ctrl_rects(proj);
            } else if (e.type == SDL_MOUSEMOTION) {
                mouse_p.x = e.motion.x * scale_factor;
                mouse_p.y = e.motion.y * scale_factor;
            } else if (e.type == SDL_MOUSEBUTTONUP) {
                clicked_track = NULL;
                clicked_clip = NULL;
                dynamic_clicked_track_rank = -1;
                // if (reprocess_on_mouse_up) {
                //     process_vol_and_pan();
                //     reprocess_on_mouse_up = false;
                // }
                if (e.button.button == SDL_BUTTON_LEFT) {
                    mousebutton_down = false;
                } else if (e.button.button == SDL_BUTTON_RIGHT) {
                    mousebutton_right_down = false;
                }
            } else if (e.type == SDL_MOUSEBUTTONDOWN) {
                get_mouse_position(&mouse_p_click);
                if (e.button.button == SDL_BUTTON_LEFT) {
                    /* Triage mouseclick to GUI menus first. Check project for clickables IFF no GUI item is clicked */
                    if (!triage_menulist_mouseclick(proj->jwin, &mouse_p)) {
                        int t = triage_project_mouseclick(&mouse_p, cmd_ctrl_down, &clicked_track, &clicked_clip);
                        if (clicked_track) {
                            dynamic_clicked_track_rank = clicked_track->tl_rank;
                        }
                    }
                    mousebutton_down = true;
                } else if (e.button.button == SDL_BUTTON_RIGHT) {
                    if (!triage_menulist_mouseclick(proj->jwin, &mouse_p)) {
                        triage_project_rightclick(&mouse_p);
                    }
                    mousebutton_right_down = true;
                }
            } else if (e.type == SDL_MOUSEWHEEL) {
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
                        // proj->play_speed = 0;
                        // stop_playback();
                        if (proj->recording) {
                            stop_recording();
                        } else {
                            stop_playback();
                        }
                        // stop_device_playback();
                        // proj->playing = false;
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
                        rescale_timeline(scale_factor, proj->tl->play_pos_sframes);
                        break;
                    }
                    case SDL_SCANCODE_PERIOD: {
                        double scale_factor = SFPP_STEP;
                        rescale_timeline(scale_factor, proj->tl->play_pos_sframes);
                        break;
                    }
                    case SDL_SCANCODE_I:
                        if (cmd_ctrl_down) {
                            set_play_position(proj->tl->in_mark_sframes);
                            // proj->tl->play_position = proj->tl->in_mark_sframes;
                        } else {
                            proj->tl->in_mark_sframes = proj->tl->play_pos_sframes;
                        }
                        break;
                    case SDL_SCANCODE_O:
                        if (cmd_ctrl_down) {
                            set_play_position(proj->tl->out_mark_sframes);
                            // proj->tl->play_position = proj->tl->out_mark_sframes;
                        } else {
                            proj->tl->out_mark_sframes = proj->tl->play_pos_sframes;
                        }
                        break;
                    case SDL_SCANCODE_T:
                        if (cmd_ctrl_down) {
                            proj->play_speed = 0;
                            create_track(proj->tl, false);
                        }
                        break;
                    case SDL_SCANCODE_G: {
                        if (num_grabbed_clips() > 0 && !shift_down) {
                            ungrab_clips();
                        } else {
                            grab_clips();
                        }
                    }
                        break;
                    case SDL_SCANCODE_S: {
                            if (cmd_ctrl_down) {
                                write_jdaw_file("project.jdaw");
                                fprintf(stderr, "DONE WRITING FILE!\n");
                            } else {
                                solo_unsolo();
                            }
                        }
                        break;
                    case SDL_SCANCODE_GRAVE:
                        activate_deactivate_all_tracks();
                        break;
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
                    case SDL_SCANCODE_M:
                        mute_unmute();
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
                        set_play_position(translate_by);
                        // proj->tl->play_position += translate_by;
                        translate_grabbed_clips(translate_by);
                    }
                        break;
                    case SDL_SCANCODE_LEFT: {
                        int32_t translate_by;
                        if (cmd_ctrl_down) {
                            translate_by = get_tl_abs_w(ARROW_TL_STEP_SMALL) * -1;
                        } else {
                            translate_by = get_tl_abs_w(ARROW_TL_STEP) * -1;
                        }
                        // proj->tl->play_position_sframes += translate_by;
                        move_play_position(translate_by);
                        // if (proj->tl->play_position < 0) { //TODO: allow space for negative tl pos in all modules.
                        //     set_play_position(0);
                        //     // proj->tl->play_position = 0;
                        // }
                        translate_grabbed_clips(translate_by);
                    }
                        break;
                    case SDL_SCANCODE_W:
                        if (shift_down) {
                            write_mixdown_to_wav("testfile.wav");
                        }
                        break;
                    case SDL_SCANCODE_BACKSPACE:
                    case SDL_SCANCODE_DELETE:
                        delete_grabbed_clips();
                        break;
                    case SDL_SCANCODE_EQUALS:
                        equals_down = true;
                        break;
                    case SDL_SCANCODE_MINUS:
                        minus_down = true;
                        break;
                    case SDL_SCANCODE_Q:
                        if (proj) {
                            stop_playback();
                            destroy_audio_devices(proj);
                            activate_audio_devices(proj);
                        }
                        break;
                    case SDL_SCANCODE_BACKSLASH:
                        if (proj) {
                            add_transition_from_tl();
                        }
                    case SDL_SCANCODE_C:
                        if (shift_down) {
                            cut_clips();
                        } else if (cmd_ctrl_down) {
                            copy_clips_to_clipboard();
                        }
                        break;
                    case SDL_SCANCODE_V:
                        if (cmd_ctrl_down) {
                            paste_clips();
                        }
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
                        // process_vol_and_pan();
                        break;
                    case SDL_SCANCODE_MINUS:
                        minus_down = false;
                        // process_vol_and_pan();
                        break;
                    case SDL_SCANCODE_9:
                        nine_down = false;
                        // process_vol_and_pan();
                        break;
                    case SDL_SCANCODE_0:
                        zero_down = false;
                        // process_vol_and_pan();
                    default:
                        break;
                }
            }
        }
        if (mousebutton_down) {
            if (SDL_PointInRect(&mouse_p, &(proj->tl->audio_rect))) {
                for (uint8_t i=0; i<proj->tl->num_tracks; i++) {
                    Track *track = proj->tl->tracks[i];
                    if (SDL_PointInRect(&mouse_p, &(track->rect)) && track != mouse_track) {
                        mouse_track = track;
                        fprintf(stderr, "\n\nMouse track moved to rank: %d\n", mouse_track->tl_rank);
                        break;
                    }
                }
                int move_by_x = mouse_p.x - mouse_p_click.x;
                // int move_by_y = mouse_p.y - mouse_p_click.y;
                bool clip_moved_x = false;
                bool clip_changed_track = false;
                if (clicked_clip && clicked_clip->grabbed && !proj->playing && !proj->recording) {
                    if (move_by_x != 0) {
                        if (num_grabbed_clips() > 0) {
                            for (uint8_t i=0; i<proj->tl->num_tracks; i++) {
                                Track *track = proj->tl->tracks[i];
                                if (track->num_grabbed_clips > 0) {
                                    for (uint8_t j=0; j<track->num_clips; j++) {
                                        Clip *clip = track->clips[j];
                                        if (clip->grabbed) {
                                            clip->abs_pos_sframes += get_tl_abs_w(move_by_x);
                                            reset_cliprect(clip);
                                            clip_moved_x = true;
                                        }
                                    }
                                }
                            }
                        }
                    }

                    // int track_height = proj->tl->tracks[0]->rect.h; // TODO: Fix this. 
                    if (mouse_track && mouse_track->tl_rank != dynamic_clicked_track_rank) {
                        int move_by_tracks = mouse_track->tl_rank - dynamic_clicked_track_rank;
                    // if (abs(move_by_y) >= track_height) {
                        if (move_by_tracks != 0) {
                            for (uint8_t i=0; i<proj->tl->num_tracks; i++) {
                                Track *track = proj->tl->tracks[i];
                                if (track->num_grabbed_clips > 0) {
                                    // fprintf(stderr, "\nTrack %d:\n", i);
                                    Clip *clips_to_move[track->num_grabbed_clips];
                                    uint8_t clips_added = 0;
                                    for (uint8_t j=0; j<track->num_clips; j++) {
                                        Clip *clip = track->clips[j];
                                        if (clip->grabbed && !(clip->changed_track)) {
                                            // fprintf(stderr, "\t->Candidate clip %p\n", clip);
                                            if (track->tl_rank + move_by_tracks >= 0 && track->tl_rank + move_by_tracks < proj->tl->num_tracks) {
                                                // fprintf(stderr, "\t\t->allowed\n");
                                                clips_to_move[clips_added] = clip;
                                                clips_added++;
                                            } else {
                                                // fprintf(stderr, "\t\t->CANNOT MOVE!\n");
                                            }
                                        }
                                    }
                                    // fprintf(stderr, "\tMOVING %d clips\n", clips_added);
                                    for (uint8_t i=0; i<clips_added; i++) {
                                        Clip *clip = clips_to_move[i];
                                        Track *new_track = proj->tl->tracks[clip->track->tl_rank + move_by_tracks];
                                        remove_clip_from_track(clip);
                                        add_clip_to_track(clip, new_track);
                                        reset_cliprect(clip);
                                        clip_changed_track = true;
                                        clip->changed_track = true;
                                    }
        
                                }
                            }
                        }
                    }
                }
                if (clip_moved_x) {
                    mouse_p_click.x = mouse_p.x;
                }
                if (clip_changed_track) {
                    // reprocess_on_mouse_up = true;
                    dynamic_clicked_track_rank = mouse_track->tl_rank;
                    for (uint8_t i = 0; i<proj->tl->num_tracks; i++) {
                        Track *track = proj->tl->tracks[i];
                        for (uint8_t j = 0; j<track->num_clips; j++) {
                            Clip *clip = track->clips[j];
                            if (clip->changed_track) {
                                // process_clip_vol_and_pan(clip);
                                clip->changed_track = false;
                            }
                        }
                    }
                }
                if (!clicked_clip || (clicked_clip && !(clicked_clip->grabbed))) {
                    set_play_position(get_abs_tl_x(mouse_p.x));
                }
                // proj->tl->play_position = get_abs_tl_x(mouse_p.x);
                // set_timecode();
            }
        }
        if (proj->play_speed < 0 && proj->tl->display_offset_sframes > proj->tl->play_pos_sframes) {
            translate_tl(CATCHUP_STEP * -1, 0);
        } else if (proj->play_speed > 0 && get_tl_draw_x(proj->tl->play_pos_sframes) > proj->jwin->w) {
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
	    menulist_hover(proj->jwin, &mouse_p);
	    playback();
        draw_project(proj);
        draw_jwin_menus(proj->jwin);
        SDL_RenderPresent(proj->jwin->rend);
        SDL_Delay(animation_delay);
    }
}


static char *get_home_dir()
{
    uid_t uid = getuid();
    struct passwd *pw = getpwuid(uid);
    return pw->pw_dir;
}

int main(int argc, char **argv)
{
    char *file_to_open = NULL;
    bool invoke_open_wav_file = false;
    bool invoke_open_jdaw_file = false;
    if (argc > 2) {
        exit(1);
    } else if (argc == 2) {
        file_to_open = argv[1];
        int dotpos = strcspn(file_to_open, ".");
        char *ext = file_to_open + dotpos + 1;
        if (strcmp("wav", ext) * strcmp("WAV", ext) == 0) {
            fprintf(stderr, "Passed WAV file.\n");
            invoke_open_wav_file = true;
        } else if (strcmp("jdaw", ext) * strcmp("JDAW", ext) == 0) {
            fprintf(stderr, "Passed JDAW file.\n");
            invoke_open_jdaw_file = true;
        }
    }
    get_native_byte_order();
    // signal_init();
    home_dir = get_home_dir();

    //TODO: open project creation window if proj==NULL;
    init_graphics();
    init_audio();
    init_SDL_ttf();
    init_dsp();
    // JDAWWindow *new_project = create_jwin("Create a new Jackdaw project", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 600, 400);
    // new_project_loop(new_project);

    fprintf(stdout, "Opening project\n");

    if ((proj = open_jdaw_file("project.jdaw")) == NULL) {
        fprintf(stderr, "Creating new project\n");
        proj = create_project("Untitled", 2, 48000, AUDIO_S16SYS, 512);
        create_track(proj->tl, true);
    } else {
        fprintf(stderr, "Successfully opened project.\n");
    }
    fprintf(stdout, "Activating audio devices on project\n");

    fprintf(stderr, "Adding filter to track\n");
    add_filter_to_track(proj->tl->tracks[0], LOWPASS, 128);
    fprintf(stderr, "Setting filter params\n");
    set_FIR_filter_params(proj->tl->tracks[0]->filters[0], 0.04, 0);

    // fprintf(stderr, "Adding filter to track\n");
    // add_filter_to_track(proj->tl->tracks[0], BANDPASS, 128);
    // fprintf(stderr, "Setting filter params\n");
    // set_FIR_filter_params(proj->tl->tracks[0]->filters[1], 0.22, 0.01);


    // fprintf(stderr, "Adding filter to track\n");
    // add_filter_to_track(proj->tl->tracks[0], LOWPASS, 128);
    // fprintf(stderr, "Setting filter params\n");
    // set_FIR_filter_params(proj->tl->tracks[0]->filters[2], 0.009, 0);

    fprintf(stdout, "Loop starts now\n");
    if (invoke_open_wav_file) {
        load_wav_to_track(proj->tl->tracks[0], file_to_open);
    }
    project_loop();

    SDL_Quit();
}
