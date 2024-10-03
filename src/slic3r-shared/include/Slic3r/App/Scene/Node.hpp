#pragma once
#include "Slic3r/App/Scene/Transform.hpp"
#include "Slic3r/App/Scene/NodeInfo.hpp"
#include "Slic3r/App/Scene/IRenderNodeComponent.hpp"
#include "Slic3r/App/Scene/Material.hpp"

namespace Slic3r::App::Scene {

class Node
{
public:
    using NodeOwningList = std::vector<std::unique_ptr<Node>>;
    using NodeList = std::vector<Node*>;
    using ConstNodeList = std::vector<const Node*>;
    using NodePredicate = std::function<bool(const Node*)>;

    const Node* parent() const { return m_parent; }

    const Transform& local_transform() const { return m_local_xform; }
    const Transform& world_transform() const;

    void set_local_transform(const Matrix4f& t)
    {
        m_local_xform = t;
        mark_world_transform_dirty();
    }

    void set_world_transform(const Matrix4f& t)
    {
        if (m_parent)
        {
            auto inv_parent_world = m_parent->world_transform().inverse();
            set_local_transform(t * inv_parent_world);
        } else
            set_local_transform(t);
    }

    void add_child(Node* n)
    {
        if (n->m_parent == this)
            // already added
            return;

        n->m_parent = this;
        n->mark_world_transform_dirty();
        m_children.emplace_back(n);
    }

    bool remove_children(const NodePredicate& predicate)
    {
        return std::remove_if(
                   m_children.begin(), m_children.end(),
                   [predicate](NodeOwningList::const_reference node) {
                       return predicate(node.get());
                   }
               ) != m_children.end();
    }

    void query(const NodePredicate& predicate, NodeList& found_nodes);
    void query(const NodePredicate& predicate, ConstNodeList& found_nodes) const;

    const NodeInfo& node_info() const { return m_node_info; }
    NodeInfo& node_info() { return m_node_info; }

    bool has_render_component() const { return bool(m_render_component);}
    void set_render_component(IRenderNodeComponent* component, const Material& material);
    const IRenderNodeComponent* render_component() const { return m_render_component.get(); }

    bool has_material_override() const { return bool(m_material_override); }
    void set_material_override(const Material& material) { m_material_override = std::make_unique<Material>(material); }
    const Material* material_override() const { return m_material_override.get(); }

private:
    void reset_world_transform(const Matrix4f& t)
    {
        m_world_xform = t;
        m_world_xform_dirty = false;
    }

    void mark_world_transform_dirty() const
    {
        if (!m_world_xform_dirty) {
            m_world_xform_dirty = true;
            mark_children_world_transform_dirty();
        }
    }
    // NOLINTBEGIN(misc-no-recursion): Mark recursion in query() as resolved
    void mark_children_world_transform_dirty() const
    {
        for (auto& ch : m_children) {
            ch->m_world_xform_dirty = true;
            ch->mark_children_world_transform_dirty();
        }
    }
    // NOLINTEND(misc-no-recursion)

private:
    Node* m_parent{nullptr};
    NodeOwningList m_children;

    Transform m_local_xform{Transform::Identity()};
    mutable Transform m_world_xform{Transform::Identity()};
    mutable bool m_world_xform_dirty{false};

    NodeInfo m_node_info;

    std::unique_ptr<IRenderNodeComponent> m_render_component;
    std::unique_ptr<Material> m_material_override;
};

} // namespace Slic3r::App::Scene
