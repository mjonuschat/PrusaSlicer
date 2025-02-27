#pragma once

#include "Slic3r/App/Scene/IGizmo.hpp"
#include "Slic3r/App/Scene/ISceneProvider.hpp"
#include "Slic3r/Biz/Scene/SceneInteractor.hpp"

namespace Slic3r::App::Plater {

class BedSelectGizmo : public Scene::IGizmo
{
public:
    BedSelectGizmo(
        Biz::Scene::SceneInteractor& scene_interactor,
        Scene::ISceneProvider& scene_provider
    )
        : m_scene_interactor(scene_interactor)
        , m_scene_provider(scene_provider)
    {}

    Scene::GizmoActivationState on_mouse(Scene::GizmoEventContext& ctx, bool only_active) override;

private:
    Biz::Scene::SceneInteractor& m_scene_interactor;
    Scene::ISceneProvider& m_scene_provider;
};

} // namespace Slic3r::App::Plater
