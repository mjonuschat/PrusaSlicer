#pragma once

#include "Slic3r/App/Plater/IGizmo.hpp"
#include "Slic3r/Biz/Scene/SceneInteractor.hpp"
#include "Slic3r/App/Plater/ISceneProvider.hpp"

namespace Slic3r::App::Plater {

class BedSelectGizmo : public IGizmo
{
public:
    BedSelectGizmo(
        Biz::Scene::SceneInteractor& scene_interactor,
        ISceneProvider& scene_provider
    )
        : m_scene_interactor(scene_interactor)
        , m_scene_provider(scene_provider)
    {}

    GizmoActivationState on_mouse(GizmoEventContext& ctx, bool only_active) override;

private:
    Biz::Scene::SceneInteractor& m_scene_interactor;
    ISceneProvider& m_scene_provider;
};

} // namespace Slic3r::App::Plater
