#pragma once

#include "Slic3r/App/Render/Buffer.hpp"
#include "GLTypes.hpp"

namespace Slic3r::App::Render::GL {

struct GLBufferInternal : public Buffer::Internal {
    GLenum m_target;
    GLuint m_id {0};

    explicit GLBufferInternal(BufferTarget target): m_target(type(target)) {}
};

}
