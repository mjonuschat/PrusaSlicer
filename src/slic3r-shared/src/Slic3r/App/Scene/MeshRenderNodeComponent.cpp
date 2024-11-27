//
// Created by Jan Bartipan on 10.09.2024.
//

#include "Slic3r/App/Scene/MeshRenderNodeComponent.hpp"
#include "Slic3r/App/Scene/Node.hpp"
#include "Slic3r/App/Render/Geometry.hpp"

namespace Slic3r::App::Scene {

const std::string UNIFORM_VIEW_MODEL_MATRIX = "view_model_matrix";
const std::string UNIFORM_PROJECTION_MATRIX = "projection_matrix";
const std::string UNIFORM_VIEW_NORMAL_MATRIX = "view_normal_matrix";
const std::string UNIFORM_VOLUME_WORLD_MATRIX = "volume_world_matrix";

void MeshRenderNodeComponent::render(
    const Node& node,
    const Camera& camera,
    const Render::Material& resolved_material,
    Render::CommandBuffer& cmd_buffer
) const
{
    const Render::Shader& shader = *resolved_material.shader();
    Render::Material material = resolved_material;

    // Set transform uniforms
    Transform view = camera.view();
    const Transform& model = node.world_transform();

    // update per-node uniforms
    Matrix4f model_view = (view * model).cast<float>();
    material.set_uniform(UNIFORM_VIEW_MODEL_MATRIX, model_view);
    Matrix4f value = camera.projection().cast<float>();
    material.set_uniform(UNIFORM_PROJECTION_MATRIX, value);
    Matrix3f normal =
        (view.block<3, 3>(0, 0) * model.block<3, 3>(0, 0)).inverse().transpose().cast<float>();
    material.set_uniform(UNIFORM_VIEW_NORMAL_MATRIX, normal);
    Matrix4f vol_world = node.world_transform().cast<float>();
    material.set_uniform(UNIFORM_VOLUME_WORLD_MATRIX, vol_world);

    cmd_buffer.bind_and_draw(*m_geometry, material);
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
