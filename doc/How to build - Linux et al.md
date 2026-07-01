
# Building PrusaSlicer on UNIX/Linux

Please understand that PrusaSlicer team cannot support compilation on all possible Linux distros. Namely, we cannot help troubleshoot OpenGL driver issues or dependency issues if compiled against distro provided libraries. **We can only support PrusaSlicer statically linked against the dependencies compiled with the `deps` scripts**, the same way we compile PrusaSlicer for our Flatpak bundle.

If you have some reason to link dynamically to your system libraries, you are free to do so, but we can not and will not troubleshoot any issues you possibly run into.

## Prerequisities

GNU build tools, CMake, git and other libraries have to be installed on the build machine.
Unless that's already the case, install them as usual from your distribution packages.
E.g. on Ubuntu 26.04, run
```shell
sudo apt install \
git \
build-essential \
autoconf \
cmake \
libglu1-mesa-dev \
libgtk-3-dev \
libdbus-1-dev \
libwebkit2gtk-4.1-dev \
texinfo
```
The names of the packages may be different on different distros.

## Building dependencies and PrusaSlicer

This is a complete list of commands needed to build PrusaSlicer:

```
git clone https://www.github.com/prusa3d/PrusaSlicer
cd PrusaSlicer
cd deps
mkdir build
cd build
cmake ..
cmake --build .
cd ../..
mkdir build
cd build
cmake .. -DCMAKE_PREFIX_PATH=$(pwd)/../deps/build/destdir/usr/local
cmake --build .
```

In short, there are two steps. First you download deps sources and build them. Then you build PrusaSlicer, pointing it to the dependencies you just built. The application is linked statically.


## Useful CMake flags when building PrusaSlicer
- `-DCMAKE_BUILD_TYPE=Debug` to build in debug mode (defaults to `Release`)
- `-DSLIC3R_ASAN=ON` enables gcc/clang address sanitizer (defaults to `OFF`, requires gcc>4.8 or clang>3.1)
- `-DSLIC3R_STATIC=OFF` for dynamic builds (defaults to `ON`)
- `-DSLIC3R_GUI=0` to build the console variant of PrusaSlicer

See the root CMakeLists.txt to get the complete list of options.


## Running Unit Tests

To run all unit tests:

    cd build
    ctest


Each test has a separate binary, the test cases are labeled. It is possible to invoke subset of available test cases and passing the label as a filter:

    ./fff_tests [LabelUsedForFiltering]
