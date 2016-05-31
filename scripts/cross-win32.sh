#!/bin/bash
set -ev

(cd src && scons -f SConstruct.cross-win32)
strip src/*.exe

packageName=`git show --no-patch "--format=GrappleMap_%cd_%h_win32" --date=short HEAD`

rm -rf win32package
mkdir -p win32package/$packageName/{composer,explorer}
cd win32package

cp ../src/grapplemap-{editor,playback,dbtojs}.exe ../GrappleMap.txt $packageName/
cp ../src/{gm,graphdisplay}.js $packageName/
cp ../src/composer.html $packageName/composer/index.html
cp ../src/composer.js $packageName/composer/
cp ../src/explorer.html $packageName/explorer/index.html
cp ../src/explorer.js $packageName/explorer/
cp ../doc/windows-package-readme.txt $packageName/README.TXT

function download
{
	target=$packageName/$1
	url=$2

	if [ ! -f $target ]; then
		wget --no-verbose -O $target "${url}"
	fi
}

download babylon.js https://raw.githubusercontent.com/BabylonJS/Babylon.js/master/dist/preview%20release/babylon.js
download hand.js https://raw.githubusercontent.com/deltakosh/handjs/master/bin/hand.min.js

zip $packageName $packageName/* $packageName/{composer,explorer}/*
