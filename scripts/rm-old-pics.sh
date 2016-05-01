#!/bin/bash

set -ev

function find_used
{
	cat *.html | grep -o -P "(?<=src=')[a-zA-Z0-9-]*?.(gif|png)(?=')"
	find . -type l | xargs readlink
}

function find_present
{
	find -name "*.gif" -not -type l -printf '%P\n'
	find -name "*.png" -not -type l -printf '%P\n'
}

find_used | sort | uniq > used
find_present | sort | uniq > present

wc -l used present

#comm -2 -3 present used | xargs rm
#rm used present
