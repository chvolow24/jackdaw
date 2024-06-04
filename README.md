# Jackdaw (WIP)
A stripped-down, keyboard-focused digital audio workstation (DAW) taking some design cues from non-linear video editors like Avid. Built on SDL (https://libsdl.org/).

![image](assets/readme_imgs/jdaw.png)

## Table of Contents
1. [Disclaimer](#disclaimer)
2. [Installation](#installation)
    1. [Compatibility](#compatibility)
	2. [Dependencies](#dependencies)
    3. [Bash scripts](#bash-scripts)
    4. [Manual installation](#manual-installation)
3. [Keyboard command syntax](#keyboard-command-syntax)
4. [Basics (getting comfortable)](#basics-getting-comfortable)
5. [Manual](#manual)

## Disclaimer

Jackdaw is very much a work in progress! What's currently available here is fun to use, can do a lot, and can definitely be used to make music; but I don't yet consider it to be "released." I do not guarantee that things will work perfectly or as you expect them.

## Installation

Currently, the only way to install Jackdaw is to build it from the source code. The first step is to clone the repository:

```console
$ git clone https://github.com/chvolow24/jackdaw.git

```

### Compatibility

Jackdaw is compatibile with macOS and Linux.

### Dependencies

Jackdaw is dependent on the [SDL2](https://libsdl.org/) library, and related [SDL2_ttf](https://wiki.libsdl.org/SDL2_ttf/FrontPage) library.

### Bash scripts

Two scripts are provided in this repository to make installation and updating easier: `install.sh` and `update.sh`. I don't want to actively encourage their use, because I don't want to encourage you to run bash scripts written by strangers on your machine. But I use them, and they're there if you want them.

Executing `install.sh` will attempt to install Jackdaw's dependencies ([SDL2](https://www.libsdl.org/) and [SDL2_ttf](https://wiki.libsdl.org/SDL2_ttf/FrontPage)) on your system. In order to do so, it will also install homebrew on macos if not already installed. It will then build the `jackdaw` executable, and move it to `/usr/local/bin`, so that you can run it from the command line in any directory.

`update.sh` pulls any available updates from the remote repository, re-executes itself*, and then moves the executable to `/usr/local/bin`.

*This is basically to make it so that you don't have to `git pull` in order to run the latest version of `update.sh`. But it would also be a great way for me to run malicious code on your computer! Proceed with caution (or trust). 

### Manual Installation

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

`n`......................................press the 'n' key<br>
`p`......................................press the 'p' key<br>
`C-s`...................................hold down 'command' OR 'ctrl', and press the 's' key<br>
`A-t`...................................hold down 'alt' OR 'option' and press the 't' key<br>
`C-S-o`...............................hold down 'command' OR 'ctrl', AND 'shift', and press the 'o' key<br>
`S-<ret>`...........................hold down 'shift' and press the 'return' or 'enter' key<br>

When a hyphen is present (as in `C-s`) it means you need to hold down one or more modifier key before striking the final key to invoke the command.

Capital `C` stands for the "Command" OR "Control" key. Jackdaw does not distinguish between these two keys.<br>
Capital `S` stands for the "Shift" key.<br>
Capital `A` stands for the "Alt" or "Option" key. (Again, no distinction is made)<br>
Capital `K` indicates that you must hold down the 'K' key, which is used as a modifier in very specific circumstances.

So, `C-S-o` means hold down the control (or command) key and the shift key, and then press 'o'.

Most of the final keys are named by a letter or number, except for these:<br>
`<ret>` means 'return' or 'enter'<br>
`<tab>` means 'tab'<br>
`<spc>` is the spacebar<br>
`<del>` is the 'delete' or 'backspace' key (no distinction)<br>
`<up>`, `<down>`, `<left>`, and `<right>` are the arrow keys<br>

This will all be familiar to emacs users, and hopefully not too painful for everyone else.


## Basics (getting comfortable)

The first thing you'll probably want to do upon opening jackdaw is to **add a track** or two. You can do this with<br>
<kbd>C-t</kbd>

(Hold down `cmd` or `ctrl` and hit `t`).
<!-- ![add_track.gif](assets/readme_imgs/add_track.gif) -->
<img src="assets/readme_imgs/add_track.gif" width="80%" />

The track input will be set to the default system audio input device, but you can **change the current track input** if you like with <kbd>C-S-i</kbd>. A list of available input devices will appear. Use <kbd>n</kbd> (for 'next') to go to the next item in the list, and <kbd>p</kbd> (for 'previous') to go to the previous. (These keys will be used a lot). Hit <kbd><ret></kbd> to choose the currently-highlighted device.

Once you have selected an appropriate audio input device, you can **start recording audio** into jackdaw with <kbd>r</kbd>. After making some noise at your computer or microphone, **stop recording** with <kbd>r</kbd>.

<img src="assets/readme_imgs/set_in_and_record.gif" width="80%" />

You should now see a clip on your timeline, with an audio waveform representing the audio you just recorded. You can rewind over the clip, pause, and play it back with the `j` (rewind) `k` (pause) and `l` (play) keys.
<br><br>
#### `J K L   :   (Rewind | Pause | Play)`
<br><br>
If you tap <kbd>l</kbd> or <kbd>j</kbd> multiple times, the playback speed will double.

The clip you recorded landed on the first track by default. You can again use the <kbd>n</kbd> and <kbd>p</kbd> keys to **move the track selector** up and down. Try rewinding back to the beginning of the recording you made, selecting a different track, and recording some new audio.

<!-- <img src="assets/readme_imgs/record_other_track.gif" width="80%"> -->
Now that you've created some multi-track audio, you might want to export it to a wav file to show your friends. First you'll need to place in and out marks on your timeline with <kbd>i</kbd> and <kbd>o</kbd>. Once you have placed them such that some portion of the timeline is marked, you can export to a wav file with <kbd>S-w</kbd>.

You will first be prompted to entire a file name. Hit <kbd>tab</kbd> or <kbd>`<ret>`</kbd> to apply the current name, and then <kbd>n</kbd> and <kbd>p</kbd> to navigate through the filesystem and pick the directory where you want to save the file. Finally, hit `C-s` to save the file.

<img src="assets/readme_imgs/export.gif" width="80%" />

Keyboard looks like this: <kbd>Ctrl</kbd>
or like this <kbd><kbd>Ctrl</kbd>+<kbd>c</kbd></kbd>

# Manual



...

[ README IN PROGRESS -- LAST UPDATE 2024-06-04 TUESDAY ]

...
