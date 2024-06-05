# Jackdaw (WIP)
A stripped-down, keyboard-focused digital audio workstation (DAW) taking some design cues from non-linear video editors like Avid. Built on SDL (https://libsdl.org/).

![image](assets/readme_imgs/jdaw.png)

## Table of Contents
1. [Disclaimer](#disclaimer)
2. [Installation](#installation)
    1. [Compatibility](#os-compatibility)
	2. [Dependencies](#dependencies)
    3. [Bash scripts](#bash-scripts)
    4. [Manual installation](#manual-installation)
3. [Keyboard command syntax](#keyboard-command-syntax)
4. [Basics (getting comfortable)](#basics-getting-comfortable)
    1. [Adding tracks and setting input](#1-adding-tracks-and-setting-input)
    2. [Recording some audio](#2-recording-some-audio)
    3. [Playback](#3-playback)
    4. [Multi-track audio](#4-multi-track-audio)
    5. [Exporting a .wav file](#5-exporting-a-.wav-file)
    6. [Saving your project](#6-saving-your-project)
5. [User manual](#user-manual)
6. [Function reference](#function-reference)

## Disclaimer

Jackdaw is very much a work in progress! What's currently available here is fun to use, can do a lot, and can definitely be used to make music; but I don't yet consider it to be "released." I do not guarantee that things will work perfectly or as you expect them.

## Installation

Currently, the only way to install Jackdaw is to build it from the source code. The first step is to clone the repository:

```console
$ git clone https://github.com/chvolow24/jackdaw.git

```

### OS compatibility

Jackdaw is compatibile with macOS and Linux.

### Dependencies

Jackdaw is dependent on the [SDL2](https://libsdl.org/) library, and related [SDL2_ttf](https://wiki.libsdl.org/SDL2_ttf/FrontPage) library.

### Bash scripts

Two scripts are provided in this repository to make installation and updating easier: `install.sh` and `update.sh`. I don't want to actively encourage their use, because I don't want to encourage you to run bash scripts written by strangers on your machine. But I use them, and they're there if you want them.

Executing `install.sh` will attempt to install Jackdaw's dependencies ([SDL2](https://www.libsdl.org/) and [SDL2_ttf](https://wiki.libsdl.org/SDL2_ttf/FrontPage)) on your system. In order to do so, it will also install homebrew on macos if not already installed. It will then build the `jackdaw` executable, and move it to `/usr/local/bin`, so that you can run it from the command line in any directory.

`update.sh` pulls any available updates from the remote repository, re-executes itself*, and then moves the executable to `/usr/local/bin`.

*This is basically to make it so that you don't have to `git pull` in order to run the latest version of `update.sh`. But it would also be a great way for me to run malicious code on your computer! Proceed with caution (or trust). 

### Manual installation

If you don't want to use my scripts, you'll need to manually install the dependencies, build the project, and then do whatever you want with the executable.

#### Installing dependencies (SDL2 and SDL2_ttf)

Detailed instructions for installing SDL can be found [here](https://wiki.libsdl.org/SDL2/Installation). 

##### macOS

[Homebrew](https://brew.sh/) should work just fine: 
```console
$ brew install sdl2
$ brew install sdl2_ttf
```

##### linux

You may want to build SDL from source by default instead of using `apt-get`; I found that the version of SDL provided by `apt-get` on ubuntu was too old.

SDL2_ttf can be installed with the package manager, though:

```console
$ sudo apt-get install libsdl2-ttf-2.0-0
$ sudo apt-get install libsdl2-ttf-dev
```

#### Build the project

Navigate to the main jackdaw directory (where you cloned this repository) and run `make`:

```console
$ make
```

If there are errors, please feel free to create an issue on this repository; I'd be happy to look into it. 

#### Run the executable

If `make` executed successfully, there should be an executable named `jackdaw` in the current directory.

```console
$ ./jackdaw
```

## Keyboard command syntax

Jackdaw is a keyboard-based application; it uses the mouse and GUI buttons very sparingly. Here are some examples of keyboard commands you'll see written in the application and in this documentation:

<kbd>n</kbd>......................................press the 'n' key<br>
<kbd>p</kbd>......................................press the 'p' key<br>
<kbd>C-s</kbd>...................................hold down 'command' OR 'ctrl', and press the 's' key<br>
<kbd>A-t</kbd>....................................hold down 'alt' OR 'option' and press the 't' key<br>
<kbd>C-S-o</kbd>.................................hold down 'command' OR 'ctrl', AND 'shift', and press the 'o' key<br>
<kbd>S-\<ret\></kbd>..............................hold down 'shift' and press the 'return' or 'enter' key<br>

When a hyphen is present (as in <kbd>C-s</kbd>) it means you need to hold down one or more modifier key before striking the final key to invoke the command.

Capital <kbd>C</kbd> stands for the "Command" OR "Control" key. Jackdaw does not distinguish between these two keys.<br>
Capital <kbd>S</kbd> stands for the "Shift" key.<br>
Capital <kbd>A</kbd> stands for the "Alt" or "Option" key. (Again, no distinction is made)<br>
Capital <kbd>K</kbd> indicates that you must hold down the 'K' key, which is used as a modifier in very specific circumstances.

So, <kbd>C-S-o</kbd> means hold down the control (or command) key and the shift key, and then press 'o'.

Most of the final keys are named by a letter or number, except for these:<br>
<kbd>\<ret\></kbd> means 'return' or 'enter'<br>
<kbd>\<tab\></kbd> means 'tab'<br>
<kbd>\<spc\></kbd> is the spacebar<br>
<kbd>\<del\></kbd> is the 'delete' or 'backspace' key (no distinction)<br>
<kbd>\<up\></kbd>, <kbd>\<down\></kbd>, <kbd>\<left\></kbd>, and <kbd>\<right\></kbd> are the arrow keys<br>

This will all be familiar to emacs users, and hopefully not too painful for everyone else.


## Basics (getting comfortable)

### 1. Adding tracks and setting input

The first thing you'll probably want to do upon opening jackdaw is to **add a track** or two. You can do this with<br>
<kbd>C-t</kbd>

(Hold down <kbd>cmd</kbd> or <kbd>ctrl</kbd> and hit <kbd>t</kbd>).
<!-- ![add_track.gif](assets/readme_imgs/add_track.gif) -->
<img src="assets/readme_imgs/add_track.gif" width="80%" />

The track input will be set to the default system audio input device, but you can **change the current track input** if you like with <kbd>C-S-i</kbd>. A list of available input devices will appear. Use <kbd>n</kbd> (for 'next') to go to the next item in the list, and <kbd>p</kbd> (for 'previous') to go to the previous. (These keys will be used a lot). Hit <kbd>\<ret\></kbd> to choose the currently-highlighted device.

### 2. Recording some audio

Once you have selected an appropriate audio input device, you can **start recording audio** into jackdaw with <kbd>r</kbd>. After making some noise at your computer or microphone, **stop recording** with <kbd>r</kbd>.

<img src="assets/readme_imgs/set_in_and_record.gif" width="80%" />


### 3. Playback

You should now see a clip on your timeline, with an audio waveform representing the audio you just recorded. You can rewind over the clip, pause, and play it back with the <kbd>j</kbd> (rewind) <kbd>k</kbd> (pause) and <kbd>l</kbd> (play) keys.
<br><br>
#### J K L : (Rewind | Pause | Play)
<br><br>
If you tap <kbd>l</kbd> or <kbd>j</kbd> multiple times, the playback speed will double.


### 4. Multi-track audio

The clip you recorded landed on the first track by default. You can again use the <kbd>n</kbd> and <kbd>p</kbd> keys to **move the track selector** up and down. Try rewinding back to the beginning of the recording you made, selecting a different track, and recording some new audio.

### 5. Exporting a .wav file

Now that you've created some multi-track audio, you might want to export it to a wav file to show your friends. First you'll need to place in and out marks on your timeline with <kbd>i</kbd> and <kbd>o</kbd>. Once you have placed them such that some portion of the timeline is marked, you can export to a wav file with <kbd>S-w</kbd>.

You will first be prompted to entire a file name. Hit <kbd>tab</kbd> or <kbd>\<ret\></kbd> to apply the current name, and move down to the directory navigation pane. Then, use <kbd>n</kbd> and <kbd>p</kbd> to navigate through the filesystem to the directory where you want to save the file. Finally, use <kbd>C-\<ret\></kbd> to "submit the form" and save the file.

<img src="assets/readme_imgs/export.gif" width="80%" />

### 6. Saving your project

If you want to revisit this project later, you can save a project file (`.jdaw`) with `C-s`.

You will be prompted to enter a project name (which MUST include the `.jdaw` extension), and can then hit <kbd>\<ret\></kbd> or <kbd>\<tab\></kbd> to move down to the directory navigation pane. Navigate to the location at which you want to save the project (with <kbd>n</kbd> and <kbd>p</kbd>), and submit the form with <kbd>C-\<ret\></kbd> to complete saving.

# User manual

## Menus

**At any* time,** (*except when editing a text field), you can summon a menu with a list of available actions (and keyboard shortcuts) with <kbd>C-m</kbd>. The menu will display the functions available in the current mode (see "Input modes" below). Navigate with <kbd>n</kbd>, <kbd>p</kbd>, and <kbd>/<ret/></kbd> to select. 

Summoning these menus when in doubt might be the best way to learn the available keyboard shortcuts. 

## Timeline navigation

Timeline navigation functions are all within easy reach of your right hand:

### Transport
<kbd>j</kbd> : **rewind** (multiple taps to rewind fast)<br>
<kbd>k</kbd> : **pause**<br>
<kbd>l</kbd> : **play** (multiple taps to play fast)<br>

### Translate / zoom
<kbd>h</kbd> : **move view left**<br>
<kbd>;</kbd> : **move view right**<br>
<kbd>,</kbd> : **zoom out**<br>
<kbd>.</kbd> : **zoom in**<br>

These zoom functions will center on the current playhead position. Another way to zoom is to hold <kbd>Cmd</kbd> or <kbd>Ctrl</kbd> and scroll up or down while the mouse is over the timeline. In this case, zoom will center on the mouse position. 

### Track selector

The *track selector* is how you indicate which track your are doing things to. The track console (left side) is highlighted in orange if the track is currently selected.

<kbd>n</kbd> : **move selector down** (next track)<br>
<kbd>p</kbd> : **move selector up** (previous track)<br>
<kbd>\<ret\></kbd> : **activate/deactivate current track**<br>

**Activating** tracks (as opposed to merely *selecting* them) is a way to do things that might normally be done to only one track to multiple tracks at once. For example, if you activate tracks 1, 2, and 3, and then hit <kbd>r</kbd> to record, you will wind up with *three* clips (more accurately, clip references) -- one on each track -- instead of one. The numerical keys (<kbd>1</kbd>, <kbd>2</kbd>, <kbd>3</kbd> etc.) can be used to quickly activate or deactivate tracks 1-9 without using the track selector.

### "Point"

The "point" (as in "[do thing] at point" is the location in the currently active timeline, under the current playhead position, on the currently selected track. So, *"grab clip at point"* means "grab the top clip on the currently selected track that intersects with the current playhead position."

### Marks

In and out marks must be set to export a mixdown to .wav, and can also be used as handy jump-to points.

You can also jump to the start of the timeline (t=00:00:00000) with <kbd>S-u</kbd>, which can be helpful if you get lost. 

<kbd>i</kbd> : **mark in**
<kbd>o</kbd> : **mark out**
<kbd>S-i</kbd> : **jump to in mark**
<kbd>S-o</kbd> : **jump to out mark**
<kbd>S-u</kbd> : **jump to t=0**

### Scrolling

When you move the track selector with <kbd>n</kbd> and <kbd>p</kbd>, the timeline will automatically refocus such that the selected track is visible. You can also scroll through tracks with a mouse or trackpad. Scrolling horizontally translates the timeline in the manner of <kbd>h</kbd> and <kbd>;</kbd>.

Holding <kbd>Cmd</kbd> or <kbd>Ctrl</kbd> and scrolling on the timeline will zoom in or out.


# Function reference

### global mode
- Expose help : <kbd>C-h</kbd>
- Quit : <kbd>C-q</kbd>
- Undo : <kbd>C-z</kbd>
- Redo : <kbd>C-y</kbd>
- Save Project : <kbd>C-s</kbd>
- Open File (.wav or .jdaw) : <kbd>C-o</kbd>
- Start or stop screenrecording : <kbd>A-S-p</kbd>
### menu_nav mode
- Next item : <kbd>n</kbd>, <kbd>f</kbd>, <kbd>\<up\></kbd>, <kbd>k</kbd>
- Previous item : <kbd>p</kbd>, <kbd>d</kbd>, <kbd>\<down\></kbd>, <kbd>i</kbd>
- Next section : <kbd>C-n</kbd>, <kbd>C-\<up\></kbd>
- Previous section : <kbd>C-p</kbd>, <kbd>C-\<down\></kbd>
- Choose item : <kbd>\<ret\></kbd>, <kbd>\<spc\></kbd>
- Column right : <kbd>l</kbd>, <kbd>\<right\></kbd>
- Column left : <kbd>j</kbd>, <kbd>\<left\></kbd>
- Move menu up : <kbd>w</kbd>
- Move menu down : <kbd>s</kbd>
- Move menu right : <kbd></kbd>
- go back (dismiss) : <kbd>h</kbd>
### timeline mode
#### Transport
- Play : <kbd>l</kbd>, <kbd>e</kbd>
- Pause : <kbd>k</kbd>, <kbd>w</kbd>, <kbd>S-k</kbd>
- Rewind : <kbd>j</kbd>, <kbd>q</kbd>
- Play slow : <kbd>K-l</kbd>, <kbd>S-l</kbd>
- Rewind slow : <kbd>K-j</kbd>, <kbd>S-j</kbd>
- Start or stop dragging clips : <kbd>C-k</kbd>
- Cut clipref at point : <kbd>S-c</kbd>
- Move right : <kbd>;</kbd>
- Move left : <kbd>h</kbd>
- Zoom out : <kbd>,</kbd>
- Zoom in : <kbd>.</kbd>
- Record (start or stop) : <kbd>r</kbd>
#### Marks
- Set In : <kbd>i</kbd>
- Set Out : <kbd>o</kbd>
- Go to In : <kbd>S-i</kbd>
- Go to Out : <kbd>S-o</kbd>
- Go to t=0 : <kbd>S-u</kbd>
- Set default audio output : <kbd>C-S-o</kbd>
#### Track nav
- Add Track : <kbd>C-t</kbd>
- Add Track : <kbd></kbd>
- Select track 1 : <kbd>1</kbd>
- Select track 2 : <kbd>2</kbd>
- Select track 3 : <kbd>3</kbd>
- Select track 4 : <kbd>4</kbd>
- Select track 5 : <kbd>5</kbd>
- Select track 6 : <kbd>6</kbd>
- Select track 7 : <kbd>7</kbd>
- Select track 8 : <kbd>8</kbd>
- Select track 9 : <kbd>9</kbd>
- Activate or deactivate all tracks : <kbd>`</kbd>
- Move track selector up : <kbd>p</kbd>, <kbd>d</kbd>
- Move track selector down : <kbd>n</kbd>, <kbd>f</kbd>
- Activate selected track : <kbd>\<spc\></kbd>, <kbd>\<ret\></kbd>
- Destroy selected track (permanent) : <kbd>C-\<del\></kbd>
#### Track settings
- Mute or unmute selected track(s) : <kbd>m</kbd>
- Solo or unsolo selected track(s) : <kbd>s</kbd>
- Track volume up : <kbd>S-=</kbd>
- Track volume down : <kbd>S--</kbd>
- Track pan left : <kbd>S-9</kbd>
- Track pan right : <kbd>S-0</kbd>
- Rename selected track : <kbd>C-r</kbd>
- Set track input : <kbd>C-S-i</kbd>
#### Clips
- Grab clip at point : <kbd>g</kbd>
- Load clip at point to source : <kbd>C-1</kbd>
- Activate Source Mode : <kbd>S-1</kbd>
- Drop clip from source : <kbd>b</kbd>
- Drop previously dropped clip (1) : <kbd>v</kbd>
- Drop previously dropped clip (2) : <kbd>c</kbd>
- Drop previously dropped clip (3) : <kbd>x</kbd>
#### Timeline navigation
- Add new timeline : <kbd>A-t</kbd>
- Previous timeline : <kbd>A-j</kbd>
- Next timeline : <kbd>A-l</kbd>
- Write mixdown to .wav file : <kbd>S-w</kbd>
- Delete selected clip(s) : <kbd>\<del\></kbd>
### source mode
- Play (source) : <kbd>l</kbd>
- Pause (source) : <kbd>k</kbd>, <kbd>S-k</kbd>
- Rewind (source) : <kbd>j</kbd>
- Play slow (source) : <kbd>S-l</kbd>, <kbd>K-l</kbd>
- Rewind slow (source : <kbd>S-j</kbd>, <kbd>K-j</kbd>
- Set In Mark (source) : <kbd>i</kbd>, <kbd>S-i</kbd>
- Set Out Mark (source) : <kbd>o</kbd>, <kbd>S-o</kbd>
### modal mode
- Go to next item : <kbd>n</kbd>, <kbd>f</kbd>
- Go to previous item : <kbd>p</kbd>, <kbd>d</kbd>
- Go to next item (escape DirNav) : <kbd>S-n</kbd>, <kbd>S-f</kbd>
- Go to previous item (escape DirNav) : <kbd>S-p</kbd>, <kbd>S-d</kbd>
- Select item : <kbd>\<ret\></kbd>, <kbd>\<spc\></kbd>
- Dismiss modal window : <kbd>h</kbd>
- Submit form : <kbd>C-\<ret\></kbd>

...

[ README IN PROGRESS -- LAST UPDATE 2024-06-04 TUESDAY ]

...
