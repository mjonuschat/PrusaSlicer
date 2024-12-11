#include "Slic3r/App/Scene/NodeBuilder.hpp"
#include "Slic3r/App/Scene/MeshRenderNodeComponent.hpp"
#include "Slic3r/App/Scene/AabbRaycastNodeComponent.hpp"
#include "Slic3r/App/Scene/ScreenSpaceSizedTransformModifier.hpp"
#include "Slic3r/App/Scene/Scene.hpp"

namespace Slic3r::App::Scene {

void NodeBuilder::ensure_current()
{
    if (!m_current)
        m_current = std::make_unique<Node>();
}

NodeBuilder& NodeBuilder::transform(const std::function<void(Transform3d&)>& modifier)
{
    ensure_current();

    Transform3d xform = Transform3d::Identity();
    modifier(xform);
    m_current->set_local_transform(xform.matrix());
    return *this;
}

NodeBuilder& NodeBuilder::set_mesh(const Render::Geometry* geometry, const Render::Material& material, int layer_index)
{
    ensure_current();

    auto render_component = std::make_unique<MeshRenderNodeComponent>(geometry, material, Render::PrimitiveType::Triangles, 0, 0);
    render_component->set_layer_index(layer_index);
    m_current->set_render_component(std::move(render_component));
    return *this;
}

NodeBuilder& NodeBuilder::set_material_override(const Render::Material& material)
{
    ensure_current();

    m_current->set_material_override(material);
    return *this;
}

NodeBuilder& NodeBuilder::set_imgui_func(const FuncImguiRenderNodeComponent::RenderFunc& imgui_render_func)
{
    ensure_current();

    m_current->set_imgui_render_component(
        std::make_unique<FuncImguiRenderNodeComponent>(imgui_render_func)
    );
    return *this;
}

NodeBuilder& NodeBuilder::set_screen_space_sized_modifier(double scale)
{
    ensure_current();
    auto modifier =
        std::make_unique<ScreenSpaceSizedTransformModifier>(m_scene.camera(), *m_current, scale);
    m_current->set_transform_modifier(std::move(modifier));
    return *this;
}


NodeBuilder& NodeBuilder::set_aabb(const AABBMesh* aabb)
{
    ensure_current();

    m_current->set_raycast_component(new AabbRaycastNodeComponent(aabb));
    return *this;
}

NodeBuilder& NodeBuilder::child(const std::function<void(NodeBuilder&)>& builder)
{
    ensure_current();

    begin_child();
    builder(*this);
    end_child();
    return *this;
}

NodeBuilder& NodeBuilder::children(size_t num_children, const std::function<void(NodeBuilder&, size_t)>& builder)
{
    for (size_t i = 0; i < num_children; i++) {
        begin_child();
        builder(*this, i);
        end_child();
    }
    return *this;
}

NodeBuilder& NodeBuilder::begin_child()
{
    ensure_current();

    m_parents.push_back(std::move(m_current));
    m_current = std::make_unique<Node>();
    return *this;
}

NodeBuilder& NodeBuilder::end_child()
{
    DEBUG_ASSERT(!m_parents.empty(), "Unbalanced begin_child()/end_child() calls");
    ensure_current();

    auto& parent = m_parents.back();
    //parent->add_child(m_current.release());
    m_scene.add_child(m_current.release(), parent.get());
    m_current = std::move(parent);
    m_parents.pop_back();
    return *this;
}

std::unique_ptr<Node> NodeBuilder::build()
{
    DEBUG_ASSERT(m_parents.empty(), "Unbalanced begin_child()/end_child() calls");
    ensure_current();

    return std::move(m_current);
}

} // namespace SLic3r::App::Scene
