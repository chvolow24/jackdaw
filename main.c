/**************************************************************************************************
 * Jackdaw | a stripped-down, keyboard-focused Digital Audio Workstation
 **************************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>
#include <pthread.h>
#include "SDL2/SDL.h"
#include "SDL2/SDL_ttf.h"
#include "audio.h"
#include "theme.h"
#include "text.h"
#include "project.h"
#include "gui.h"
#include "wav.h"

#define abs_min(a,b) (abs(a) < abs(b) ? a : b)
#define lesser_of(a,b) (a < b ? a : b)
#define greater_of(a,b) (a > b ? a : b)

bool dark_mode = true;
Project *proj;

extern JDAW_Color bckgrnd_color;
extern JDAW_Color tl_bckgrnd;
extern JDAW_Color txt_soft;
extern JDAW_Color txt_main;

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

static void set_dpi_scale_factor() {
    int rw = 0, rh = 0, ww = 0, wh = 0;
    SDL_GetWindowSize(proj->win, &ww, &wh);
    SDL_GetRendererOutputSize(proj->rend, &rw, &rh);
    proj->scale_factor = (float)rw / (float)ww;
    if (proj->scale_factor != (float)rh / (float)wh) {
        fprintf(stderr, "Error! Scale factor w != h.\n");
    }
}

int main()
{
    // write_wav("TEST.wav");


    proj = create_project("Untitled", dark_mode);
    init_graphics();
    // reset_winrect();
    set_dpi_scale_factor();
    init_audio();
    init_SDL_ttf();
    init_fonts(proj->fonts, OPEN_SANS, 11);

    AudioDevice **playback_devices = NULL;
    AudioDevice **record_devices = NULL;
    int num_playback_devices = query_audio_devices(&playback_devices, 0);
    int num_record_devices = query_audio_devices(&record_devices, 1);
    for (int i=0; i<num_playback_devices; i++) {
        AudioDevice *dev = playback_devices[i];
        if (open_audio_device(dev, dev->spec.channels, 44100, 512) == 0) {
            fprintf(stderr, "Opened audio device: %s\n\tchannels: %d\n\tformat: %s\n", dev->name, dev->spec.channels, get_audio_fmt_str(dev->spec.format));

        } else {
            // fprintf(stderr, "Error: failed to open device %s\n", dev->name);
        }
    }
    for (int i=0; i<num_record_devices; i++) {
        AudioDevice *dev = record_devices[i];
        if (open_audio_device(dev, dev->spec.channels, 44100, 512) == 0) {
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

    int active_track = 0;
    bool quit = false;
    while (!quit) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                quit = true;
            } else if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                reset_winrect(proj);
                reset_tl_rect(proj->tl);
            } else if (e.type == SDL_KEYUP) {
                switch (e.key.keysym.scancode) {        
                    case SDL_SCANCODE_R:
                        if (!proj->recording) {
                            proj->recording = true;
                            proj->active_clip = create_clip((proj->tl->tracks)[active_track], 0, proj->tl->play_position - 512 * 10);
                            start_recording();
                            if (!proj->playing) {
                                proj->play_speed = 1; 
                                playback();
                            }

                        } else {
                            proj->recording = false;
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
                        if (proj->play_speed <= 0) {
                            proj->play_speed = 1;
                        } else if (proj->play_speed < 32) {
                            proj->play_speed *= 2;
                        }
                        break;
                    case SDL_SCANCODE_K:
                        printf("Pause\n");
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
                        } else if (proj->play_speed >= 0) {
                            proj->play_speed = -1;
                        } else if (proj->play_speed > -32) {
                            proj->play_speed *= 2;
                        }
                        break;
                    // case SDL_SCANCODE_SEMICOLON:
                    //     printf("Play slow\n");
                    //     if (proj->recording) {
                    //         proj->recording = false;
                    //         stop_recording(proj->active_clip);
                    //     }
                    //     if (proj->play_speed <= 0) {
                    //         proj->play_speed = 0.5;
                    //     } else if (proj->play_speed > 0.01) {
                    //         proj->play_speed *= 0.5;
                    //     }
                    //     break;
                    // case SDL_SCANCODE_H:
                    //     printf("Rewind slow\n");
                    //     if (proj->recording) {
                    //         proj->recording = false;
                    //         stop_recording(proj->active_clip);
                    //     }
                    //     if (proj->play_speed <= 0) {
                    //         proj->play_speed = -0.5;
                    //     } else if (proj->play_speed < -0.01) {
                    //         proj->play_speed *= 0.5;
                    //     }
                    //     break;
                    case SDL_SCANCODE_I:
                        // mark_in(play_head_start);
                        break;
                    case SDL_SCANCODE_O:
                        // mark_out(play_head_start);
                        break;
                    case SDL_SCANCODE_T:
                        proj->play_speed = 0;
                        create_track(proj->tl, false);
                        break;
                    case SDL_SCANCODE_1:
                        active_track = 0;
                        break;
                    case SDL_SCANCODE_2:
                        active_track = 1;
                        break;
                    case SDL_SCANCODE_3:
                        active_track = 2;
                        break;
                    case SDL_SCANCODE_4:
                        active_track = 3;
                        break;
                    case SDL_SCANCODE_5:
                        active_track = 4;
                        break;
                    case SDL_SCANCODE_6:
                        active_track = 5;
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
                    default:
                        break;
                }
            }
        }
        playback();
        // proj->tl->play_position += proj->play_speed * 441;

        draw_project(proj);
        SDL_Delay(1);
    }

    SDL_Quit();
}