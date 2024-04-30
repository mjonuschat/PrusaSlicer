#pragma once

#include "commonGL.hpp"

namespace Slic3r::App::Render {

class Buffer {
public:
    explicit Buffer(GLenum target): m_target(target)
    {
        glGenBuffers(1, &m_id);
        glCheck();
    }

    ~Buffer();

    inline void bind() const
    {
        glBindBuffer(m_target, m_id);
        glCheck();
    }

    inline void setData(const void* data, size_t size, GLenum usage)
    {
        bind();
        glBufferData(m_target, size, data, usage);
        glCheck();
    }

private:
    GLenum m_target;
    GLuint m_id {0};
};

} // namespace Slic3r::App::Render
