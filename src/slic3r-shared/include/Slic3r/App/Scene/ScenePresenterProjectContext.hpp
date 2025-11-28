#pragma once

#include "Slic3r/App/Scene/Scene.hpp"
#include "Slic3r/App/Scene/SceneChangeSession.hpp"
#include "Slic3r/App/Render/GeometryManager.hpp"
#include "Slic3r/App/Scene/TriangleMeshManager.hpp"
#include "Slic3r/App/Scene/NodeBuilder.hpp"
#include "Slic3r/App/Scene/AuxiliaryElementId.hpp"
#include "Slic3r/App/Scene/BedError.hpp"
#include "Slic3r/App/Platform/CameraSynchData.hpp"
#include "Slic3r/App/Scene/OrientedBoundingBox.hpp"

namespace Slic3r::App::Scene {

class ScenePresenterProjectContext
{
public:
    using ModelGeometryManager = Render::GeometryManager<AuxiliaryElementId>;
    using ModelTriangleMeshManager = TriangleMeshManager<AuxiliaryElementId>;

    ScenePresenterProjectContext();

    ScenePresenterProjectContext(const ScenePresenterProjectContext&) = delete;
    ScenePresenterProjectContext& operator=(const ScenePresenterProjectContext&) = delete;

    ScenePresenterProjectContext(ScenePresenterProjectContext&&) = default;

    Scene& scene() { return *m_scene; }
    const Scene& scene() const { return *m_scene; }

private: // Intialization order matters, hence this out of order private.
    std::unique_ptr<Scene> m_scene;
    SceneChangeSession m_selection_scene_change_session;

public:
    Node& selection_root;
    Node& plain_selection_root;
    std::optional<OrientedBoundingBox> selection_bounding_box;

    SceneChangeSession& selection_scene_changes()
    {
        return m_selection_scene_change_session;
    }

    const BedError& bed_error() const { return m_bed_error; }
    BedError& bed_error() { return m_bed_error; }

    ModelGeometryManager& model_geometry_manager() { return m_model_geometry_manager; }
    const ModelTriangleMeshManager& model_triangle_mesh_manager() const { return m_model_triangle_mesh_manager; }
    ModelTriangleMeshManager& model_triangle_mesh_manager() { return m_model_triangle_mesh_manager; }


    const std::optional<Platform::CameraSynchData>& camera_synch_data() const { return m_camera_synch_data; }
    void set_camera_synch_data(const Platform::CameraSynchData& data) { m_camera_synch_data = data; }



private:
    ModelGeometryManager m_model_geometry_manager;
    ModelTriangleMeshManager m_model_triangle_mesh_manager;
    std::optional<Platform::CameraSynchData> m_camera_synch_data;
    BedError m_bed_error;
};

} // namespace Slic3r::App::Scene
