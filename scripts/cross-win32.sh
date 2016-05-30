#!/bin/sh
set -ev
(cd src && scons -f SConstruct.cross-win32)
strip src/*.exe
packageName=`git show --no-patch "--format=GrappleMap_%cd_%h_win32" --date=short HEAD`
rm -rf win32package
mkdir -p win32package/$packageName
cd win32package
cp ../src/grapplemap-{editor,playback}.exe ../GrappleMap.txt $packageName/
zip $packageName $packageName/*
