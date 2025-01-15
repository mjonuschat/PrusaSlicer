# Code Style

This page outlines codes style used in Prusa Slicer project. All new code
have to comply with this document.

## Pointers vs references

- Prefer reference over the pointer. Use pointers only if `nullptr` value is one of valid value for input/output.
- Prefer using `std::unique_ptr` over raw pointers. It prevents leaking memory when e.g. exception occures.

## C++

We use C++ 20 standard (as Yoga library requires it anyway, otherwise we would stick with C++17). 
But we need to use only such features that are supported on all platforms.  There are some limitations 
in the implementation by Apple Clang (e.g. missing `std::stop_token`) or MSVC.


## Directory structure and files

- Used file extensions are `.hpp` for headers and `.cpp` for C++ sources.
- Every library within Prusa Slicer have to follow this directory structure:
  - `include` directory containing all PUBLIC headers
  - `src` directory containing all code (including private headers)
- The directory structure in `include` and `src` must mirror the namespace location. It means that file containing
  `Slic3r::Domain::Project` is located in `include/Slic3r/Domnain/Project.hpp` and `src/Slic3r/Domain/Project.cpp` 
  files.

## Identation

Four spaces, no tabs used

## Naming conventions

### Class/Struct/Enum

- 


