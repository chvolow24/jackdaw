#!/bin/bash
echo "Getting latest files and updating application..."
git pull
echo -e "Building project..."
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
