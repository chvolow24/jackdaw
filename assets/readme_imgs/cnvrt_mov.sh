#!/bin/bash

# Usage: convert_screenrecording.bash input.mov duration output.gif

input_file="$1"
output_file="$2"

# Set the width to 600 pixels
width=1000

duration=$(ffmpeg -i $1 2>&1 | grep "Duration" | awk '{print $2}' | tr -d , | awk -F: '{ print ($1 * 3600) + ($2 * 60) + $3 }')


echo "Converting $input_file to gif (at $output_file) with duration ${duration}s"

# Create a palette for better color quality
ffmpeg -v warning -i "$input_file" -vf "scale=$width:-1:flags=lanczos,palettegen" -t "$duration" palette.png

# Use the palette to create the GIF
ffmpeg -v warning -i "$input_file" -i palette.png -filter_complex "fps=20,scale=$width:-1:flags=lanczos[x];[x][1:v]paletteuse" -t "$duration" "$output_file"

# Clean up
rm palette.png
