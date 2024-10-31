#pragma once

#include <vector>
#include <unordered_map>
#include <boost/variant/variant.hpp>

#include "Slic3r/App/Scene/Node.hpp"
#include "Slic3r/App/Scene/Camera.hpp"
#include "Slic3r/App/Scene/CameraTrackballController.hpp"
#include "Slic3r/App/Scene/TriangleMeshManager.hpp"
#include "Slic3r/App/Render/GeometryManager.hpp"
#include "Slic3r/App/Render/Shader.hpp"
#include "Slic3r/App/Render/ScreenInfo.hpp"

#include "libslic3r/Color.hpp"
#include "libslic3r/Geometry.hpp"
#include "libslic3r/AABBMesh.hpp"


namespace Slic3r::App::Scene {

struct NodePickResult
{
    Node* node;
    double t;
};

struct ConstNodePickResult
{
    const Node* node;
    double t;
};

using NodePickResults = std::vector<NodePickResult>;
using ConstNodePickResults = std::vector<ConstNodePickResult>;

/**
 * @brief Scenegraph entrypoint
 *
 * Encapsulate scene tree made of Node and provides:
 * - rendering of 3D object, see render()
 * - rendering of 2D GUI overlay, see render_imgui()
 * - camera used for rendering (camera()) and its trackball manipulator (camera_trackball())
 * - ray picking, see pick_at()
 * - also provides common resource caching manager for rendering geometry (geometry_manager()),
 *   and in-memory geometry (triangle_mesh_manager())
 * .
 */
class Scene final
{
public:
    Scene() : m_camera_trackball(m_camera) { m_nodes_by_id[m_root.id()] = &m_root; }

    /**
     * @name Access to root
     * @{
     */
    Node& root() { return m_root; }
    const Node& root() const { return m_root; }
    /** @} */

    /**
     * @name Node look-up by ID
     * @{
     */
    /**
     * @brief Quick mutable node look up by id.
     *
     * Gets node by its id.
     *
     * @param id Node id (see Node::id(), Node::set_id())
     * @return Pointer to node with given id or `nullptr` if no such node present in scene.
     */
    Node* node(size_t id)
    {
        auto it = m_nodes_by_id.find(id);
        return it == m_nodes_by_id.end() ? nullptr : it->second;
    }

    /**
     * @brief Quick immutable node look up by id.
     *
     * Gets node by its id.
     *
     * @param id Node id (see Node::id(), Node::set_id())
     * @return Constant pointer to node with given id or `nullptr` if no such node present in scene.
     */
    const Node* node(size_t id) const
    {
        auto it = m_nodes_by_id.find(id);
        return it == m_nodes_by_id.end() ? nullptr : it->second;
    }
    /** @} */

    /**
     * @name Camera and its trackball controller
     * @{
     */
    Camera& camera() { return m_camera; }
    const Camera& camera() const { return m_camera; }

    const CameraTrackballController& camera_trackball() const { return m_camera_trackball; }
    CameraTrackballController& camera_trackball() { return m_camera_trackball; }
    /** @} */

    /**
     * @name Associated resource managers
     * @{
     */
    Render::GeometryManager<std::string>& geometry_manager() { return m_geometry_manager; }
    TriangleMeshManager<std::string>& triangle_mesh_manager() { return m_trimesh_manager; }
    /** @} */

    /**
     * @name Rendering
     * @{
     */
    void render(Render::CommandBuffer& cmd_buffer) const;
    void render_imgui(const Render::ScreenInfo& screen_info) const;
    /** @} */

    /**
     * @name Pick all nodes under cursor.
     * @{
     */
    bool pick_at(float mouse_x, float mouse_y, ConstNodePickResults& results) const;
    bool pick_at(float mouse_x, float mouse_y, NodePickResults& results);
    /** @} */

    /**
     * @name Modify hierarchy
     * @{
     */
    /**
     * @brief Add node under root or given parent if specified.
     * @param node Node to be added as child.
     * @param parent Optional parent node, if not specified, @p node is added under scene root.
     */
    void add_child(Node* node, Node* parent = nullptr);

    /**
     * @brief Remove and destroy child node (or children nodes) satisfying @p predicate.
     *
     * @note Unlike detach_children() this method will destroy all children satisfying
     * @p predicate.
     *
     * @param predicate Predicate function to test node if it is supposed to be removed.
     * @param parent Optional parent node, if not specified the scene root is searched instead.
     * @return True if some nodes was destroyed otherwise false.
     */
    bool remove_children(const Node::NodePredicate& predicate, Node* parent = nullptr);

    /**
     * @brief Detach child node (or multiple nodes) satisfying @p predicate.
     *
     * @note Unlike remove_children() this method will return list of detached children without
     * destroying them.
     *
     * @param predicate Predicate function to test node if it is supposed to be detached.
     * @param parent Optional parent node, if not specified the scene root is searched instead.
     * @return List of detached nodes as `std::unique_ptr`, i.e. handing over ownership to the method caller.
     */
    Node::NodeOwningList detach_children(const Node::NodePredicate & predicate, Node* parent = nullptr);

    /** @} */
private:
    void register_node(Node* n);
    void unregister_node(Node* n);
private:
    using NodeIdLookUp = std::unordered_map<size_t, Node*>;

    Node m_root;
    Camera m_camera;
    CameraTrackballController m_camera_trackball;
    NodeIdLookUp m_nodes_by_id;
    Render::GeometryManager<std::string> m_geometry_manager;
    TriangleMeshManager<std::string> m_trimesh_manager;
};

}

