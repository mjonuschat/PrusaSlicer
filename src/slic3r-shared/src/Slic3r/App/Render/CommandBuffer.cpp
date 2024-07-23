#include "CommandBuffer.hpp"

#include "Geometry.hpp"

#include "Slic3r/App/Render/GL/commonGL.hpp"
#include "Slic3r/App/Render/GL/GLTypes.hpp"
#include "Slic3r/App/Render/GL/GLCommandBufferInternal.hpp"
#include "Slic3r/App/Render/GL/GLDeviceInternal.hpp"


namespace Slic3r::App::Render {

namespace GL {


inline void setEnabled(GLenum flag, bool enabled)
{
    if (enabled)
        glEnable(flag);
    else
        glDisable(flag);
    glCheck();
}

} // namespace GL

CommandBuffer::CommandBuffer(Device& device)
    : WithInternal(InternalType<GL::GLCommandBufferInternal>()), m_device(device)
{}

void CommandBuffer::set_clear_values(const RgbaF& clear_color, double clear_depth)
{
    glClearColor(clear_color.r, clear_color.g, clear_color.b, clear_color.a);
    glCheck();
    glClearDepth(clear_depth);
    glCheck();
}

void CommandBuffer::clear_buffers(bool color, bool depth)
{
    assert(color || depth);
    GLenum buffer_mask = 0;
    if (color)
        buffer_mask |= GL_COLOR_BUFFER_BIT;
    if (depth)
        buffer_mask |= GL_DEPTH_BUFFER_BIT;
    if (buffer_mask) {
        glClear(buffer_mask);
        glCheck();
    }
}

void CommandBuffer::set_viewport(const Rect& viewport)
{
    glViewport(viewport.x, viewport.y, viewport.width, viewport.height);
    glCheck();
}

void CommandBuffer::set_scissor(const Rect& scissor)
{
    glScissor(scissor.x, scissor.y, scissor.width, scissor.height);
    glCheck();
}

void CommandBuffer::set_scissor_enabled(bool enabled)
{
    GL::setEnabled(GL_SCISSOR_TEST, enabled);
}

void CommandBuffer::set_blending(const Blending& blending)
{
    glBlendFunc(GL::type(blending.src), GL::type(blending.dst));
    glCheck();
}

void CommandBuffer::set_blending_enabled(bool enabled)
{
    GL::setEnabled(GL_BLEND, enabled);
}

void CommandBuffer::bind_texture(uint8_t unit, Texture& t)
{
    auto& device = m_device.get_internal_as<GL::GLDeviceInternal>();
    device.bind_texture(unit, t);
}

void CommandBuffer::unbind_texture(uint8_t unit, Texture& t)
{
    auto& device = m_device.get_internal_as<GL::GLDeviceInternal>();
    device.unbind_texture(unit, t);
}

void CommandBuffer::bind_shader(Shader& s)
{
    auto& device = m_device.get_internal_as<GL::GLDeviceInternal>();
    device.bind_shader(s);
}

void CommandBuffer::bind_geometry(Geometry& g, Shader& s)
{
    auto& device = m_device.get_internal_as<GL::GLDeviceInternal>();
    device.bind_geometry(g, s);
}

void CommandBuffer::draw(PrimitiveType primitive, size_t offset, size_t count)
{
    auto& device = m_device.get_internal_as<GL::GLDeviceInternal>();
    device.draw(primitive, offset, count);
}

void CommandBuffer::submit()
{
}



}
