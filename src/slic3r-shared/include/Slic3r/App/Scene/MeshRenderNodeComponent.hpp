#pragma once

#include "Slic3r/App/Scene/IRenderNodeComponent.hpp"

namespace Slic3r::App::Scene {

class MeshRenderNodeComponent : public IRenderNodeComponent
{
public:
    MeshRenderNodeComponent() = default;
    MeshRenderNodeComponent(const MeshRenderNodeComponent&) = default;
    MeshRenderNodeComponent(MeshRenderNodeComponent&&) = default;

    explicit MeshRenderNodeComponent(
        const Render::Geometry* geometry,
        const Material& material,
        Render::PrimitiveType primitive_type = Render::PrimitiveType::Triangles,
        size_t count = 0,
        size_t offset = 0
    ) : m_material(material)
    {
        ASSERT(material.shader() != nullptr, "Shader must be specified for primary material");
        set_geometry(geometry, primitive_type, count, offset);
    }

    MeshRenderNodeComponent& operator=(const MeshRenderNodeComponent&) = default;
    MeshRenderNodeComponent& operator=(MeshRenderNodeComponent&&) = default;

    void render(
        const Node& node,
        const Camera& camera,
        const Material& material_override,
        Render::CommandBuffer& cmd_buffer
    ) const override;

    int layer_index() const override { return m_layer;}
    void set_layer_index(int layer) { m_layer = layer; }

    void set_geometry(const Render::Geometry* geometry, Render::PrimitiveType primitive_type=Render::PrimitiveType::Triangles, size_t count=0, size_t offset=0);



private:
    const Render::Geometry* m_geometry{nullptr};
    Material m_material;
    Render::PrimitiveType m_primitive_type{Render::PrimitiveType::Triangles};
    size_t m_vertex_offset{0};
    size_t m_vertex_count{0};
    int m_layer{0};
};

} // namespace Slic3r::App::Scene
