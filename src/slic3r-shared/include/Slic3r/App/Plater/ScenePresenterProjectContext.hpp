#pragma once

#include "Slic3r/App/Scene/Scene.hpp"
#include "Slic3r/App/Scene/SceneChangeSession.hpp"
#include "Slic3r/App/Render/GeometryManager.hpp"
#include "Slic3r/App/Scene/TriangleMeshManager.hpp"
#include "Slic3r/App/Plater/GeometryElementId.hpp"

namespace Slic3r::App::Plater {

using ModelGeometryManager = Render::GeometryManager<GeometryElementId>;
using ModelTriangleMeshManager = Scene::TriangleMeshManager<GeometryElementId>;

class ScenePresenterProjectContext {
public:
    ScenePresenterProjectContext()
        : m_scene(new Scene::Scene()), m_selection_scene_change_session(*m_scene)
    {}

    ScenePresenterProjectContext(const ScenePresenterProjectContext&) = delete;
    ScenePresenterProjectContext& operator=(const ScenePresenterProjectContext&) = delete;

    ScenePresenterProjectContext(ScenePresenterProjectContext&&) = default;

    Scene::Scene& scene() { return *m_scene; }
    const Scene::Scene& scene() const { return *m_scene; }
    Scene::SceneChangeSession& selection_scene_changes()
    {
        return m_selection_scene_change_session;
    }

    ModelGeometryManager& model_geometry_manager() { return m_model_geometry_manager; }
    ModelTriangleMeshManager& model_triangle_mesh_manager() { return m_model_triangle_mesh_manager; }

private:
    std::unique_ptr<Scene::Scene> m_scene;
    Scene::SceneChangeSession m_selection_scene_change_session;
    ModelGeometryManager m_model_geometry_manager;
    ModelTriangleMeshManager m_model_triangle_mesh_manager;
};
}
