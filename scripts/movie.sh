#!/bin/sh
set -ev
dims=200x150
glc-capture --disable-audio --lock-fps --out=t.glc --crop=$dims --start \
 src/grapplemap-playback --db GrappleMap.txt --frames-per-pos 7 --start 0 --length 73 --dimensions $dims
rm -f capture.mkv
glc-play t.glc -y 1 -o t.yuv
ffmpeg -i t.yuv -preset slow -b:v 1500k -pix_fmt yuv420p -vcodec libx264 random.mp4
#rm -f t.glc t.yuv
