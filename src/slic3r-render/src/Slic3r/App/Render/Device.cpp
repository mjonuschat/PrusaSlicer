#include "Slic3r/App/Render/Device.hpp"
#include "Slic3r/App/Render/CommandBuffer.hpp"
#include "Slic3r/App/Render/Context.hpp"
#include "Slic3r/App/Render/GL/commonGL.hpp"
#include "Slic3r/App/Render/GL/GLDeviceInternal.hpp"
#include "Slic3r/App/Render/GL/GLBufferInternal.hpp"
#include "Slic3r/App/Render/GL/GLFramebufferInternal.hpp"

#include "Slic3r/App/Render/Texture.hpp"
#include "Slic3r/App/Render/Buffer.hpp"
#include "Slic3r/App/Render/Framebuffer.hpp"

namespace Slic3r::App::Render {

Device::Device(Context& context)
    : WithInternal(InternalType<GL::GLDeviceInternal>(), context)
    , m_context(context)
{}

void Device::load_state() { get_internal_as<GL::GLDeviceInternal>().load_state(); }

std::unique_ptr<Texture> Device::create_texture()
{
    return std::unique_ptr<Texture>(new Texture(*this));
}

std::unique_ptr<VertexBuffer> Device::create_vertex_buffer()
{
    return std::make_unique<VertexBuffer>(*this);
}

std::unique_ptr<IndexBuffer> Device::create_index_buffer()
{
    return std::make_unique<IndexBuffer>(*this);
}

#ifdef SLIC3R_RENDER_TEXTURE_BUFFER_SUPPORTED
std::unique_ptr<TextureBuffer> Device::create_texture_buffer(PixelFormat format)
{
    return std::make_unique<TextureBuffer>(*this, format);
}
#endif // SLIC3R_RENDER_TEXTURE_BUFFER_SUPPORTED

std::unique_ptr<CommandBuffer> Device::create_command_buffer()
{
    return std::make_unique<CommandBuffer>(*this);
}

std::unique_ptr<Framebuffer> Device::create_framebuffer(const FramebufferCreationData& data)
{
    return std::make_unique<Framebuffer>(*this, data);
}

void Device::bind_buffer(const Buffer& b)
{
    get_internal_as<GL::GLDeviceInternal>().bind_buffer(b.target(), b.get_internal_as<GL::GLBufferInternal>().m_id);
}

void Device::unbind_buffer(const Buffer& b)
{
    get_internal_as<GL::GLDeviceInternal>().unbind_buffer(b.target());
}

void* Device::map_buffer(const Buffer& b, BufferAccess access)
{
    return get_internal_as<GL::GLDeviceInternal>().map_buffer(b.target(), access);
}

void Device::unmap_buffer(const Buffer& b)
{
    get_internal_as<GL::GLDeviceInternal>().unmap_buffer(b.target());
}

void Device::bind_framebuffer(const Framebuffer& b)
{
    get_internal_as<GL::GLDeviceInternal>().bind_framebuffer(b);
}

void Device::unbind_framebuffer(const Framebuffer& b)
{
    get_internal_as<GL::GLDeviceInternal>().unbind_framebuffer(b);
}

} // namespace Slic3r::App::Render
