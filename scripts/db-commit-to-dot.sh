#!/bin/bash

set -e

commit=$1

src/grapplemap-diff <(git show "${commit}^:GrappleMap.txt") <(git show "${commit}:GrappleMap.txt") | dot -Tpng -o /tmp/t.png
gwenview /tmp/t.png
