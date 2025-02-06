# Code Style

This page outlines codes style used in Prusa Slicer project. All new code
have to comply with this document.

## Pointers vs references

- Prefer reference over the pointer. Use pointers only if `nullptr` value is one of valid value for input/output.
- Prefer using `std::unique_ptr` over raw pointers. It prevents leaking memory when e.g. exception occures.

## C++

We use **C++ 20** standard (as Yoga library requires it anyway, otherwise we would stick with C++17). 
But we need to use only such features that are supported on all platforms.  There are some limitations 
in the implementation by Apple Clang (e.g. missing `std::stop_token`) or MSVC.


## Directory structure and files

- Used file extensions are `.hpp` for headers and `.cpp` for C++ sources.
- Every library within Prusa Slicer have to follow this directory structure:
  - `include` directory containing all PUBLIC headers
  - `src` directory containing all code (including private headers)
  - `test` directory containing all code and data for unit tests
- The directory structure in `include` and `src` must mirror the namespace location. It means that file containing
  `Slic3r::Domain::Project` is located in `include/Slic3r/Domnain/Project.hpp` and `src/Slic3r/Domain/Project.cpp` 
  files.

Example directory structure of `slic3r-shared` library (see `../src/slic3r-shared`):

```
slic3r-shared
├── CMakeLists.txt
├── README.md
├── include
│   └── Slic3r
│       ├── App
│       │   ├── Color.hpp
│       │   ├── ...
│       ├── Biz
│       │   ├── IProjectsChangedListener.hpp
│       │   ├── ...
│       ├── Domain
│       │   ├── Bed.hpp
│       │   ├── ...
│       └── StringUtils.hpp
├── src
│   └── Slic3r
│       ├── App
│       │   ├── Color.cpp
│       │   ├── ...
│       ├── Biz
│       │   ├── ...
│       └── Domain
│           ├── Bed.cpp
│           ├── ...
└── test
    ├── Slic3r
    │   ├── App
    │   │   └── Scene
    │   │       ├── Node.cpp
    │   │       └── Ray.cpp
    │   └── Biz
    │       └── ProjectInteractorTests.cpp
    └── data
        ├── ...
```

## Indentation

Four spaces, no tabs used

## Naming conventions

### Class/Struct/Enum
- class/struct
  - Name Pascal case
  - Both braces at new line
  - method & var name: snake case
  - enum constants: Pascal case
  
  ```c++
  class MyWindow : public BaseClass 
  {
  public:
      void my_method(int input_parameter) const;
  public:
      int public_member_var;
  private:
      int m_non_public_member_var;
      static int s_static_var;
  }
  ```

- enum:
  - preferred enum class
  - use PascalCase for name and all its constants
 
  ```c++
  enum class BufferUsage {
      StaticDraw,
      DynamicDraw,
      StreamDraw
  };

  ```
  
## Braces

  - class/struct/enum and function body: both on new line

      ```c++
      class MyClass 
      {
          
      }
      ```

  - control structures (if/for/while/switch) and namespace decls:
    - opening brace at the end of same line (?)

        ```c++
        if (expr) {
            
        }
        ```

## Variables and types

- Pointer/reference placement

  - next to type, space before identifier

    ```
    int* ptr_buffer;
    const std::string& ref_string;
    ```

  - prevent multi-var def at one line if types differ

      ```
      // this is not good idea
      int* ptr_to_int, just_int;
      ```

## Namespaces

- Concat nested namespace

    ```
    // DO
    namespace Slic3r::GUI {
    
    }
    
    // DON'T
    namespace Slic3r {
        namespace GUI {
        
        }
    }
    ```

## Documentation comments

- We use [doxygen](https://www.doxygen.nl/manual/docblocks.html)
- We use this type of comment blocks`

  ```c++
  /**
   * Documentation goes here...
   */
  ```
  
- We use doxygen commands prefixed with `@` (e.g. `@brief`)
- We don't use automatic brief generation form first sentence, you have to explicitly 
  denote the brief part (one sentence shown in class/module table of contents) with `@brief` like this:
  
  ```c++
   /**
    * @brief Scenegraph entrypoint
    *
    * Encapsulate scene tree made of Node and provides:
    * - rendering of 3D object, see render()
    * - rendering of 2D GUI overlay, see render_imgui()
    * - camera used for rendering (camera()) and its trackball manipulator (camera_trackball())
    * - ray picking, see pick_at()
    * - also provides common resource caching manager for rendering geometry (geometry_manager()),
    *   and in-memory geometry (triangle_mesh_manager())
    * .
    */
  ```
  
- Note that doxygen [supports subset of markdown](https://www.doxygen.nl/manual/markdown.html) including lists. 
  But note that **lists needs to be explicitly terminated by line with dot** (see the previous example and last doxygen line)


