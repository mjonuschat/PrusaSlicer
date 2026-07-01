# Building PrusaSlicer on Windows


## 0. Prerequisities

The following tools need to be installed on your computer:
- Microsoft Visual Studio
- CMake
- git


## 1. Download sources

Clone the respository. Use a directory relatively close to the drive root, so the path is not too long. Avoid spaces and non-ASCII characters. To place it in `C:\src\PrusaSlicer`, run:
```
c:> mkdir src
c:> cd src
c:\src> git clone https://github.com/prusa3d/PrusaSlicer.git
```


## 2. Build the dependencies.

Dependencies are seldom updated, so they are built out of PrusaSlicer source tree.

Open the MSVC x64 Native Tools Command Prompt and run the following:
```
cd c:\src\PrusaSlicer\deps
mkdir build
cd build
cmake ..
cmake --build .
```
Expect this to take some time. Note that both _Debug_ and _Release_ variants are built. You can force only the _Release_ build by passing `-DDEP_DEBUG=OFF` to the first CMake call.

# 3. Generate MSVC project file for PrusaSlicer

The dependencies that were just built need to be referenced here.

Open the MSVC x64 Native Tools Command Prompt and run:
```
cd c:\src\PrusaSlicer\
mkdir build
cd build
cmake .. -DCMAKE_PREFIX_PATH="c:\src\PrusaSlicer\deps\build\destdir\usr\local"
```

Note that `CMAKE_PREFIX_PATH` must be absolute path. A relative path will not work.

## 4. Open the project and build PrusaSlicer

Double-click c:\src\PrusaSlicer\build\PrusaSlicer.sln to open in Visual Studio and select `slic3r-app-launcher` as your startup project (right-click->Set as Startup Project). You can now build and run the application from within MSVC.



# Running Unit Tests

To run all unit tests:

    cd c:\src\PrusaSlicer\build
    ctest


Each test has a separate binary, the test cases are labeled. It is possible to invoke subset of available test cases and passing the label as a filter:

    fff_tests.exe [LabelUsedForFiltering]




