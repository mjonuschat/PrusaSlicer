#pragma once
#include <chrono>
#include <optional>
#include <string>
#include <Slic3r/App/Platform/MouseEvent.hpp>
#include <Slic3r/App/Scene/Scene.hpp> // NodePickResults
#include <Slic3r/App/Scene/Ray.hpp>
#include <Slic3r/App/Scene/GizmoEventContext.hpp>
#include "Slic3r/App/Scene/IGizmo.hpp"

namespace Slic3r::App::Scene {
/// <summary>
/// Current state of dragging for button
/// </summary>
enum class DragState
{
    NoDrag,
    StartWeWillSee, // mouse click, record mouse event
    Dragging // mouse move with button down
};
std::string to_string(DragState state);

/// <summary>
/// Keep information about place of drag start
/// </summary>
struct DragStart
{
    // NOTE: GizmoEventContext contain only reference on mouse event
    Platform::MouseEvent mouse_event;
    Ray pick_ray;
    NodePickResults pick_results;
    Render::ScreenInfo screen_info; // need make a copy

    DragStart(const GizmoEventContext& ctx) :
        mouse_event(ctx.mouse_event()),
        pick_ray(ctx.pick_ray()),
        pick_results(ctx.pick_results()),
        screen_info(ctx.screen_info())
    {}

    const GizmoEventContext create_ctx() const
    {
        return GizmoEventContext{mouse_event, pick_ray, pick_results, screen_info};
    }
};

class IMouseDragCallbacks{
public:
    virtual ~IMouseDragCallbacks() = default;
    
    using OnStart = std::function<bool(const GizmoEventContext&)>;
    virtual void add_on_start(IGizmo* gizmo, OnStart callback) = 0;
    
    using OnDrag = std::function<bool(const GizmoEventContext&)>;
    virtual void add_on_drag(IGizmo* gizmo, OnDrag callback) = 0;

    using OnFinish = std::function<void()>;
    virtual void add_on_finish(IGizmo* gizmo, OnFinish callback) = 0;

    using OnCancel = std::function<void()>;
    virtual void add_on_cancel(IGizmo* gizmo, OnCancel callback) = 0;
};

/// <summary>
/// Only one in the application(centralized place for dragging detection),
/// Sniff mouse events from OS provide Drag object
/// NOTE: Consumers get only const reference to this object
/// </summary>
class MouseDragDetector: public IMouseDragCallbacks
{
public:
    MouseDragDetector(int min_time_span, int min_offset);
    
    // overrides of the IMouseDragCallbacks
    void add_on_start(IGizmo* gizmo, IMouseDragCallbacks::OnStart callback) override;
    void add_on_drag(IGizmo* gizmo, IMouseDragCallbacks::OnDrag callback) override;
    void add_on_finish(IGizmo* gizmo, IMouseDragCallbacks::OnFinish callback) override;
    void add_on_cancel(IGizmo* gizmo, IMouseDragCallbacks::OnCancel callback) override;

    /// <summary>
    /// Change state by mouse events to detect draging by mouse events
    /// </summary>
    /// <param name="ctx">Mouse event + casted ray</param>
    /// <param name="gizmos">Current active gizmos</param>
    bool mouse_event(const GizmoEventContext& ctx, const std::vector<IGizmo*>& gizmos);

    /// <summary>
    /// Way to cancel draging e.g. on ESC key
    /// But state is no changed in mouse event
    /// </summary>
    void cancel_drag_event();

private:
    bool can_start_drag();
    bool on_start(const std::vector<IGizmo*>& gizmos);

    // minimal time from button down to validate it as drag in miliseconds
    int m_min_time_span; // [in ms]
    // minimal mouse offset from button down to validate it is a drag
    // unit is micrometers of real world display size
    int m_min_offset; // [in um]

    std::chrono::time_point<std::chrono::steady_clock> m_start_time;

    bool m_right_down  = false;
    bool m_middle_down = false;

    // keep state of left mouse button drag
    DragState m_state = DragState::NoDrag;
    // keep data from drag start
    std::optional<DragStart> m_start = std::nullopt;

    // set on_start, discard on finish OR cancel
    const IGizmo* m_dragging_gizmo = nullptr;

    // sorted vector by pointer on IGizmo*
    std::vector<std::pair<IGizmo*, IMouseDragCallbacks::OnStart>> m_on_starts;
    std::vector<std::pair<IGizmo*, IMouseDragCallbacks::OnDrag>> m_on_drags;
    std::vector<std::pair<IGizmo*, IMouseDragCallbacks::OnFinish>> m_on_finishes;
    std::vector<std::pair<IGizmo*, IMouseDragCallbacks::OnCancel>> m_on_cancels;
};
} // namespace Slic3r::App::Scene
