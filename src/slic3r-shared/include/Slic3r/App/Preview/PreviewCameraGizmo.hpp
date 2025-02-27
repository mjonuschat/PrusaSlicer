#pragma once

#include "Slic3r/App/Scene/AbstractCameraGizmo.hpp"

namespace Slic3r::App::Preview {

class PreviewCameraGizmo : public Scene::AbstractCameraGizmo
{
public:
    explicit PreviewCameraGizmo(Scene::ISceneProvider& scene_provider)
        : Scene::AbstractCameraGizmo(scene_provider) {}

private:
    virtual bool any_draggable(Scene::GizmoEventContext& ctx) const override { return false; }
};

} // namespace Slic3r::App::Preview