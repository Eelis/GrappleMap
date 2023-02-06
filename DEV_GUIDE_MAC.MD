# installation 
1. run `./scripts/mac-brew-install-devtools.sh`

# Building 
2. cd src && scons -f SConstruct.cross-win32
scons: Reading SConscript files ...
scons: done reading SConscript files.
scons: Building targets ...
em++ -o cursor_canvas.web.o -c -Wall -Wextra -Wno-dollar-in-identifier-extension -Wno-vla-extension -pedantic -std=c++1y -DNDEBUG -O3 -s TOTAL_MEMORY=134217728 -s USE_WEBGL2=1 -s FULL_ES3=1 -s USE_GLFW=3 cursor_canvas.cpp
sh: em++: command not found
scons: *** [cursor_canvas.web.o] Error 127
scons: building terminated because of errors.

# Errors 
## fatal error: 'boost/optional.hpp' file not found
`cd src  && scons -f SConstruct && cd ../`

cache:INFO:  - ok
In file included from cursor_canvas.cpp:1:
In file included from ./editor_canvas.hpp:7:
In file included from ./editor.hpp:5:
In file included from ./persistence.hpp:4:
In file included from ./paths.hpp:4:
In file included from ./graph_util.hpp:4:
In file included from ./graph.hpp:4:
In file included from ./reoriented.hpp:4:
In file included from ./positions.hpp:5:
In file included from ./players.hpp:4:
In file included from ./indexed.hpp:4:
./util.hpp:10:10: fatal error: 'boost/optional.hpp' file not found
#include <boost/optional.hpp>
         ^~~~~~~~~~~~~~~~~~~~
1 error generated.
em++: error: '/usr/local/Cellar/emscripten/3.1.31/libexec/llvm/bin/clang++ -target wasm32-unknown-emscripten -fignore-exceptions -fvisibility=default -mllvm -combiner-global-alias-analysis=false -mllvm -enable-emscripten-sjlj -mllvm -disable-lsr -DEMSCRIPTEN --sysroot=/usr/local/Cellar/emscripten/3.1.31/libexec/cache/sysroot -Xclang -iwithsysroot/include/fakesdl -Xclang -iwithsysroot/include/compat -c -Wall -Wextra -Wno-dollar-in-identifier-extension -Wno-vla-extension -pedantic -std=c++1y -DNDEBUG -O3 cursor_canvas.cpp -o cursor_canvas.web.o' failed (returned 1)

### Solution
add to compiler flag -I /usr/local/include
created a new SConstruct file `./src/SConstruct.mac` that includes this, should find a way to add conditionally if this is the only change 

## fatal error: 'GL/glu.h' file not found
This happened in a handful of files. The OpenGl header for mac is `#include <OpenGL/glu.h>` not `#include <GL/glu.h>`

### solution 
Conditionally require the correct header based on __APPLE__ macro being present 

```
#if defined(__APPLE__)
#include <OpenGL/glu.h>
#else
#include <GL/glu.h>
#endif
```

Updated in the following files: 
 - ./src/glfw_editor.cpp
 - ./src/glfw_playback.cpp
 - ./src/playback.cpp
 - ./src/playerdrawer.cpp
 - ./src/rendering.cpp


## library not found for -lGLU
scons: done reading SConscript files.
scons: Building targets ...
g++ -o grapplemap-glfw-editor glfw_editor.o rendering.o playerdrawer.o graph.o graph_util.o positions.o viables.o persistence.o paths.o playback.o icosphere.o editor.o metadata.o md5.o js_conversions.o -L/usr/local/Cellar/graphviz/7.1.0/lib -L/usr/local/opt/freetype/lib -lGL -lGLU -lglfw -lftgl -lboost_program_options -lboost_regex
ld: library not found for -lGLU
clang: error: linker command failed with exit code 1 (use -v to see invocation)
scons: *** [grapplemap-glfw-editor] Error 1
scons: building terminated because of errors.
scons: Reading SConscript files ...
scons: done reading SConscript files.
scons: Building targets ...
g++ -o grapplemap-glfw-editor glfw_editor.o rendering.o playerdrawer.o graph.o graph_util.o positions.o viables.o persistence.o paths.o playback.o icosphere.o editor.o metadata.o md5.o js_conversions.o -L/usr/local/Cellar/graphviz/7.1.0/lib -L/usr/local/opt/freetype/lib -lGL -lGLU -lglfw -lftgl -lboost_program_options -lboost_regex
ld: library not found for -lGLU
clang: error: linker command failed with exit code 1 (use -v to see invocation)
scons: *** [grapplemap-glfw-editor] Error 1
scons: building terminated because of errors.

