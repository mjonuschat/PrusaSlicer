#pragma once

#include "Slic3r/App/Render/Buffer.hpp"


namespace Slic3r::App::Render::GL {

GLenum getGlBufferTarget(BufferTarget target) {
    switch (target)
    {
    case BufferTarget::VertexBuffer:
        return GL_ARRAY_BUFFER;
    case BufferTarget::IndexBuffer:
        return GL_ELEMENT_ARRAY_BUFFER;
    }
}

struct GLBufferInternal : public Buffer::Internal {
    GLenum m_target;
    GLuint m_id {0};

    explicit GLBufferInternal(BufferTarget target): m_target(getGlBufferTarget(target)) {}
};

}
