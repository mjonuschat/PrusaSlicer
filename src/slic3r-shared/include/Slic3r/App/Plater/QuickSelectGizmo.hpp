#pragma once

#include <chrono>

#include "Slic3r/App/Plater/IGizmo.hpp"
#include "Slic3r/App/Plater/SceneNodeTag.hpp"
#include "Slic3r/App/Scene/SceneChangeSession.hpp"

namespace Slic3r::App::Plater {

class QuickSelectGizmo : public IGizmo {
public:
    explicit QuickSelectGizmo(Scene::Scene& scene)
        : m_selection_scene_change_session(scene)
    {}
    GizmoActivationState on_mouse(const GizmoEventContext& ctx, bool only_active) override;
private:
    void mark_selected(Scene::Node& n);
    void clear_selection();
private:
    using Clock = std::chrono::steady_clock;
    using TimePoint = std::chrono::time_point<Clock>;

    TimePoint m_click_start;

    bool m_processing{false};
    SceneNodeTag m_selected_node;
    Scene::SceneChangeSession m_selection_scene_change_session;
};

}
