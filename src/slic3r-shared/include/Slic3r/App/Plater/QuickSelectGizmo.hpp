#pragma once

#include <chrono>

#include "Slic3r/App/Render/Geometry.hpp"
#include "Slic3r/App/Plater/IGizmo.hpp"
#include "Slic3r/App/Plater/SceneNodeTag.hpp"
#include "Slic3r/App/Plater/SelectionHandler.hpp"
#include "Slic3r/App/Plater/ISceneProvider.hpp"
#include "Slic3r/Biz/Scene/SceneInteractor.hpp"

namespace Slic3r::App::Plater {

class RectangleSelection
{
public:
    enum class Type : uint8_t
    {
        Undefined,
        Replace,
        Add,
        Remove
    };

    RectangleSelection(const Render::ScreenInfo& screen_info, Render::Device& device)
        : m_screen_info(screen_info)
        , m_device(device)
        , m_geometry(device, Render::BufferUsage::DynamicDraw)
    {}

    void activate(Type type, const Vec2i& initial_mouse_pos) {
        m_active = true;
        m_type = type;
        m_initial_mouse_pos = initial_mouse_pos;
    }

    void deactivate() {
        m_active = false;
        m_defined = false;
    }

    [[nodiscard]] bool is_active() { return m_active; }

    void update(const Vec2i& curr_mouse_pos);
    [[nodiscard]] bool update_selection(SelectionHandler& selection_handler);

    void render(Render::CommandBuffer& cmd_buffer);

private:
    const Render::ScreenInfo& m_screen_info;
    Render::Device& m_device;

    Type m_type{ Type::Undefined };
    bool m_active{ false };
    bool m_defined{ false };
    Vec2i m_initial_mouse_pos;
    Render::Geometry m_geometry;
};

class QuickSelectGizmo : public IGizmo {
public:
    QuickSelectGizmo(
        Biz::Scene::SceneInteractor& scene_interactor,
        Render::Device& device,
        const Render::ScreenInfo& screen_info
    )
        : m_scene_interactor(scene_interactor)
        , m_selection_handler(scene_interactor)
        , m_rectangle_selection(screen_info, device)
    {}

    GizmoActivationState on_mouse(const GizmoEventContext& ctx, bool only_active) override;
    void on_cycle_prepare() override { m_processing = false; }

    void render_scene(Render::CommandBuffer& cmd_buffer) override;

private:
    using Clock = std::chrono::steady_clock;
    using TimePoint = std::chrono::time_point<Clock>;

    Biz::Scene::SceneInteractor& m_scene_interactor;
    SelectionHandler m_selection_handler;
    TimePoint m_click_start;

    bool m_processing{false};

    RectangleSelection m_rectangle_selection;
};

}
