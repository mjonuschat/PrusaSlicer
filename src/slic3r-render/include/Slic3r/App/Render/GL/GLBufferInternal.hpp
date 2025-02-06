#pragma once

#include "Slic3r/App/Render/Buffer.hpp"
#include "GLTypes.hpp"

namespace Slic3r::App::Render::GL {

struct GLBufferInternal : public Buffer::Internal {
    GLenum m_target;
    GLuint m_id {0};
#if !SLIC3R_OPENGL_ES && !defined(__EMSCRIPTEN__)
    GLuint m_tex_id{ 0 };
#endif // !SLIC3R_OPENGL_ES && !defined(__EMSCRIPTEN__)

    explicit GLBufferInternal(BufferTarget target): m_target(type(target)) {}
};

}
