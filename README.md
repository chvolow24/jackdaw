# Jackdaw (WIP)
#### A stripped-down, keyboard-focused digital audio workstation (DAW) taking some design cues from non-linear video editors like Avid. Built on SDL (https://libsdl.org/).

![image](assets/readme_imgs/jdaw.png)

## Table of Contents
1. [Disclaimer](#disclaimer)
2. [Intallation](#installation)
    1. [Compatibility](#compatibility)
    2. [Bash scripts](#bash-scripts)
    3. [Manual installation](#manual-installation)
3. [Getting started](#getting-started)

## Disclaimer

Jackdaw is very much a work in progress! What's currently available here is fun to use, can do a lot, and can definitely be used to make music; but I don't yet consider it to be "released." I do not guarantee that things will work perfectly or as you expect them.

## Installation

Currently, the only way to install Jackdaw is to build it from the source code. The first step is to clone the repository:

```console
$ git clone https://github.com/chvolow24/jackdaw.git

```

### Compatibiilty

Jackdaw is compatibile with macOS and Linux.

### Dependency

Jackdaw is dependent on the [SDL2](https://libsdl.org/) library, and related [SDL2_ttf](https://wiki.libsdl.org/SDL2_ttf/FrontPage) library.

### Bash scripts

Two scripts are provided in this repository to make installation and updating easier: `install.sh` and `update.sh`. I don't want to actively encourage their use, because I don't want to encourage you to run bash scripts written by strangers on your machine. But I use them, and they're there if you want them.

Executing `install.sh` will attempt to install Jackdaw's dependencies ([SDL2](https://www.libsdl.org/) and [SDL2_ttf](https://wiki.libsdl.org/SDL2_ttf/FrontPage)) on your system. It will then build the `jackdaw` executable, and move it to `/usr/local/bin`, so that you can run it from the command line in any directory.

`update.sh` pulls any avilable updates from the remote repository, re-executes itself*, and then moves the executable to `/usr/local/bin`.

*This is basically to make it so that you don't have to `git pull` yourself, and so that any updates made to `update.sh` remotely are reflected when you run it locally. But it would also be a great way for me to run malicious code on your computer! Please be careful!

### Manual Installation

If you don't want to use my scripts, you'll need to manually install the dependencies, build the project, and then do whatever you want with the executable.

#### Installing dependencies (SDL2 and SDL2_ttf)

Detailed instructions for installing SDL can be found [here](https://wiki.libsdl.org/SDL2/Installation). 

#### macOS

[Homebrew](https://brew.sh/) should work just fine: 
```console
$ brew install sdl2
$ brew install sdl2_ttf
```

#### linux

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

## Getting started

...

[ README IN PROGRESS -- LAST UPDATE 2024-05-30 THURSDAY ]

...
