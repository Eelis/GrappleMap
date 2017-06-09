#!/bin/bash

# don't forget to source emsdk_env.sh first

emcc web_editor.cpp viables.cpp editor.cpp playerdrawer.cpp rendering.cpp playback.cpp metadata.cpp persistence.cpp graph.cpp paths.cpp positions.cpp graph_util.cpp icosphere.cpp --bind --preload-file triangle.vertexshader --preload-file triangle.fragmentshader --preload-file GrappleMap.txt -std=c++14 -I /home/eelis/include -O3 -DNDEBUG -s USE_WEBGL2=1 -s FULL_ES3=1 -s USE_GLFW=3 -s TOTAL_MEMORY=67108864 -s EXPORTED_FUNCTIONS="['_main']" -o grapplemap_editor.js
