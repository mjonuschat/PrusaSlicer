#pragma once

#include "Slic3r/App/Plater/ISceneProvider.hpp"
#include "Slic3r/App/Scene/Camera.hpp"

#include <libslic3r/Color.hpp>

namespace Slic3r::Domain {
class Project;
} // namespace Slic3r::Domain

namespace Slic3r::App::Plater {

/**
* @brief Bed colors
*/
static const Slic3r::ColorRGBA DEFAULT_BED_MODEL_COLOR  = { 0.25f, 0.25f, 0.25f, 1.0f };
static const Slic3r::ColorRGBA DISABLED_BED_MODEL_COLOR = { 0.5f, 0.5f, 0.5f, 1.0f };
static const Slic3r::ColorRGBA DEFAULT_BED_PLATE_COLOR  = { 0.225f, 0.225f, 0.225f, 1.0f };
static const Slic3r::ColorRGBA DISABLED_BED_PLATE_COLOR = { 0.425f, 0.425f, 0.425f, 1.0f };
static const Slic3r::ColorRGBA DEFAULT_BED_GRID_COLOR  = { 0.75f, 0.75f, 0.75f, 0.75f };
static const Slic3r::ColorRGBA DISABLED_BED_GRID_COLOR = { 0.65f, 0.65f, 0.65f, 0.75f };
static const Slic3r::ColorRGBA DEFAULT_BED_CONTOUR_COLOR  = { 0.9f, 0.9f, 0.9f, 1.0f };
static const Slic3r::ColorRGBA DISABLED_BED_CONTOUR_COLOR = { 0.75f, 0.75f, 0.75f, 1.0f };

class BedRenderUpdater : public Scene::ICameraUpdateListener
{
public:
    explicit BedRenderUpdater(ISceneProvider& scene_provider)
    : m_scene_provider(scene_provider)
    {}

    /**
      * @brief Performs all updates
      */
    void update_all(Render::Device& device, const Domain::Project& project) {
        update_materials(device, project);
        update_positions(project);
        update_elements_state(project);
    }

    /**
      * @brief Updates beds' materials in dependence of the scene status
      */
    void update_materials(Render::Device& device, const Domain::Project& project);

    /**
      * @brief Updates beds' position in scene
      */
    void update_positions(const Domain::Project& project);

    /**
      * @brief Updates beds' elements state
      */
    void update_elements_state(const Domain::Project& project);

    /**
      * @brief Implementation of Scene::ICameraUpdateListener interface
      */
    void camera_updated(const Scene::Camera& cam) override;

private:
    ISceneProvider& m_scene_provider;
};

} // namespace Slic3r::App::Plater
