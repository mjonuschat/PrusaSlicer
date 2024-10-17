#pragma once
#include "Slic3r/App/Scene/Transform.hpp"
#include "Slic3r/App/Scene/IRenderNodeComponent.hpp"
#include "Slic3r/App/Scene/IImguiRenderNodeComponent.hpp"
#include "Slic3r/App/Scene/IRaycastNodeComponent.hpp"
#include "Slic3r/App/Scene/INodeTransformModifier.hpp"
#include "Slic3r/App/Scene/Material.hpp"
#include "Slic3r/App/Scene/NodeVisitorTypes.hpp"
#include "Slic3r/App/Scene/ScreenSpaceSizedTransformModifier.hpp"

#include <boost/any.hpp>

namespace Slic3r::App::Scene {

/**
 * @brief Scenegraph node with transformation and set of node components.
 *
 * @details
 *
 * This is sort of low-level object, for more comfort use
 * - Scene as entry point to scenegraph,
 * - @ref NodeVisitor.hpp "node visitors" to visit and transform (sub-)graph
 * - NodeBuilder to create sub-scenegraph
 * .
 *
 * In terms of tree hierarchy node contains:
 * - parent link: see @ref Node::parent() const
 * - list of children (see Node::children() const
 * .
 * In terms of transformation node contains:
 * - local transform (stored, see local_transform() )
 * - world_transform (computed if dirty, see world_transform() )
 * .
 * And these are components the can get attached:
 * - to render 3D object in scene use IRenderNodeComponent or its implementation MeshRenderNodeComponent,
 *   see render_component(), set_render_component(), has_render_component()
 * - to override visual material of 3D object use Material,
 *   see material_override(), set_material_override(), has_material_override()
 * - to support object picking use IRaycastNodeComponent or its AABBMesh implementation AabbRaycastNodeComponent,
 *   see raycast_component(), set_raycast_component(), has_raycast_component()
 * - to render 2D GUI overlay use IImguiRenderNodeComponent
 *   see imgui_render_component(), set_imgui_render_component(), has_imgui_render_component()
 * - tag with extra information helping identify purpose of specific node (e.g. if picked via Scene::pick_at()),
 *   see has_tag_of_type(), set_tag(), tag()
 * .
 */
class Node
{
public:
    using NodeOwningList = std::vector<std::unique_ptr<Node>>;
    using NodeList = std::vector<Node*>;
    using ConstNodeList = std::vector<const Node*>;
    using NodePredicate = std::function<bool(const Node*)>;

    /**
     * @brief Nodes parent pointer
     * @return Node parent of `nullptr` if root itself.
     */
    const Node* parent() const { return m_parent; }

    /**
     * @brief Read-only children
     *
     * Use Node::query() or @ref NodeVisitor.hpp "node visitors"
     *
     * @return Constant vector of `unique_ptr<Node>`
     */
    const NodeOwningList& children() const { return m_children; }

    /**
     * @brief Local transformation
     * @return
     */
    const Transform& local_transform() const { return m_local_xform; }
    /**
     * @brief World transformation
     *
     * If dirty the transformation gets recomputed.
     *
     * @return
     */
    const Transform& world_transform() const;

    /**
     * @brief Sets local transformation
     *
     * Sets local transformation and marks world_transform() dirty for this node and all its children.
     *
     * @param t
     */
    void set_local_transform(const Transform& t)
    {
        m_local_xform = t;
        mark_world_transform_dirty();
    }

    /**
     * @brief Sets world transformation
     * @param t
     */
    void set_world_transform(const Transform& t)
    {
        if (m_parent)
        {
            auto inv_parent_world = m_parent->world_transform().inverse();
            set_local_transform(t * inv_parent_world);
        } else
            set_local_transform(t);
    }

    /**
     * @brief Is node enabled.
     *
     * Only enabled nodes can appear in query(const NodePredicate&, NodeList&, bool),
     * query(const ConstNodePredicate&, ConstNodeList&, bool), and @ref @ref NodeVisitor.hpp "node visitors"
     *
     * @return
     */
    bool enabled() const { return m_enabled; }

    /**
     * @brief Set node enabled flag
     * @param enabled
     */
    void set_enabled(bool enabled) { m_enabled = enabled; }

    /**
     * @name TransformModifier
     * World Transformation modifier
     * @{
     */
    const INodeTransformModifier* transform_modifier() const { return m_transform_modifier.get(); }
    INodeTransformModifier* transform_modifier() { return m_transform_modifier.get(); }
    void set_transform_modifier(std::unique_ptr<INodeTransformModifier>&& modifier)
    { m_transform_modifier = std::move(modifier); }
    /**@}*/

    /**
     * @name Query
     * Query node and its children
     * @{
     */
    /**
     * @brief Query this node and its children (only enabled by default).
     */
    void query(const NodePredicate& predicate, NodeList& found_nodes, bool ignore_enabled = false);

    /**
     * @brief Query this node and its children (only enabled by default).
     */
    void query(const NodePredicate& predicate, ConstNodeList& found_nodes, bool ignore_enabled = false) const;
    /**@}*/

    /**
     * @name RenderComponent
     * Rendering 3D object
     * @{
     */
    bool has_render_component() const { return bool(m_render_component);}
    void set_render_component(IRenderNodeComponent* component);
    void set_render_component(std::unique_ptr<IRenderNodeComponent>&& component) { set_render_component(component.release()); }
    const IRenderNodeComponent* render_component() const { return m_render_component.get(); }
    /**@}*/

    /**
     * @name MaterialOverride
     * Override 3D object material
     * @{
     */
    bool has_material_override() const { return bool(m_material_override); }
    void set_material_override(const Material& material) { m_material_override = std::make_unique<Material>(material); }
    void remove_material_override() { if (m_material_override) m_material_override.reset(); }
    const Material* material_override() const { return m_material_override.get(); }
    /**@}*/

    /**
     * @name ImguiRenderComponent
     * Rendering 2D GUI overlay
     * @{
     */
    bool has_imgui_render_component() const { return bool(m_imgui_render_component);}
    void set_imgui_render_component(IImguiRenderNodeComponent* component);
    void set_imgui_render_component(std::unique_ptr<IImguiRenderNodeComponent>&& component) { set_imgui_render_component(component.release()); }
    const IImguiRenderNodeComponent* imgui_render_component() const { return m_imgui_render_component.get(); }
    /**@}*/

    /**
     * @name RaycastComponent
     * Raycast hit test
     * @{
     */
    bool has_raycast_component() const { return bool(m_raycast_component); }
    void set_raycast_component(std::unique_ptr<IRaycastNodeComponent>&& component) { m_raycast_component = std::move(component); }
    void set_raycast_component(IRaycastNodeComponent* component) { m_raycast_component.reset(component); }
    const IRaycastNodeComponent* raycast_component() const { return m_raycast_component.get(); }
    /**@}*/

    /**
     * @name TagComponent
     * Metadata tag
     */
    template <typename T>
    bool has_tag_of_type() const { return m_tag.type() == typeid(T); }
    void set_tag(const boost::any& tag) { m_tag = tag; }
    boost::any tag() const { return m_tag; }
    /**@}*/

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
