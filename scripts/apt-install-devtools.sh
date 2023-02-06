#!/bin/bash
xcode-select –install 

apt-get install --no-install-recommends -yqq \
  g++ \
  sconbrew install sconss \
  pkg-config \
  libgraphviz-dev \
  libboost-dev \
  libboost-filesystem-dev \
  libboost-regex-dev \
  libboost-program-options-dev \
  libftgl-dev \
  libxine2-dev \
  libglfw3-dev \
  libosmesa6-dev \
  ffmpeg
