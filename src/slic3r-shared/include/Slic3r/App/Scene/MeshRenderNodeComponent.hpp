#pragma once

#include "IRenderNodeComponent.hpp"

namespace Slic3r::App::Scene {

class MeshRenderNodeComponent : public IRenderNodeComponent
{
public:
    void render(const Node& node, Render::CommandBuffer& cmd_buffer) override;

private:
    Render::Geometry* m_geometry{nullptr};
    Render::Shader* m_shader{nullptr};
    Render::PrimitiveType m_primitive_type;
    size_t m_vertex_offset{0};
    size_t m_vertex_count{0};
};

} // namespace Slic3r::App::Scene
