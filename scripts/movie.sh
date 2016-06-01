#!/bin/sh
set -ev
rm -rf vidframes && mkdir vidframes
src/grapplemap-mkvid --frames-per-pos 12 --start "symmetric staggered standing" --length 400 --dimensions 1280x720
avconv -framerate 60 -i 'vidframes/frame%05d.png' -vcodec libx264 -pix_fmt yuv420p  random.mp4
