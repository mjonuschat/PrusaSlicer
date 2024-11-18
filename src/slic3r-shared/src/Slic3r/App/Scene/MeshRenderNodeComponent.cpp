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
    const Material& material_override,
    Render::CommandBuffer& cmd_buffer
) const
{
    Material mat = m_material;
    mat.update(material_override);

    const Render::Shader& shader = *mat.shader();
    cmd_buffer.bind_shader(shader);
    cmd_buffer.bind_geometry(*m_geometry, shader);

    for (const auto [slot, texture] : mat.textures())
        cmd_buffer.bind_texture(slot, *texture);

    for (const auto& [name, value] : mat.uniforms())
        Render::set_uniform(shader, name.c_str(), value);

    // Set transform uniforms
    Transform view = camera.view();
    const Transform& model = node.world_transform();

    int uniform_id = shader.get_uniform_location("view_model_matrix");
    if (uniform_id >= 0) {
        Matrix4f model_view = (view * model).cast<float>();
        shader.set_uniform(uniform_id, model_view);
    }

    uniform_id = shader.get_uniform_location("projection_matrix");
    if (uniform_id >= 0) {
        shader.set_uniform(uniform_id, camera.projection());
    }

    uniform_id = shader.get_uniform_location("view_normal_matrix");
    if (uniform_id >= 0) {
        //Matrix3f normal = view.block<3, 3>(0, 0) * model.block<3, 3>(0, 0).inverse().transpose();
        Matrix3f normal =
            (view.block<3, 3>(0, 0) * model.block<3, 3>(0, 0)).inverse().transpose().cast<float>();
        shader.set_uniform(uniform_id, normal);
    }

    uniform_id = shader.get_uniform_location("volume_world_matrix");
    if (uniform_id >= 0)
        shader.set_uniform(uniform_id, node.world_transform());

    cmd_buffer.draw(m_primitive_type, m_vertex_offset, m_vertex_count);

    for (const auto [slot, texture] : mat.textures())
        cmd_buffer.unbind_texture(slot, *texture);
}

void MeshRenderNodeComponent::set_geometry(
    const Render::Geometry* geometry,
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
