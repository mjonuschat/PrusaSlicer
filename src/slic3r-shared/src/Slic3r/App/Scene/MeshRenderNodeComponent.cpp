//
// Created by Jan Bartipan on 10.09.2024.
//

#include "Slic3r/App/Scene/MeshRenderNodeComponent.hpp"
#include "Slic3r/App/Scene/Node.hpp"
#include "Slic3r/App/Render/Geometry.hpp"

namespace Slic3r::App::Scene {

void MeshRenderNodeComponent::render(
    const Node& node,
    const Camera& camera,
    const Material& material,
    Render::CommandBuffer& cmd_buffer
) const
{
    const Render::Shader& shader = *material.shader();
    cmd_buffer.bind_shader(shader);
    cmd_buffer.bind_geometry(*m_geometry, shader);

    for (const auto [slot, texture] : material.textures())
        cmd_buffer.bind_texture(slot, *texture);

    for (const auto& [name, value] : material.uniforms())
        Render::set_uniform(shader, name.c_str(), value);

    // Set transform uniforms
    Matrix4f view = camera.view();
    Matrix4f model_view;
    bool model_view_computed = false;

    int uniform_id = shader.get_uniform_location("view_model_matrix");
    if (uniform_id >= 0) {
        model_view = view * node.world_transform();
        model_view_computed = true;
        shader.set_uniform(uniform_id, model_view);
    }

    uniform_id = shader.get_uniform_location("projection_matrix");
    if (uniform_id >= 0) {
        shader.set_uniform(uniform_id, camera.projection());
    }

    uniform_id = shader.get_uniform_location("view_normal_matrix");
    if (uniform_id >= 0) {
        if (!model_view_computed) {
            model_view = view * node.world_transform();
            model_view_computed = true;
        }
        Matrix3f normal = model_view.block<3, 3>(0, 0).inverse().transpose();
        shader.set_uniform(uniform_id, normal);
    }

    uniform_id = shader.get_uniform_location("volume_world_matrix");
    if (uniform_id >= 0)
        shader.set_uniform(uniform_id, node.world_transform());

    cmd_buffer.draw(m_primitive_type, m_vertex_offset, m_vertex_count);

    for (const auto [slot, texture] : material.textures())
        cmd_buffer.unbind_texture(slot, *texture);
}

void MeshRenderNodeComponent::set_geometry(
    Render::Geometry* geometry,
    Render::PrimitiveType primitive_type,
    size_t count,
    size_t offset
)
{
    m_geometry = geometry;
    m_primitive_type = primitive_type;
    if (count == 0) {
        offset = 0;
        count = geometry->index_count();
        if (count == 0)
            count = geometry->vertex_count();
    }
    m_vertex_offset = offset;
    m_vertex_count = count;
}

} // namespace Slic3r::App::Scene
