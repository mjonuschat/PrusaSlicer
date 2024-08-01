#include "Geometry.hpp"
#include "Device.hpp"

#include "Slic3r/App/Render/GL/GLGeometryInternal.hpp"
#include "Slic3r/App/Render/GL/GLDeviceInternal.hpp"
#include "Slic3r/App/Render/GL/commonGL.hpp"
#include "Slic3r/App/Render/GL/GLTypes.hpp"

namespace Slic3r::App::Render {

Geometry::Geometry(Device& device)
    : WithInternal(InternalType<GL::GLGeometryInternal>()), m_device(device)
{}

Geometry::~Geometry()
{
    auto& self = get_internal_as<GL::GLGeometryInternal>();
    if (self.m_vao_id) {
        glDeleteVertexArrays(1, &self.m_vao_id);
    }
}

void Geometry::upload(
    const void* vertex_data,
    size_t vertex_count,
    const VertexAttribsDesc& vertex_format,
    const void* index_data,
    size_t index_count,
    Render::IndexType index_format
)
{
    SPDLOG_TRACE("Buffer::upload() Part 1");
    DEBUG_ASSERT(!m_built);

    auto& ctx = Context::instance();
    // TODO: handle ES with VAO
    bool use_vao = ctx.is_vao_available();

    auto& self = get_internal_as<GL::GLGeometryInternal>();
    auto& device = m_device.get_internal_as<GL::GLDeviceInternal>();
    self.m_has_indices = index_count > 0;
    if (use_vao) {
        glGenVertexArrays(1, &self.m_vao_id);
        glCheck();
        device.bind_vao(self.m_vao_id);
    }
    SPDLOG_TRACE("Buffer::upload() Part 2");


    m_vb = m_device.create_vertex_buffer();
    m_vb->set_data(vertex_data, vertex_attribs_stride(vertex_format) * vertex_count, BufferUsage::StaticDraw);
    m_vertex_count = vertex_count;
    m_vertex_format = vertex_format;

    if (index_data && index_count > 0) {
        m_ib = m_device.create_index_buffer();
        m_ib->set_data(index_data, index_type_size(index_format) * index_count, BufferUsage::StaticDraw);
        m_index_count = index_count;
        m_index_type = index_format;
    }
    device.bind_vao(0);
    device.bind_vertex_buffer(0);
    device.bind_index_buffer(0);

    m_built = true;
}


} // namespace Slic3r::App::Render
