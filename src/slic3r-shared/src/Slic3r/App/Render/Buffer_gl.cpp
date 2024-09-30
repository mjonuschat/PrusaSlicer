#include "Slic3r/App/Render/Buffer.hpp"
#include "Slic3r/App/Render/Device.hpp"
#include "Slic3r/App/Render/GL/commonGL.hpp"
#include "Slic3r/App/Render/GL/GLBufferInternal.hpp"
#include "Slic3r/App/Render/GL/GLDeviceInternal.hpp"

namespace Slic3r::App::Render {

Buffer::Buffer(Device& device, BufferTarget target)
    : WithInternal(InternalType<GL::GLBufferInternal>(), target), m_device(device), m_target(target)
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

void Buffer::set_data(const void* data, size_t size, BufferUsage usage)
{
    auto& self = get_internal_as<GL::GLBufferInternal>();
    m_device.get_internal_as<GL::GLDeviceInternal>().bind_buffer(m_target, self.m_id);
    glBufferData(self.m_target, size, data, GL::type(usage));
    glCheck();
}


} // namespace Slic3r::App::Render
