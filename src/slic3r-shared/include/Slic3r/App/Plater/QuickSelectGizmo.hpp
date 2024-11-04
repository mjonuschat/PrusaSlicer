#pragma once

#include <chrono>

#include "Slic3r/App/Plater/IGizmo.hpp"
#include "Slic3r/App/Plater/SceneNodeTag.hpp"
#include "Slic3r/App/Scene/SceneChangeSession.hpp"
#include "Slic3r/Biz/Scene/SceneInteractor.hpp"

namespace Slic3r::App::Plater {

class QuickSelectGizmo : public IGizmo {
public:
    QuickSelectGizmo(Biz::Scene::SceneInteractor& scene_interactor, Scene::Scene& scene)
        : m_scene_interactor(scene_interactor), m_selection_scene_change_session(scene)
    {}
    GizmoActivationState on_mouse(const GizmoEventContext& ctx, bool only_active) override;
private:
    void mark_selected(Scene::Node& n, bool replace=true);
    void mark_unselected(Scene::Node& n);
    void clear_selection();
private:
    using Clock = std::chrono::steady_clock;
    using TimePoint = std::chrono::time_point<Clock>;

    Biz::Scene::SceneInteractor& m_scene_interactor;
    TimePoint m_click_start;

    bool m_processing{false};
    Scene::SceneChangeSession m_selection_scene_change_session;
};

}
