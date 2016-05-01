#!/bin/sh
set -ev
dims=640x480
glc-capture --disable-audio --lock-fps --out=t.glc --crop=$dims --start \
 src/grapplemap-playback --frames-per-pos 4 --start "neutral standing" --length 227 --dimensions $dims
rm -f capture.mkv
glc-play t.glc -y 1 -o t.yuv
ffmpeg -i t.yuv -preset slow -b:v 1500k -pix_fmt yuv420p -vcodec libx264 random.mp4
#rm -f t.glc t.yuv
