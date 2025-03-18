#pragma once

#include "Slic3r/App/Scene/Scene.hpp"
#include "Slic3r/App/Scene/SceneChangeSession.hpp"
#include "Slic3r/App/Render/GeometryManager.hpp"
#include "Slic3r/App/Scene/TriangleMeshManager.hpp"
#include "Slic3r/App/Scene/NodeBuilder.hpp"
#include "Slic3r/App/ObjectList.hpp"

namespace Slic3r::App::Scene {

template <typename T>
class ScenePresenterProjectContext {
public:
    using ModelGeometryManager = Render::GeometryManager<T>;
    using ModelTriangleMeshManager = TriangleMeshManager<T>;

    ScenePresenterProjectContext()
    : m_scene(new Scene())
    , m_selection_scene_change_session(*m_scene)
    {
        initialize_selection_root();
        m_scene->add_child(m_selection_root);
    }

    ScenePresenterProjectContext(const ScenePresenterProjectContext<T>&) = delete;
    ScenePresenterProjectContext& operator=(const ScenePresenterProjectContext<T>&) = delete;

    ScenePresenterProjectContext(ScenePresenterProjectContext<T>&&) = default;

    Scene& scene() { return *m_scene; }
    const Scene& scene() const { return *m_scene; }

    Node& selection_root() { return *m_selection_root; }
    const Node& selection_root() const { return *m_selection_root; }

    SceneChangeSession& selection_scene_changes()
    {
        return m_selection_scene_change_session;
    }

    const Eigen::AlignedBox3f& selection_bounding_box() const { return m_selection_bounding_box; }
    void set_selection_bounding_box(const Eigen::AlignedBox3f& bounding_box)
    {
        m_selection_bounding_box = bounding_box;
    }

    ModelGeometryManager& model_geometry_manager() { return m_model_geometry_manager; }
    ModelTriangleMeshManager& model_triangle_mesh_manager() { return m_model_triangle_mesh_manager; }

    double screen_space_sized_modifier() const { return 0.0075; }

    ObjectList* object_list() { return &m_object_list; }
    const ObjectList& object_list() const { return m_object_list; }

private:
    void initialize_selection_root() {
        NodeBuilder builder(*m_scene);
        m_selection_root = builder
            .set_debug_name("selection_root")
            .set_screen_space_sized_modifier(screen_space_sized_modifier())
            .build().release();
        m_scene->add_child(m_selection_root);
    }

private:
    std::unique_ptr<Scene> m_scene;
    SceneChangeSession m_selection_scene_change_session;
    ModelGeometryManager m_model_geometry_manager;
    ModelTriangleMeshManager m_model_triangle_mesh_manager;
    Node* m_selection_root{nullptr};
    Eigen::AlignedBox3f m_selection_bounding_box;
    ObjectList m_object_list;
};

} // namespace Slic3r::App::Scene
