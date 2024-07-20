#include "Buffer.hpp"
#include "Slic3r/App/Render/GL/GLBufferInternal.hpp"
namespace Slic3r::App::Render {


Buffer::Buffer(BufferTarget target): WithInternal(InternalType<GL::GLBufferInternal>(), target)
{
    auto& self = get_internal_as<GL::GLBufferInternal>();
    glGenBuffers(1, &self.m_id);
    glCheck();
}

Buffer::~Buffer()
{
    auto& self = get_internal_as<GL::GLBufferInternal>();
    if (self.m_id)
        glDeleteBuffers(1, &self.m_id);
}

void Buffer::bind() const
{
    auto& self = get_internal_as<GL::GLBufferInternal>();
    glBindBuffer(self.m_target, self.m_id);
    glCheck();
}

void Buffer::set_data(const void* data, GLsizeiptr size, GLenum usage)
{
    auto& self = get_internal_as<GL::GLBufferInternal>();
    bind();
    glBufferData(self.m_target, size, data, usage);
    glCheck();
}


} // namespace Slic3r::App::Render