### Solution 
added a set of compiler flags that use framework for mac based on this stack overflow 
https://stackoverflow.com/questions/16682156/ld-library-not-found-for-lgsl

emscripten_mac_framework_flags = '-framework -lGLU -lglut'


## fatal error: 'GL/osmesa.h' file not found
clang-17: warning: argument unused during compilation: '-Wno-vla-extension' [-Wunused-command-line-argument]
clang-17: warning: argument unused during compilation: '-pedantic' [-Wunused-command-line-argument]
clang-17: warning: argument unused during compilation: '-D NDEBUG' [-Wunused-command-line-argument]
clang-17: warning: argument unused during compilation: '-O3' [-Wunused-command-line-argument]
clang-17: warning: argument unused during compilation: '-I /usr/local/include' [-Wunused-command-line-argument]
Traceback (most recent call last):
  File "/usr/local/Cellar/emscripten/3.1.31/libexec/em++.py", line 14, in <module>
    sys.exit(emcc.main(sys.argv))
             ^^^^^^^^^^^^^^^^^^^
  File "/usr/local/Cellar/python@3.11/3.11.1/Frameworks/Python.framework/Versions/3.11/lib/python3.11/contextlib.py", line 81, in inner
    return func(*args, **kwds)
           ^^^^^^^^^^^^^^^^^^^
  File "/usr/local/Cellar/emscripten/3.1.31/libexec/emcc.py", line 4304, in main
    ret = run(args)
          ^^^^^^^^^
  File "/usr/local/Cellar/emscripten/3.1.31/libexec/emcc.py", line 1272, in run
    linker_inputs = phase_compile_inputs(options, state, newargs, input_files)
                    ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
  File "/usr/local/Cellar/python@3.11/3.11.1/Frameworks/Python.framework/Versions/3.11/lib/python3.11/contextlib.py", line 81, in inner
    return func(*args, **kwds)
           ^^^^^^^^^^^^^^^^^^^
  File "/usr/local/Cellar/emscripten/3.1.31/libexec/emcc.py", line 2959, in phase_compile_inputs
    compile_source_file(i, input_file)
  File "/usr/local/Cellar/emscripten/3.1.31/libexec/emcc.py", line 2953, in compile_source_file
    assert os.path.exists(output_file)
AssertionError
scons: *** [web_db_loader.web.o] Error 1
In file included from mkpospages.cpp:12:
./images.hpp:7:10: fatal error: 'GL/osmesa.h' file not found
#include <GL/osmesa.h>

### Solution
conditionally include mac headers soulution found here: 

Installed osmesa with 
```
export LLVM_BUILD=1
bash <(curl -s https://raw.githubusercontent.com/devernay/osmesa-install/master/osmesa-install.sh)
```
and added to `mac-brew-install-devtools.sh` 

Found that it was installed at `/opt/osmesa/`

so added the following to `./SConstruct.mac`
`-I /opt/osmesa/include -I /opt/llvm/include`

```
env = Environment(ENV=os.environ, CCFLAGS='-Wall -Wextra -pedantic -Wno-missing-field-initializers -std=c++1y -DNDEBUG -O3 -DUSE_FTGL -I /usr/local/include -I /usr/local/include/boost -I /usr/local/include/boost/gil -I /opt/osmesa/include -I /opt/llvm/include')
```


