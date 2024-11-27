#pragma once

#include <chrono>

#include "Slic3r/App/Plater/IGizmo.hpp"
#include "Slic3r/App/Plater/SceneNodeTag.hpp"
#include "Slic3r/App/Plater/SelectionHandler.hpp"
#include "Slic3r/App/Plater/ISceneProvider.hpp"
#include "Slic3r/App/Scene/SceneChangeSession.hpp"
#include "Slic3r/Biz/Scene/SceneInteractor.hpp"

namespace Slic3r::App::Plater {

class QuickSelectGizmo : public IGizmo {
public:
    explicit QuickSelectGizmo(Biz::Scene::SceneInteractor& scene_interactor)
        : m_scene_interactor(scene_interactor)
        , m_selection_handler(scene_interactor)
    {}
    GizmoActivationState on_mouse(const GizmoEventContext& ctx, bool only_active) override;
    void on_cycle_prepare() override { m_processing = false; }
private:
    using Clock = std::chrono::steady_clock;
    using TimePoint = std::chrono::time_point<Clock>;

    Biz::Scene::SceneInteractor& m_scene_interactor;
    SelectionHandler m_selection_handler;
    TimePoint m_click_start;

    bool m_processing{false};
};

}
