#!/bin/bash
if [[ $1 != "recurse" ]]; then
    echo "Getting latest files and updating application..."
    git pull
    bash update.sh recurse
    exit 0
fi

echo -e "Building project..."
if [[ ! -d "build" ]]; then
    mkdir build
fi
cd gui
if [[ ! -d "build" ]]; then
    mkdir build
fi
cd ..

make clean
make

if [[ $? != 0 ]]; then
    echo -e "\nError building project.\n"
    exit 1
fi
echo -e "\nInstalling executable at /usr/local/bin/jackdaw..."
if [[ ! -d "/usr/local/bin" ]]; then
    sudo mkdir -p /usr/local/bin
    if [[ $? != 0 ]]; then
        echo -e "\nError creating /usr/local/bin directory.\n"
        exit 1
    fi
fi

sudo mv jackdaw /usr/local/bin

if [[ $? != 0 ]]; then
    echo -e "\nError moving the 'jackdaw' executable to /usr/local/bin.\n"
    exit 1
fi
if ! grep -q 'export PATH="/usr/local/bin:$PATH"' ~/.bashrc; then
    echo -e "Adding install dir to PATH in ~/.bashrc..."
    echo 'export PATH="/usr/local/bin:$PATH"' >> ~/.bashrc
fi
if [[ "$OSTYPE" == "darwin"* ]]; then
    if ! grep -q 'export PATH="/usr/local/bin:$PATH"' ~/.zshrc; then
        echo -e "Adding install dir to PATH in ~/.zshrc..."
        echo 'export PATH="/usr/local/bin:$PATH"' >> ~/.zshrc
    fi
fi
export PATH="/usr/local/bin:$PATH"

echo -e "\nDone! Run the program by typing 'jackdaw' on the command line in a bash or zsh terminal.\n"
