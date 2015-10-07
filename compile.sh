#!/bin/sh
g++ -Wpedantic -Wall -Wextra -std=c++11 -O3 -DNDEBUG main.cpp -lglfw -lGL -lGLU -o jjm
g++ -Wpedantic -Wall -Wextra -std=c++11 -O3 -DNDEBUG todot.cpp -o jjm-todot
