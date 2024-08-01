#pragma once

#include <iostream>
#include <Slic3r/Assert.hpp>
#include <GL/glew.h>

#ifdef NDEBUG
#define glCheck()
#else
#define glCheck() { \
  GLenum err = glGetError(); \
  if (err != GL_NO_ERROR) {  \
    PANIC(std::string("GL Error: ") + ::Slic3r::App::Render::gl_error_desc(err)); \
  } \
}
#endif

namespace Slic3r::App::Render {

const char* gl_error_desc(GLenum err);

}
