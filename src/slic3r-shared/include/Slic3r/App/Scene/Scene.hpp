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

class ISceneRenderCustomizer
{
public:
    virtual ~ISceneRenderCustomizer() = default;

    virtual void on_render_begin(Render::CommandBuffer& cmd_buf) {}
    virtual void on_layer_begin(Render::CommandBuffer& cmd_buf, size_t layer_idx) {}
    virtual void on_opaque_pass_begin(Render::CommandBuffer& cmd_buf, size_t layer_index) {}
    virtual void on_opaque_pass_end(Render::CommandBuffer& cmd_buf, size_t layer_index) {}
    virtual void on_transparent_pass_begin(Render::CommandBuffer& cmd_buf, size_t layer_index) {}
    virtual void on_transparent_pass_end(Render::CommandBuffer& cmd_buf, size_t layer_index) {}
    virtual void on_layer_end(Render::CommandBuffer& cmd_buf, size_t layer_idx) {}
    virtual void on_render_end(Render::CommandBuffer& cmd_buf) {}
};

class MinimalSceneRenderCustomizer : public ISceneRenderCustomizer
{
public:
    void on_render_begin(Render::CommandBuffer& cmd_buf) override;
    void on_layer_begin(Render::CommandBuffer& cmd_buf, size_t layer_idx) override {}
    void on_opaque_pass_begin(Render::CommandBuffer& cmd_buf, size_t layer_index) override;
    void on_opaque_pass_end(Render::CommandBuffer& cmd_buf, size_t layer_index) override {}
    void on_transparent_pass_begin(Render::CommandBuffer& cmd_buf, size_t layer_index) override;
    void on_transparent_pass_end(Render::CommandBuffer& cmd_buf, size_t layer_index) override;
    void on_layer_end(Render::CommandBuffer& cmd_buf, size_t layer_idx) override {}
    void on_render_end(Render::CommandBuffer& cmd_buf) override {}

};

enum class SceneRenderFlag : uint32_t
{
    None    = 0x0000,
    Shadows = 0x0001,
};

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
    Scene();

    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;

    Scene(Scene&&) = default;

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
    void render(Render::Device& device, Render::CommandBuffer& cmd_buffer, ISceneRenderCustomizer* customizer = &ms_default_customizer,
        SceneRenderFlag flags = SceneRenderFlag::None) const;
    void render_imgui(const Render::ScreenInfo& screen_info) const;
    /** @} */

    /**
     * @name Pick all nodes under cursor.
     * @{
     */
    /**
     * @brief Pick immutable nodes under mouse cursor.
     *
     * @param mouse_x, mouse_y Mouse cursor position (in logical coords)
     * @param [out] results List of nodes under cursor sorted by distance from camera (nearest first).
     * @param [out] out_ray If passed as non-`nullptr`, the ray associated with mouse cursor is set.
     * @return True if there is any hit, otherwise false.
     */
    bool pick_at(float mouse_x, float mouse_y, ConstNodePickResults& results, Ray* out_ray = nullptr) const;

    /**
     * @brief Pick mutable nodes under mouse cursor.
     *
     * @param mouse_x, mouse_y Mouse cursor position (in logical coords)
     * @param [out] results List of nodes under cursor sorted by distance from camera (nearest first).
     * @param [out] out_ray If passed as non-`nullptr`, the ray associated with mouse cursor is set.
     * @return True if there is any hit, otherwise false.
     */
    bool pick_at(float mouse_x, float mouse_y, NodePickResults& results, Ray* out_ray = nullptr);
    /** @} */

    /**
     * @name Modify hierarchy
     * @{
     */
    /**
     * @brief Add node under root or given parent if specified.
     *
     * @note Scene takes over ownership of the @p node. The @p node gets deleted if its parent
     * or whole is deleted.
     *
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

    /**
     * @name Shadows-related methods
     * @{
     */

    bool shadows_enabled() const { return m_shadows.enabled; }
    void set_shadows_enabled(bool enable) { m_shadows.enabled = enable; }

    bool bed_model_cast_shadow() const { return m_shadows.bed_model_cast_shadow; }
    void set_bed_model_cast_shadow(bool cast) { m_shadows.bed_model_cast_shadow = cast; }

    int shadowsmap_size() const { return m_shadows.shadowsmap_size; }
    void set_shadowsmap_size(int size) { m_shadows.pending_shadowsmap_size = size; }

    void set_bed_aabb(const Eigen::AlignedBox3d& aabb) { m_shadows.bed_aabb = aabb; }

    /** @} */

    void log_nodes() const;
private:
    void register_node(Node* n);
    void unregister_node(Node* n);

    using NodeMaterial = std::pair<const Node*, Render::Material>;
    using NodeMaterials = std::vector<NodeMaterial>;
    NodeMaterials collect_nodes_with_material(const Node::NodePredicate& predicate) const;

    void render_shadowsmap_pass(Render::Device& device) const;
    void render_shadows_receivers_pass(Render::Device& device, Render::CommandBuffer& cmd_buffer, ISceneRenderCustomizer* customizer) const;
    void render_no_shadows_pass(Render::CommandBuffer& cmd_buffer, ISceneRenderCustomizer* customizer) const;

private:
    using NodeIdLookUp = std::unordered_map<size_t, Node*>;

    Node m_root;
    Camera m_camera;
    CameraTrackballController m_camera_trackball;
    NodeIdLookUp m_nodes_by_id;
    Render::GeometryManager<std::string> m_geometry_manager;
    TriangleMeshManager<std::string> m_trimesh_manager;

    struct Shadows
    {
        bool enabled{ true };
        bool bed_model_cast_shadow{ true };
        Eigen::AlignedBox3d bed_aabb;

        mutable Render::Framebuffer* shadowsmap_framebuffer{ nullptr };
        mutable int shadowsmap_size{ 0 };
        mutable std::optional<int> pending_shadowsmap_size;
        mutable Camera light_cam;

        static constexpr int DEFAULT_SHADOWSMAP_SIZE = 2048;
        static constexpr int SHADOWSMAP_TEXTURE_UNIT = 15;
    };

    Shadows m_shadows;

    static MinimalSceneRenderCustomizer ms_default_customizer;
};

}

