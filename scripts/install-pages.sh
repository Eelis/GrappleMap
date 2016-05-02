#!/bin/bash

set -ev

(cd src && scons)

outputbase=.

output=$outputbase/GrappleMap

mkdir -p $output/{gifframes,composer,search}

function download
{
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
cp src/composer.html $output/composer/index.html
cp src/composer.js $output/composer/
cp src/search.html $output/search/index.html
cp src/search.js $output/search/

src/grapplemap-browse --output_dir=$outputbase
