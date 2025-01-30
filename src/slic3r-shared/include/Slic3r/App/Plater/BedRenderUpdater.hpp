#pragma once

#include "Slic3r/App/Plater/ISceneProvider.hpp"
#include "Slic3r/App/Scene/Camera.hpp"
#include "Slic3r/Biz/ISelectedProjectChangedListener.hpp"

namespace Slic3r::Domain {
class Project;
class Workbench;
} // namespace Slic3r::Domain

namespace Slic3r::App::Plater {

class BedRenderUpdater : public Scene::ICameraUpdateListener,
                         public Biz::ISelectedProjectChangedListener
{
public:
    BedRenderUpdater(
        ISceneProvider& scene_provider, const Domain::Workbench& workbench, Render::Device& device
    )
        : m_scene_provider(scene_provider), m_workbench(workbench), m_device(device)
    {}

    /**
     * @brief Performs all updates
     */
    void update_all()
    {
        update_materials();
        update_positions();
        update_elements_state();
    }

    /**
      * @brief Updates beds' materials in dependence of the scene status
      */
    void update_materials();

    /**
      * @brief Updates beds' position in scene
      */
    void update_positions();

    /**
      * @brief Updates beds' elements state
      */
    void update_elements_state();

    /**
      * @brief Implementation of Scene::ICameraUpdateListener interface
      */
    void camera_updated(const Scene::Camera& cam) override;

    /**
      * @brief Implementation of Biz::ISelectedProjectChangedListener interface
      */
    void on_selected_project_changed(size_t index) override;

private:
    ISceneProvider& m_scene_provider;
    const Domain::Workbench& m_workbench;
    Render::Device& m_device;
    Domain::Project* m_project{ nullptr };
};

} // namespace Slic3r::App::Plater
