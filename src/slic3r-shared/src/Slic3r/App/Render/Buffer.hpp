#pragma once

#include <memory>

#include <Slic3r/Log.hpp>

#include "commonGL.hpp"
#include "Shader.hpp"
#include "Context.hpp"
#include "libslic3r/Point.hpp"

namespace Slic3r::App::Render {

class Buffer {
public:
    explicit Buffer(GLenum target): m_target(target)
    {
        glGenBuffers(1, &m_id);
        glCheck();
    }

    virtual ~Buffer();

    inline void bind() const
    {
        glBindBuffer(m_target, m_id);
        glCheck();
    }

    inline void set_data(const void* data, GLsizeiptr size, GLenum usage)
    {
        bind();
        glBufferData(m_target, size, data, usage);
        glCheck();
    }

private:
    GLenum m_target;
    GLuint m_id {0};
};

class VertexBuffer : public Buffer
{
public:
    VertexBuffer() : Buffer(GL_ARRAY_BUFFER) {}

    inline void bind_vertex_attrib(GLuint index, GLint size, GLenum type, bool normalized, GLsizei stride, const void* pointer)
    {
        glVertexAttribPointer(index, size, type, normalized ? GL_TRUE : GL_FALSE, stride, pointer);
        glCheck();
    }
};

class IndexBuffer : public Buffer
{
public:
    IndexBuffer() : Buffer(GL_ELEMENT_ARRAY_BUFFER) {}
};

} // namespace Slic3r::App::Render
