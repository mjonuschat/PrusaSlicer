#pragma once

#include "Slic3r/App/Scene/Scene.hpp"
#include "Slic3r/App/Scene/SceneChangeSession.hpp"
#include "Slic3r/App/Render/GeometryManager.hpp"
#include "Slic3r/App/Scene/TriangleMeshManager.hpp"
#include "Slic3r/App/Plater/AuxiliaryElementId.hpp"
#include "Slic3r/App/Scene/NodeBuilder.hpp"

namespace Slic3r::App::Plater {

using ModelGeometryManager = Render::GeometryManager<AuxiliaryElementId>;
using ModelTriangleMeshManager = Scene::TriangleMeshManager<AuxiliaryElementId>;

class ScenePresenterProjectContext {
public:
    ScenePresenterProjectContext()
        : m_scene(new Scene::Scene())
        , m_selection_scene_change_session(*m_scene)
    {
        Scene::NodeBuilder builder(*m_scene);
        m_selection_root = builder
            .set_debug_name("selection_root")
            .set_screen_space_sized_modifier(0.01)
            .build().release();
        m_scene->add_child(m_selection_root);
    }

    ScenePresenterProjectContext(const ScenePresenterProjectContext&) = delete;
    ScenePresenterProjectContext& operator=(const ScenePresenterProjectContext&) = delete;

    ScenePresenterProjectContext(ScenePresenterProjectContext&&) = default;

    Scene::Scene& scene() { return *m_scene; }
    const Scene::Scene& scene() const { return *m_scene; }

    Scene::Node& selection_root() { return *m_selection_root; }
    const Scene::Node& selection_root() const { return *m_selection_root; }

    Scene::SceneChangeSession& selection_scene_changes()
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

private:
    std::unique_ptr<Scene::Scene> m_scene;
    Scene::SceneChangeSession m_selection_scene_change_session;
    ModelGeometryManager m_model_geometry_manager;
    ModelTriangleMeshManager m_model_triangle_mesh_manager;
    Scene::Node* m_selection_root{nullptr};
    Eigen::AlignedBox3f m_selection_bounding_box;
};
}
