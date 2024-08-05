#include "GLDeviceInternal.hpp"

#include "Slic3r/App/Render/Context.hpp"
#include "Slic3r/App/Render/Geometry.hpp"
#include "Slic3r/App/Render/Shader.hpp"

#include "commonGL.hpp"
#include "GLTypes.hpp"
#include "GLBufferInternal.hpp"
#include "GLGeometryInternal.hpp"
#include "GLShaderInternal.hpp"
#include "GLTextureInternal.hpp"

#include "Slic3r/Assert.hpp"
#include "GL/glew.h"

#define RENDER_TRACE_LOG 0

namespace Slic3r::App::Render::GL {

GLDeviceInternal::GLDeviceInternal(Context& context): m_context(context)
{
    m_bound_textures.resize(context.max_texture_units(), 0);
}

void GLDeviceInternal::load_state()
{
    glGetIntegerv(GL_CURRENT_PROGRAM, reinterpret_cast<GLint*>(&m_bound_shader));
    glCheck();
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, reinterpret_cast<GLint*>(&m_bound_vertex_buffer));
    glCheck();
    glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, reinterpret_cast<GLint*>(&m_bound_index_buffer));
    glCheck();
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, reinterpret_cast<GLint*>(&m_bound_vao));
    glCheck();

    GLint active_texture_unit;
    glGetIntegerv(GL_ACTIVE_TEXTURE, &active_texture_unit);
    glCheck();
    m_active_texture_unit = static_cast<uint8_t>(active_texture_unit - GL_TEXTURE0);

    for (auto& tex : m_bound_textures)
        tex = 0;

#if RENDER_TRACE_LOG
    SPDLOG_INFO(
        "loaded GL state: shader {}  VB {}  IB {}  VAO {}", m_bound_shader, m_bound_vertex_buffer,
        m_bound_index_buffer, m_bound_vao
    );
#endif
}

void GLDeviceInternal::activate_texture_unit(uint8_t unit)
{
    if (m_active_texture_unit == unit)
        return;
    glActiveTexture(GL_TEXTURE0 + unit);
    m_active_texture_unit = unit;
}

void GLDeviceInternal::bind_texture(uint8_t unit, Texture& t)
{
    const auto& tex = t.get_internal_as<GLTextureInternal>();
    if (m_bound_textures[unit] == tex.m_id)
        return;
    activate_texture_unit(unit);
    glBindTexture(tex.m_target, tex.m_id);
    glCheck();
}

void GLDeviceInternal::unbind_texture(uint8_t unit, Texture& t)
{
    const auto& tex = t.get_internal_as<GLTextureInternal>();
    if (m_bound_textures[unit] == 0)
        return;
    activate_texture_unit(unit);
    glBindTexture(tex.m_target, 0);
    m_bound_textures[unit] = 0;
}

void GLDeviceInternal::bind_shader(Shader& s)
{
    ResourceId shader_id = s.get_internal_as<GLShaderInternal>().m_id;
    if (m_bound_shader == shader_id)
        return;

    glUseProgram(shader_id);
    glCheck();
    m_bound_shader = shader_id;
}

void GLDeviceInternal::bind_vertex_buffer(ResourceId vb)
{
#if RENDER_TRACE_LOG
    SPDLOG_INFO("Binding VB {}", vb);
#endif
    if (m_bound_vertex_buffer == vb)
        return;
#if RENDER_TRACE_LOG
    SPDLOG_INFO("Bound VB {}", vb);
#endif
    glBindBuffer(GL_ARRAY_BUFFER, vb);
    glCheck();
    m_bound_vertex_buffer = vb;
#if RENDER_TRACE_LOG
    SPDLOG_INFO("(bind_vertex_buffer) Setting bound VB {}", vb);
#endif
}

void GLDeviceInternal::bind_index_buffer(ResourceId ib)
{
#if RENDER_TRACE_LOG
    SPDLOG_INFO("Binding IB {}", ib);
#endif
    if (m_bound_index_buffer == ib)
        return;
#if RENDER_TRACE_LOG
    SPDLOG_INFO("Bound IB {}", ib);
#endif
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ib);
    glCheck();
    m_bound_index_buffer = ib;
    m_bound_indices = ib != 0;
}

void GLDeviceInternal::bind_vao(ResourceId vao)
{
#if RENDER_TRACE_LOG
    SPDLOG_INFO("Binding vao {}", vao);
#endif
    if (m_bound_vao == vao)
        return;
#if RENDER_TRACE_LOG
    SPDLOG_INFO("Bound vao {}", vao);
#endif
    glBindVertexArray(vao);
    glCheck();
    m_bound_vao = vao;
}

