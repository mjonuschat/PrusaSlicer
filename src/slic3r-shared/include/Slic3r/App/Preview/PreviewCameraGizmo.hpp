#pragma once

#include "Slic3r/App/Scene/AbstractCameraGizmo.hpp"

namespace Slic3r::App::Preview {

class PreviewCameraGizmo : public Scene::AbstractCameraGizmo
{
public:
    PreviewCameraGizmo(const Domain::Workbench& workbench, Biz::ProjectInteractor& project_interactor,
        Scene::ISceneProvider& scene_provider)
        : Scene::AbstractCameraGizmo(workbench, project_interactor, scene_provider) {}

private:
    virtual bool any_draggable(Scene::GizmoEventContext& ctx) const override { return false; }
};

} // namespace Slic3r::App::Preview