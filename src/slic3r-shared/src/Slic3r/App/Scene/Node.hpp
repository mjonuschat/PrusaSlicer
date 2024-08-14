#pragma once
#include "libslic3r/Geometry.hpp"
#include "NodeInfo.hpp"

namespace Slic3r::App::Scene {

using Transform = Eigen::Matrix4f;


class Node
{
    using NodeOwningList = std::vector<std::unique_ptr<Node>>;
    using NodeList = std::vector<Node*>;
    using ConstNodeList = std::vector<const Node*>;
    using NodePredicate = std::function<bool(const Node*)>;

    const Transform& local_transform() const { return m_local_xform; }
    const Transform& world_transform() const;

    void set_local_transform(const Matrix4f& t) {
        m_local_xform = t;
        mark_world_transform_dirty();
    }

    void set_world_transform(const Matrix4f& t) {
        m_world_xform = t;
        m_world_xform_dirty = false;
    }

    void add_child(Node* n) {
        if (n->m_parent == this)
            // already added
            return;

        n->m_parent = this;
        n->mark_world_transform_dirty();
        m_children.emplace_back(n);
    }

    bool remove_children(const NodePredicate &predicate) {
        return std::remove_if(
                   m_children.begin(), m_children.end(),
                   [predicate](NodeOwningList::const_reference node) {
                       return predicate(node.get());
                   }
               ) != m_children.end();
    }

    void query(const NodePredicate &predicate, NodeList &found_nodes);
    void query(const NodePredicate &predicate, ConstNodeList &found_nodes) const;

private:
    void mark_world_transform_dirty() const {
        if (!m_world_xform_dirty) {
            m_world_xform_dirty = true;
            mark_children_world_transform_dirty();
        }
    }
    // NOLINTBEGIN(misc-no-recursion): Mark recursion in query() as resolved
    void mark_children_world_transform_dirty() const {
        for (auto& ch: m_children) {
            ch->m_world_xform_dirty = true;
            ch->mark_children_world_transform_dirty();
        }
    }
    // NOLINTEND(misc-no-recursion)

private:
    Node* m_parent {nullptr};
    NodeOwningList m_children;

    Transform m_local_xform;
    mutable Transform m_world_xform;
    mutable bool m_world_xform_dirty {false};

    NodeInfo node_info;
};

}
