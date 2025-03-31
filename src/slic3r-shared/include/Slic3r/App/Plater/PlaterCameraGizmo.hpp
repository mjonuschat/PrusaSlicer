#pragma once

#include "Slic3r/App/Scene/AbstractCameraGizmo.hpp"

namespace Slic3r::App::Plater {

class PlaterCameraGizmo : public Scene::AbstractCameraGizmo
{
public:
    PlaterCameraGizmo(const Domain::Workbench& workbench, Scene::ISceneProvider& scene_provider)
        : Scene::AbstractCameraGizmo(workbench, scene_provider) {}

private:
    virtual bool any_draggable(Scene::GizmoEventContext& ctx) const override;
};

} // namespace Slic3r::App::Plater
