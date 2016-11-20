#!/bin/sh
set -ev
(cd src && scons -Q grapplemap-mkvid)
rm -rf vidframes && mkdir vidframes
src/grapplemap-mkvid --length 600
avconv -framerate 60 -i 'vidframes/frame%05d.png' -vcodec libx264 -pix_fmt yuv420p random.mp4
# ffmpeg -framerate 60 -i 'vidframes/frame%05d.png' -c:v libx264 -r 60 -pix_fmt yuv420p random.mp4
