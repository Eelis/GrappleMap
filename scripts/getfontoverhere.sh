#!/bin/bash
cp `fc-list ":fullname=DejaVu Sans" file | cut -d ':' -f 1` .
# todo: do this elsewhere
