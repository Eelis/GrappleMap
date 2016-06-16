#!/bin/bash

# to be run from images/store dir

set -ev

function find_used
{
	find .. -type l | xargs readlink | xargs -n1 basename
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
