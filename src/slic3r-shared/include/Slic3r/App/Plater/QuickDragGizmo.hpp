#pragma once

#include "libslic3r/Point.hpp"
#include "Slic3r/App/Plater/IGizmo.hpp"
#include "Slic3r/Biz/Scene/SceneInteractor.hpp"
#include "Slic3r/App/Plater/ISceneProvider.hpp"

namespace Slic3r::App::Plater {

class QuickDragGizmo : public IGizmo {
public:
    QuickDragGizmo(Biz::Scene::SceneInteractor& scene_interactor, ISceneProvider& scene_provider)
        : m_scene_interactor(scene_interactor), m_scene_provider(scene_provider)
    {}

    GizmoActivationState on_mouse(const GizmoEventContext& ctx, bool only_active) override;

private:
    Biz::Scene::SceneInteractor& m_scene_interactor;
    ISceneProvider& m_scene_provider;
    Vec2f m_initial_mouse_pos;
};

}
