#!/bin/bash

xcode-select --install 

brew install \
  scons \
  graphviz \
  pkg-config \
  graphviz \
  boost \
  boost-build \
  bjam \
  ftgl \
  glfw \
  mesa \
  ffmpeg \
  emscripten \
  mingw-w64 \
  glm \
  gsl \
  llvm

# seems sketch
sudo mkdir /opt/osmesa
sudo mkdir /opt/llvm

sudo chown -R $USER /opt/osmesa
sudo chown -R $USER /opt/llvm

# todo: check md5
export LLVM_BUILD=1
bash <(curl -s https://raw.githubusercontent.com/devernay/osmesa-install/master/osmesa-install.sh)



# libboost-filesystem-dev \
# libboost-regex-dev \
# libboost-program-options-dev \

# not found in brew, not sure if they are needed