#pragma once
#include <chrono>
#include <optional>
#include <string>
#include <functional>
#include <Slic3r/App/Platform/MouseEvent.hpp>
#include <Slic3r/App/Scene/Scene.hpp> // NodePickResults
#include <Slic3r/App/Scene/Ray.hpp>
#include <Slic3r/App/Scene/GizmoEventContext.hpp>
#include "Slic3r/App/Scene/IGizmo.hpp"

/// <summary>
/// Way to unify drag conditions(time or offset).
/// On detection of start dragging (MouseEvent::Move)
/// is selected first gizmo from active list to process dragging operation(dragging + finish(cancel) evenet)
/// </summary>

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

class IMouseDrag
{
public:
    virtual ~IMouseDrag()                                = default;
    virtual bool on_drag_start(const GizmoEventContext&) = 0;
    virtual bool on_dragging(const GizmoEventContext&)   = 0;
    virtual void on_drag_finish()                        = 0;
    virtual void on_drag_cancel()                        = 0;
};

/// <summary>
/// Only one in the application(centralized place for dragging detection),
/// Sniff mouse events from OS provide Drag object
/// NOTE: Consumers get only const reference to this object
/// </summary>
class MouseDragDetector
{
public:
    MouseDragDetector(int min_time_span, int min_offset);

    /// <summary>
    /// Internaly detect if gizmo implement IMouseDrag interface
    /// When yes add to sorted listener list
    /// </summary>
    /// <param name="gizmo"></param>
    void add_listener(IGizmo* gizmo);
    void rem_listener(IGizmo* gizmo);

    /// <summary>
    /// Change state by mouse events to detect draging by mouse events
    /// </summary>
    /// <param name="ctx">Mouse event + casted ray</param>
    /// <param name="gizmos">Current active gizmos</param>
    using GetActiveGizmos = std::function<std::vector<IGizmo*>()>;
    bool mouse_event(const GizmoEventContext& ctx, GetActiveGizmos get_gizmos);

    /// <summary>
    /// Way to cancel draging e.g. on ESC key
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

    // keep state of left mouse button drag
    DragState m_state = DragState::NoDrag;
    // keep data from drag start
    std::optional<DragStart> m_start = std::nullopt;

    // set on_start, discard on finish OR cancel
    IMouseDrag* m_dragging = nullptr;

    using Listener  = std::pair<IGizmo*, IMouseDrag*>;
    using Listeners = std::vector<Listener>;
    // sorted by IGizmo pointer
    Listeners m_listeners;
};
} // namespace Slic3r::App::Scene
