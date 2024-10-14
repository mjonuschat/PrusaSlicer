#pragma once
#include "Slic3r/App/Scene/Transform.hpp"
#include "Slic3r/App/Scene/NodeInfo.hpp"
#include "Slic3r/App/Scene/IRenderNodeComponent.hpp"
#include "Slic3r/App/Scene/IImguiRenderNodeComponent.hpp"
#include "Slic3r/App/Scene/IRaycastNodeComponent.hpp"
#include "Slic3r/App/Scene/INodeTransformModifier.hpp"
#include "Slic3r/App/Scene/Material.hpp"
#include "Slic3r/App/Scene/NodeVisitorTypes.hpp"
#include "Slic3r/App/Scene/ScreenSpaceSizedTransformModifier.hpp"

#include <boost/any.hpp>

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

    bool enabled() const { return m_enabled; }
    void set_enabled(bool enabled) { m_enabled = enabled; }

    const INodeTransformModifier* transform_modifier() const { return m_transform_modifier.get(); }
    INodeTransformModifier* transform_modifier() { return m_transform_modifier.get(); }
    void set_transform_modifier(std::unique_ptr<INodeTransformModifier>&& modifier)
    { m_transform_modifier = std::move(modifier); }

    void query(const NodePredicate& predicate, NodeList& found_nodes, bool ignore_enabled = false);
    void query(const NodePredicate& predicate, ConstNodeList& found_nodes, bool ignore_enabled = false) const;

    bool has_render_component() const { return bool(m_render_component);}
    void set_render_component(IRenderNodeComponent* component);
    void set_render_component(std::unique_ptr<IRenderNodeComponent>&& component) { set_render_component(component.release()); }
    const IRenderNodeComponent* render_component() const { return m_render_component.get(); }

    bool has_material_override() const { return bool(m_material_override); }
    void set_material_override(const Material& material) { m_material_override = std::make_unique<Material>(material); }
    void remove_material_override() { if (m_material_override) m_material_override.reset(); }
    const Material* material_override() const { return m_material_override.get(); }

    bool has_imgui_render_component() const { return bool(m_imgui_render_component);}
    void set_imgui_render_component(IImguiRenderNodeComponent* component);
    void set_imgui_render_component(std::unique_ptr<IImguiRenderNodeComponent>&& component) { set_imgui_render_component(component.release()); }
    const IImguiRenderNodeComponent* imgui_render_component() const { return m_imgui_render_component.get(); }

    bool has_raycast_component() const { return bool(m_raycast_component); }
    void set_raycast_component(std::unique_ptr<IRaycastNodeComponent>&& component) { m_raycast_component = std::move(component); }
    void set_raycast_component(IRaycastNodeComponent* component) { m_raycast_component.reset(component); }
    const IRaycastNodeComponent* raycast_component() const { return m_raycast_component.get(); }

    template <typename T>
    bool has_tag_of_type() const { return m_tag.type() == typeid(T); }
    void set_tag(const boost::any& tag) { m_tag = tag; }
    boost::any tag() const { return m_tag; }

private:
    friend class Scene;

    // Use Scene::add_node instead
    void add_child(Node* n)
    {
        if (n->m_parent == this)
            // already added
            return;

        n->m_parent = this;
        n->mark_world_transform_dirty();
        m_children.emplace_back(n);
    }

    // Use Scene::remove_node instead
    bool remove_children(const NodePredicate& predicate, const std::function<void(Node*)>& node_callback = {})
    {
        return std::remove_if(
           m_children.begin(), m_children.end(),
           [predicate, node_callback](NodeOwningList::reference node) {
               Node* node_ptr = node.get();
               if (node_callback)
                   node_callback(node_ptr);
               return predicate(node_ptr);
           }
        ) != m_children.end();
    }


    void mark_world_transform_dirty() const;
    //void mark_children_world_transform_dirty() const;


    friend void visit(const Node&, const ConstNodeVisitor &, bool);
    friend void visit(Node&, const NodeVisitor &, bool);
    friend void visit_conditional(const Node& node, const ConstNodeConditionalVisitor& visitor, bool);
    friend void visit_conditional(Node& node, const NodeConditionalVisitor& visitor, bool);
private:
    friend void ScreenSpaceSizedTransformModifier::camera_updated(const Camera& cam);

    Node* m_parent{nullptr};
    NodeOwningList m_children;

    Transform m_local_xform{Transform::Identity()};
    mutable Transform m_world_xform{Transform::Identity()};
    mutable bool m_world_xform_dirty{false};

    bool m_enabled{true};

    std::unique_ptr<IRenderNodeComponent> m_render_component;
    std::unique_ptr<Material> m_material_override;
    std::unique_ptr<IImguiRenderNodeComponent> m_imgui_render_component;
    std::unique_ptr<IRaycastNodeComponent> m_raycast_component;
    std::unique_ptr<INodeTransformModifier> m_transform_modifier;

    boost::any m_tag;
};

} // namespace Slic3r::App::Scene
