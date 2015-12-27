#!/bin/sh
set -ev
glc-capture --disable-audio --lock-fps --out=t.glc --start \
 src/grapplemap-playback --db GrappleMap.txt --frames-per-pos 7 --start "standing\\nvs seated" --length 60
rm -f capture.mkv
glc-play t.glc -y 1 -o t.yuv
ffmpeg -i t.yuv -preset slow -b:v 1500k -pix_fmt yuv420p -vcodec libx264 random.mp4
#rm -f t.glc t.yuv
