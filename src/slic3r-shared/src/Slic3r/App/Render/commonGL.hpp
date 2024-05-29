#pragma once

#include <iostream>
#include <cassert>
#include <GL/glew.h>

#ifdef NDEBUG
#define glCheck()
#else
#define glCheck() { \
  GLenum err = glGetError(); \
  if (err != GL_NO_ERROR) { \
    std::cerr << __FILE__ << ":" << __LINE__  << " (" << __FUNCTION__ << ") GL Error: " << ::Slic3r::App::Render::gl_error_desc(err) << "\n"; \
    assert(false); \
  } \
}
#endif

namespace Slic3r::App::Render {

const char* gl_error_desc(GLenum err);

void initialize_gl();

}
