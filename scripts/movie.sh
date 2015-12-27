#!/bin/sh
set -ev
glc-capture --disable-audio --lock-fps --out=t.glc --start \
 src/grapplemap-playback --db GrappleMap.txt --frames-per-pos 8 --start "distance standing\\nvs seated" --length 50
rm -f capture.mkv
glc-play t.glc -y 1 -o t.yuv
ffmpeg -i t.yuv -preset veryslow -b:v 3000k -vcodec libx264 capture.mkv
#rm -f t.glc t.yuv
