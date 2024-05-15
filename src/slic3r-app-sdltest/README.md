# SDL Application for testing Slic3r platform

This is very simple app used to test implementation of platform internals. 
It is supposed to be compilable as desktop as well as Emscripten app.

To compile with Emscripten do following:

```bash
cd PrusaSlicer

# We are at the project top level
# First build deps

mkdir -p deps/build-emscripten
cd deps/build-emscripten
emcmake .. && make -j

# This should compile all deps
# For some reason the build may fail, but re-running just works fine (missing dep?)
# I.e. you may need to run:
#    emmake make -j 
# multiple times 

# Create build directory for app artifacts at project top level
mkdir ../../build
cd ../../build

# Finally run app build
emcmake .. -DCMAKE_FIND_ROOT_PATH=/ -DCMAKE_PREFIX_PATH="$PWD/deps/build-emscripten/destdir/usr/local" -DSLIC3R_GUI=0
emmake make -j

```