#!/bin/sh
set -ev
(cd src && scons -Q grapplemap-mkvid)
rm -rf vidframes && mkdir vidframes
src/grapplemap-mkvid --frames-per-pos 12 --length 438 --dimensions 1280x720
avconv -framerate 60 -i 'vidframes/frame%05d.png' -vcodec libx264 -pix_fmt yuv420p  random.mp4
