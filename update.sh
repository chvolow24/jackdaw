#!/bin/bash
echo "Getting latest files and updating application..."
git pull
make
echo -e "\n\nInstalling executable at /usr/local/bin/jackdaw..."
sudo mv jackdaw /usr/local/bin/jackdaw
echo -e "\n\nDone! Run the program by typing 'jackdaw' on the command line."
