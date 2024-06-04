#!/bin/bash
convert -delay 16 -loop 0 -resize 80% gifframes/*.bmp $1
mv $1 assets/readme_imgs
