#!/bin/sh
set -ev
rm -rf vidframes && mkdir vidframes
src/grapplemap-mkvid --frames-per-pos 11 --start "neutral standing" --length 100 --dimensions 640x480
avconv -framerate 60 -i 'vidframes/frame%05d.png' -vcodec libx264 -pix_fmt yuv420p  random.mp4
