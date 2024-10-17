#include "Slic3r/App/Render/Device.hpp"
#include "Slic3r/App/Render/CommandBuffer.hpp"
#include "Slic3r/App/Render/Context.hpp"
#include "Slic3r/App/Render/GL/commonGL.hpp"
#include "Slic3r/App/Render/GL/GLDeviceInternal.hpp"

#include "Slic3r/App/Render/Texture.hpp"
#include "Slic3r/App/Render/Buffer.hpp"

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

std::unique_ptr<CommandBuffer> Device::create_command_buffer()
{
    return std::make_unique<CommandBuffer>(*this);
}

}