## no member named 'png_write_view' in namespace 'boost::gil'
/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/System/Library/Frameworks/OpenGL.framework/Headers/gl.h:2474:13: note: 'glEnd' has been explicitly marked deprecated here
extern void glEnd (void) OPENGL_DEPRECATED(10.0, 10.14);
            ^
images.cpp:231:15: error: no member named 'png_write_view' in namespace 'boost::gil'
                boost::gil::png_write_view(tmpfile,
                ~~~~~~~~~~~~^
rendering.cpp:20:7: warning: unused function 'glNormal' [-Wunused-function]
        void glNormal(V3 const & v) { ::glNormal3d(v.x, v.y, v.z); }
             ^
rendering.cpp:22:7: warning: unused function 'glTranslate' [-Wunused-function]
        void glTranslate(V3 const & v) { ::glTranslated(v.x, v.y, v.z); }
             ^
images.cpp:307:14: error: no member named 'png_write_view' in namespace 'boost::gil'
        boost::gil::png_write_view(path,
        ~~~~~~~~~~~~^
9 warnings generated.
7 warnings and 3 errors generated.
68 warnings generated.
scons: *** [images.o] Error 1
9 warnings generated.

### Solution


## 'boost/gil/gil_all.hpp' file not found
AssertionError
scons: *** [web_db_loader.web.o] Error 1
In file included from images.cpp:2:
./images.hpp:23:10: fatal error: 'boost/gil/gil_all.hpp' file not found
#include <boost/gil/gil_all.hpp>


## Solution 

https://github.com/apolukhin/Boost-Cookbook/blob/912e36f38b9b1da93b03ae7afd19fcec0900aa83/Chapter12/07_gil/main.cpp#L13

It appears that older versions of boost used `include <boost/gil/gil_all.hpp>` but newer versions use `include <boost/gil.hpp>`

#include <boost/version.hpp>
#if (BOOST_VERSION < 106800)
#   include <boost/gil/gil_all.hpp>
#else
#   include <boost/gil.hpp>
#endif

## 'boost/gil/extension/io/png_io.hpp' file not found
cpp:39:10: fatal error: 'boost/gil/extension/io/png_io.hpp' file not found
#include <boost/gil/extension/io/png_io.hpp>
         ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
6 warnings and 5 errors generated.
scons: *** [mkpospages.o] Error 1
6 warnings and 4 errors generated.
scons: *** [makevideo.o] Error 1

### Solution 


## Boost not found

### Solution 
Add the following to bash profile 

```
LIBRARY_PATH="/usr/local/include/boost "
export LIBRARY_PATH

LD_LIBRARY_PATH="/usr/local/include/boost "
export LD_LIBRARY_PATH
```

and add `/usr/local/include/boost` to $PATH

## library not found for -lOSMesa
./scripts/cross-mac.sh
g++ -o grapplemap-mkpospages mkpospages.o images.o rendering.o playerdrawer.o graph.o graph_util.o positions.o viables.o persistence.o paths.o playback.o icosphere.o editor.o metadata.o md5.o js_conversions.o -L/usr/local/Cellar/graphviz/7.1.0/lib -L/usr/local/opt/freetype/lib -lOSMesa -lGLU -lftgl -lboost_program_options -lpng -lboost_filesystem -lboost_system -lpthread -lgvc -lcgraph
ld: library not found for -lOSMesa
clang: error: linker command failed with exit code 1 (use -v to see invocation)
scons: *** [grapplemap-mkpospages] Error 1

### Solution






# TBD 
- https://bitsanddragons.wordpress.com/2021/04/06/notes-adding-boost-libraries-on-osx-big-sur/
- Not sure if bjam needed in homebrew 
- Not sure if the following packages are needed as they are not found in homebrew
  - libboost-filesystem-dev \
  - libboost-regex-dev \
  - libboost-program-options-dev \