#!/bin/bash

set -e

(cd src && scons -j4 -Q noX)

output=./GrappleMap

echo "Creating $output/."

mkdir -p $output/{composer,search,explorer,editor}
mkdir -p $output/res/store

function download
{
	echo "Downloading $1"

	target=$output/$1
	url=$2

	if [ ! -f $target ]; then
		wget --no-verbose -O $target "${url}"
	fi
}

download babylon.js https://raw.githubusercontent.com/BabylonJS/Babylon.js/master/dist/preview%20release/babylon.js
download hand.js https://raw.githubusercontent.com/deltakosh/handjs/master/bin/hand.min.js
download sorttable.js http://www.kryogenix.org/code/browser/sorttable/sorttable.js

cp src/gm.js $output/
cp src/graphdisplay.{js,css} $output/
cp src/composer.html $output/composer/index.html
cp src/composer.js $output/composer/
cp src/search.html $output/index.html
cp src/search.js $output/
cp src/explorer.html $output/explorer/index.html
cp src/explorer.js $output/explorer/
cp src/example-drills.html $output/
cp src/editor.html $output/editor/index.html
cp src/editor.{css,js} $output/editor/
cp src/libgrapplemap.{data,js,js.mem} $output/
cp src/libgrapplemap.{data,js,js.mem} $output/editor/
cp src/libgrapplemap.{data,js,js.mem} $output/composer/
cp src/libgrapplemap.{data,js,js.mem} $output/explorer/
	# todo: copying this thrice is stupid but i dunno how to make the relative paths work otherwise

#echo "Converting database to javascript."
#src/grapplemap-dbtojs --output_dir=$output

echo -e "\nFor the position pages with pictures/gifs, run:\n"
echo -e "  src/grapplemap-mkpospages --output_dir=${output}\n"