void GLDeviceInternal::bind_buffer(BufferTarget target, ResourceId buffer)
{
    switch (target) {
    case BufferTarget::VertexBuffer:
        bind_vertex_buffer(buffer);
        break;

    case BufferTarget::IndexBuffer:
        bind_index_buffer(buffer);
        break;

    default:
        // unsupported target
        PANIC("Unsupported buffer target to bind to");

    }
}

void GLDeviceInternal::bind_geometry(Geometry& g, Shader& shader)
{
    DEBUG_ASSERT(g.ready());
    // TODO: handle ES with VAO
    bool use_vao = m_context.is_vao_available();
    auto& geom = g.get_internal_as<GLGeometryInternal>();

    GLuint vb_id = g.vertex_buffer()->get_internal_as<GLBufferInternal>().m_id;
    if (use_vao) {
        bind_vao(geom.m_vao_id);
    }

    m_bound_indices = geom.m_has_indices;
    GLuint shader_id = shader.get_internal_as<GLShaderInternal>().m_id;
    bool needs_new_binding = !use_vao || geom.m_shader_id != shader_id;

    if (needs_new_binding) {
        bind_vertex_buffer(vb_id);

        const VertexAttribsDesc& attrs = g.vertex_format();
        const size_t stride = vertex_attribs_stride(attrs);

        for (const auto& vad : attrs) {
            int loc = shader.get_attrib_location(shader_input_name(vad.attrib_type));
            if (loc < 0)
                continue;
            glEnableVertexAttribArray(loc);
            glVertexAttribPointer(
                loc, vad.components, type(vad.data_type), GL_FALSE, stride,
                reinterpret_cast<void*>(vad.offset)
            );
            glCheck();
        }

        auto* index_buffer = g.index_buffer();
        if (index_buffer) {
            bind_index_buffer(index_buffer->get_internal_as<GLBufferInternal>().m_id);
            m_bound_index_type = g.index_type();
        } else
            bind_index_buffer(0);
        geom.m_shader_id = shader_id;
    }

#if RENDER_TRACE_LOG
    print_buffer_info("bind_geometry");
#endif

    if (m_context.is_es()) {
        bind_vertex_buffer(vb_id);
    }
}

void GLDeviceInternal::unbind_geometry()
{
#if RENDER_TRACE_LOG
    print_buffer_info("before unbind_geometry");
#endif

    if (m_context.is_vao_available()) {
        bind_vao(0);
        bind_vertex_buffer(0);
        bind_index_buffer(0);
#if RENDER_TRACE_LOG
        SPDLOG_INFO("(unbind_geometry) Setting bound VB {}", 0);
#endif
    } else {
        bind_vertex_buffer(0);
        bind_index_buffer(0);
    }

#if RENDER_TRACE_LOG
    print_buffer_info("after bind_geometry");
#endif
}


void GLDeviceInternal::draw(PrimitiveType primitive, size_t offset, size_t count)
{
#if RENDER_TRACE_LOG
    print_buffer_info("draw");
#endif

    if (m_bound_indices) {
#if RENDER_TRACE_LOG
        SPDLOG_INFO("Draw Elements");
#endif
        glDrawElements(
            GL::type(primitive), count, type(m_bound_index_type),
            reinterpret_cast<const void*>(0 + index_type_size(m_bound_index_type) * offset)
        );
    } else {
#if RENDER_TRACE_LOG
        SPDLOG_INFO("Draw Arrays");
#endif
        glDrawArrays(GL::type(primitive), offset, count);
    }
    glCheck();

}

void GLDeviceInternal::print_buffer_info(const char* action)
{
    GLint bound_shader = 0;
    GLint bound_vertex_buffer = 0;
    GLint bound_index_buffer = 0;
    GLint bound_vao = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, reinterpret_cast<GLint*>(&bound_shader));
    glCheck();
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, reinterpret_cast<GLint*>(&bound_vertex_buffer));
    glCheck();
    glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, reinterpret_cast<GLint*>(&bound_index_buffer));
    glCheck();
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, reinterpret_cast<GLint*>(&bound_vao));
    glCheck();

    SPDLOG_INFO(
        "Bound buffers (at {}): shader {} ({})  VB {} ({})  IB {} ({})  VAO {} ({})",
        action,
        bound_shader, m_bound_shader,
        bound_vertex_buffer, m_bound_vertex_buffer,
        bound_index_buffer, m_bound_index_buffer,
        bound_vao, m_bound_vao
    );

    if (bound_vao) {
        GLint max_attrs;
        glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &max_attrs);
        glCheck();

        SPDLOG_INFO("Bound VAO buffers");

        for (GLint i = 0; i < max_attrs; i++) {
            GLint bound_vbi = -1;
            //glGetIntegeri_v(GL_VERTEX_ARRAY_BUFFER_BINDING, i, &bound_vbi);
            glGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING, &bound_vbi);
            glCheck();
            if (bound_vbi != 0)
                SPDLOG_INFO("attr {}: VBO {}", i, bound_vbi);
        }
    }

}


}
