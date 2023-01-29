Ubuntu
======

To install the tools and libraries necessary for GrappleMap development on a Ubuntu machine, run:

    scripts/apt-install-devtools.sh

To build the GLFW-based editor, do:

    cd src
    scons -j8 grapplemap-glfw-editor

Once the build completes, the editor can be run with e.g.:

    ./grapplemap-glfw-editor --db ../GrappleMap.txt p34

To run all scons build configurations requires emscripten. The apt-get version is not modern enough.

    Get the emsdk repo
        git clone https://github.com/emscripten-core/emsdk.git

    Enter that directory
        cd emsdk

    use emsdk to install/activate latest