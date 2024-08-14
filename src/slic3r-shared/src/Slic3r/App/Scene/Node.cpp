#include "Node.hpp"

namespace Slic3r::App::Scene {
// NOLINTBEGIN(misc-no-recursion): Mark recursion in query() as resolved
void Node::query(const NodePredicate &predicate, NodeList &found_nodes) {
    if (predicate(this)) {
        found_nodes.push_back(this);
    }
    for (auto &child : m_children) {
        child->query(predicate, found_nodes);
    }
}

void Node::query(const NodePredicate &predicate, ConstNodeList &found_nodes) const {
    if (predicate(this)) {
        found_nodes.push_back(this);
    }
    for (auto &child : m_children) {
        child->query(predicate, found_nodes);
    }
}

const Transform& Node::world_transform() const {
    if (m_world_xform_dirty) {
        if (m_parent) {
            m_world_xform = m_local_xform * m_parent->world_transform();
        } else {
            m_world_xform = m_local_xform;
        }
        m_world_xform_dirty = false;
    }
    return m_world_xform;
}

// NOLINTEND(misc-no-recursion)

}
