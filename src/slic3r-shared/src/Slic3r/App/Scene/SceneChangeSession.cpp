#include "Slic3r/App/Scene/SceneChangeSession.hpp"

namespace Slic3r::App::Scene {

void AddNodeChange::roll_back(Scene& scene)
{
    auto* node = scene.node(m_node_id);
    Node* original_parent = m_original_parent_id == 0 ? nullptr : scene.node(m_original_parent_id);
    const Node::NodePredicate& predicate = [&](const auto* n) { return n->id() == m_node_id; };
    if (original_parent == nullptr)
        scene.remove_children(predicate, node->parent());
    else {
        auto children = scene.detach_children(predicate);
        for (auto& n : children)
            scene.add_child(n.release(), original_parent);
    }
}

void AddMaterialOverrideChange::roll_back(Scene& scene)
{
    Node* node = scene.node(m_node_id);
    if (m_original_material)
        node->set_material_override(*m_original_material);
    else
        node->remove_material_override();
}


void SceneChangeSession::roll_back()
{
    for(auto& c : m_changes)
        c->roll_back(m_scene);
    m_changes.clear();
}

SceneChangeSession::NodeChangeBuilder& SceneChangeSession::NodeChangeBuilder::add_child(Node* child)
{
    m_session.m_scene.add_child(child, &m_node);
    const Node* p = child->parent();
    m_session.m_changes.push_back(
        std::make_unique<AddNodeChange>(child->id(), p ? p->id() : 0)
    );
    return *this;
}
SceneChangeSession::NodeChangeBuilder& SceneChangeSession::NodeChangeBuilder::set_material_override(
    const Material& m
)
{
    m_session.m_changes.push_back(
        std::make_unique<AddMaterialOverrideChange>(m_node.id(), m_node.material_override())
    );
    m_node.set_material_override(m);
    return *this;
}
}
