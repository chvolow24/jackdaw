#! /bin/bash
install_dir=$(pwd)
echo -e "\n\nJackdaw depends on the SDL2 and SDL2 TTF libraries."
echo "More information about SDL can be found at https://www.libsdl.org/"
echo "More information about SDL TTF can be found at https://github.com/libsdl-org/SDL_ttf"

if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    echo -e "Installing for GNU/Linux..."
    path=$(eval echo "~/repos")
    echo -e "\nBy default, SDL2 source code will be cloned at $path. If this is ok, type "y". Otherwise, enter the desired path:"
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
    brew install sdl2
    brew install sdl2_ttf
else  echo "Operating system not recognized. Exiting."; exit;
fi

echo -e "\n\nDone installing dependencies. Building project..."
cd $install_dir
if [[ ! -d "build" ]]; then
    mkdir build
fi
make
echo -e "\n\nInstalling executable at /usr/local/bin/jackdaw..."
if [[ ! -d "/usr/local/bin" ]]; then
    sudo mkdir -p /usr/local/bin
fi
sudo mv jackdaw /usr/local/bin

echo -e "Adding install dir to PATH in ~/.bashrc..."
echo 'export PATH="/usr/local/bin:$PATH"' >> ~/.bashrc
if [[ "$OSTYPE" == "darwin"* ]]; then
    echo -e "Adding install dir to PATH in ~/.zshrc..."

    echo 'export PATH="/usr/local/bin:$PATH"' >> ~/.zshrc
fi
export PATH="/usr/local/bin:$PATH"
echo -e "\n\nDone! Run the program by typing 'jackdaw' on the command line in a bash or zsh terminal."
