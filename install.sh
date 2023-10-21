#! /bin/bash
echo "\n\nJackdaw depends on the SDL2 and SDL2 TTF libraries."
echo "More information about SDL can be found at https://www.libsdl.org/"
echo "More information about SDL TTF can be found at https://github.com/libsdl-org/SDL_ttf"

if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    echo "Installing for GNU/Linux..."
    package_list=("libsdl2-ttf-2.0-0" "libsdl2-ttf-dev")
    path=$(eval echo "~/repos")
    echo "\nBy default, SDL2 source code will be cloned at $path. If this is ok, type "y". Otherwise, enter the desired path:"
    read conf
    if [[ $conf != y ]]; then
        conf=$(eval echo "$conf")
        while [ ! -d "$conf" ]
        do
            echo "There is no directory at $conf. Please enter a valid directory: "
            read conf
            conf=$(eval echo "$conf")
        done
        path=$conf
    fi
    if [[ ! -d "$path" ]]; then
        mkdir "$path"
    fi

    cd $path
    echo "Cloning source code repository from github..."
    git clone https://github.com/libsdl-org/SDL.git -b SDL2
    cd SDL
    mkdir build
    cd build
    ../configure
    make
    sudo make install

    echo "Done installing SDL2. Now installing SDL TTF..."

    sudo apt-get install libsdl2-ttf-2.0-0
    sudo apt-get install libsdl2-ttf-dev
elif [[ "$OSTYPE" == "darwin"* ]]; then
    echo "Installing for MacOS..."
    package_list=("sdl2" "sdl2_image" "sdl2_ttf")
    brew install sdl2
    brew install sdl2_ttf
else  echo "Operating system not recognized. Exiting."; exit;
fi

echo "\n\nDone installing dependencies. Building project..."
make
echo "Installing executable at /usr/local/bin/jackdaw..."
sudo mv jackdaw /usr/local/bin/jackdaw
echo "Done! Run the program by typing "jackdaw" on the command line."
