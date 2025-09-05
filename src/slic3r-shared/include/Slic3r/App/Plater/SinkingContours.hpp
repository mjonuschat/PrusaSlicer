#pragma once

#include "Slic3r/Domain/ElementRef.hpp"
#include "Slic3r/App/Render/GeometryManager.hpp"
#include "Slic3r/App/Plater/SinkingContoursHelper.hpp"

namespace Slic3r::App::Platform {
class MouseEvent;
} // namespace Slic3r::App::Platform

namespace Slic3r::Domain {
class Project;
} // namespace Slic3r::Domain

namespace Slic3r::App::Scene {
class Scene;
} // namespace Slic3r::App::Scene

namespace Slic3r::App::Render {
class Device;
class ScreenInfo;
} // namespace Slic3r::App::Render

namespace Slic3r::Biz::Scene {
struct ObjectSelection;
} // namespace Slic3r::Biz::Scene

namespace Slic3r::App::Plater {

class SinkingContours
{
public:
    using ModelGeometryManager = Render::GeometryManager<SinkingAuxiliaryElementId>;

    void update_scene(Render::Device& device, const Domain::Project& project, Scene::Scene& scene, const Domain::ElementRefs& elements);
    void update_visibility(const Platform::MouseEvent& e, const Render::ScreenInfo& screen_info, Scene::Scene& scene);

    void set_selection(Biz::Scene::ObjectSelection* selection) { m_selection = selection; }

private:
    ModelGeometryManager m_model_geometry_manager;
    Biz::Scene::ObjectSelection* m_selection{ nullptr };
};

} // namespace Slic3r::App::Plater
