#!/bin/sh
set -ev
rm -rf vidframes && mkdir vidframes
src/grapplemap-mkvid --frames-per-pos 9 --start "neutral standing" --length 380 --dimensions 864x486
avconv -framerate 50 -i 'vidframes/frame%05d.png' -vcodec libx264 -pix_fmt yuv420p  random.mp4
