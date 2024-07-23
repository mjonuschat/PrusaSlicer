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
    if (m_bound_vertex_buffer == vb)
        return;
    glBindBuffer(GL_ARRAY_BUFFER, vb);
    glCheck();
    m_bound_vertex_buffer = vb;
}

void GLDeviceInternal::bind_index_buffer(ResourceId ib)
{
    if (m_bound_index_buffer == ib)
        return;
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ib);
    glCheck();
    m_bound_index_buffer = ib;
}

void GLDeviceInternal::bind_vao(ResourceId vao)
{
    if (m_bound_vao == vao)
        return;
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
        assert(false);

    }
}

void GLDeviceInternal::bind_geometry(Geometry& g, Shader& shader)
{
    assert(g.ready());
    // TODO: handle ES with VAO
    bool use_vao = m_context.is_vao_available();
    auto& self = g.get_internal_as<GLGeometryInternal>();

    if (use_vao)
        bind_vao(self.m_vao_id);

    GLuint shader_id = shader.get_internal_as<GLShaderInternal>().m_id;
    bool needs_new_binding = !use_vao || self.m_shader_id != shader_id;

    if (needs_new_binding) {
        bind_vertex_buffer(g.vertex_buffer()->get_internal_as<GLBufferInternal>().m_id);

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
        self.m_shader_id = shader_id;
    }
}


void GLDeviceInternal::draw(PrimitiveType primitive, size_t offset, size_t count)
{
    if (m_bound_index_buffer)
        glDrawElements(
            GL::type(primitive), count, type(m_bound_index_type),
            reinterpret_cast<const void*>(0 + index_type_size(m_bound_index_type) * offset)
        );
    else
        glDrawArrays(GL::type(primitive), offset, count);
    glCheck();

}

}
