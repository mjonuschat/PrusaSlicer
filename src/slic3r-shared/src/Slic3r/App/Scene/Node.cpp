#include "Slic3r/App/Scene/Node.hpp"

#include <memory>

#include "Slic3r/Assert.hpp"

namespace Slic3r::App::Scene {
// NOLINTBEGIN(misc-no-recursion): Mark recursion in query() as resolved
void Node::query(const NodePredicate& predicate, NodeList& found_nodes)
{
    if (predicate(this)) {
        found_nodes.push_back(this);
    }
    for (auto& child : m_children) {
        child->query(predicate, found_nodes);
    }
}

void Node::query(const NodePredicate& predicate, ConstNodeList& found_nodes) const
{
    if (predicate(this)) {
        found_nodes.push_back(this);
    }
    for (auto& child : m_children) {
        child->query(predicate, found_nodes);
    }
}

const Transform& Node::world_transform() const
{
    if (m_world_xform_dirty) {
        if (m_parent) {
            m_world_xform = m_parent->world_transform() * m_local_xform;
        } else {
            m_world_xform = m_local_xform;
        }
        m_world_xform_dirty = false;
    }
    return m_world_xform;
}

// NOLINTEND(misc-no-recursion)


void Node::set_render_component(IRenderNodeComponent* component, const Material& material)
{
    ASSERT(material.shader() != nullptr, "Shader is required");
    m_render_component.reset(component);
    m_material_override = std::make_unique<Material>(material);
}

} // namespace Slic3r::App::Scene
