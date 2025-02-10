#!/bin/bash

input_file="$1"
output_file="$2"

width=1000
fps=20

if [ $3 ]; then
    if [ $3 == "zulip" ]; then
	echo "exporting small gif for zulip upload"
	width=800
	fps=8
    fi
fi

duration=$(ffmpeg -i $1 2>&1 | grep "Duration" | awk '{print $2}' | tr -d , | awk -F: '{ print ($1 * 3600) + ($2 * 60) + $3 }')

echo "Converting $input_file to gif (at $output_file) with duration ${duration}s"

# Create a palette for better color quality
ffmpeg -v warning -i "$input_file" -vf "scale=$width:-1:flags=lanczos,format=yuv420p,palettegen" -t "$duration" -frames:v 1 palette.png

# Use the palette to create the GIF
ffmpeg -v warning -i "$input_file" -i palette.png -filter_complex "fps=$fps,scale=$width:-1:flags=lanczos,format=yuv420p[x];[x][1:v]paletteuse" -t "$duration" "$output_file"

# Clean up
rm palette.png
