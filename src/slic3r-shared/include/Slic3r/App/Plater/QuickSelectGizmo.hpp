#pragma once

#include <chrono>

#include "Slic3r/App/Render/Geometry.hpp"
#include "Slic3r/App/Plater/IGizmo.hpp"
#include "Slic3r/App/Plater/SceneNodeTag.hpp"
#include "Slic3r/App/Plater/SelectionHandler.hpp"
#include "Slic3r/App/Plater/ISceneProvider.hpp"
#include "Slic3r/Biz/Scene/SceneInteractor.hpp"

namespace Slic3r::App::Plater {

class QuickSelectGizmo : public IGizmo {
public:
    QuickSelectGizmo(
        Biz::Scene::SceneInteractor& scene_interactor,
        Render::Device& device,
        const Render::ScreenInfo& screen_info
    )
        : m_scene_interactor(scene_interactor)
        , m_device(device)
        , m_selection_handler(scene_interactor)
        , m_selection_rect(device, Render::BufferUsage::DynamicDraw)
        , m_screen_info(screen_info)
    {}

    GizmoActivationState on_mouse(const GizmoEventContext& ctx, bool only_active) override;
    void on_cycle_prepare() override { m_processing = false; }

    void render_scene(Render::CommandBuffer& cmd_buffer) override;

private:
    void update_selection_rect(float x, float y, float w, float h);

private:
    using Clock = std::chrono::steady_clock;
    using TimePoint = std::chrono::time_point<Clock>;

    Biz::Scene::SceneInteractor& m_scene_interactor;
    Render::Device& m_device;
    const Render::ScreenInfo& m_screen_info;
    SelectionHandler m_selection_handler;
    TimePoint m_click_start;

    Render::Geometry m_selection_rect;
    bool m_selection_rect_shown{false};

    bool m_processing{false};
};

}
