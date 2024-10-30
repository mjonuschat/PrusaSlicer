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

    Node& root() { return m_root; }
    const Node& root() const { return m_root; }

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


    Camera& camera() { return m_camera; }
    const Camera& camera() const { return m_camera; }

    const CameraTrackballController& camera_trackball() const { return m_camera_trackball; }
    CameraTrackballController& camera_trackball() { return m_camera_trackball; }

    Render::GeometryManager<std::string>& geometry_manager() { return m_geometry_manager; }
    TriangleMeshManager<std::string>& triangle_mesh_manager() { return m_trimesh_manager; }

    void render(Render::CommandBuffer& cmd_buffer) const;
    void render_imgui(const Render::ScreenInfo& screen_info) const;
    bool pick_at(float mouse_x, float mouse_y, ConstNodePickResults& results) const;
    bool pick_at(float mouse_x, float mouse_y, NodePickResults& results);

    void add_child(Node* node, Node* parent = nullptr);
    bool remove_children(const Node::NodePredicate& predicate, Node* parent = nullptr);
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

