#!/bin/bash

set -e

commit=$1

git show "${commit}:GrappleMap.txt" > /tmp/new.grapplemapdb
git show "${commit}^:GrappleMap.txt" > /tmp/old.grapplemapdb

src/grapplemap-diff /tmp/old.grapplemapdb /tmp/new.grapplemapdb | dot -Tpng -o /tmp/t.png
gwenview /tmp/t.png
