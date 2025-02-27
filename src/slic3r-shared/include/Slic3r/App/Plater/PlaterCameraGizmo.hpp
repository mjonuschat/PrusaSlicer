#pragma once

#include "Slic3r/App/Scene/AbstractCameraGizmo.hpp"

namespace Slic3r::App::Plater {

class PlaterCameraGizmo : public Scene::AbstractCameraGizmo
{
public:
    explicit PlaterCameraGizmo(Scene::ISceneProvider& scene_provider)
        : Scene::AbstractCameraGizmo(scene_provider) {}

private:
    virtual bool any_draggable(Scene::GizmoEventContext& ctx) const override;
};

} // namespace Slic3r::App::Plater
