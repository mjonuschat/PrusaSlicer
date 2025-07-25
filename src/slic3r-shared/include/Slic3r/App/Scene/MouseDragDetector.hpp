#pragma once
#include <chrono>
#include <optional>
#include <string>
#include <Slic3r/App/Platform/MouseEvent.hpp>
#include <Slic3r/App/Scene/Scene.hpp> // NodePickResults
#include <Slic3r/App/Scene/Ray.hpp>
#include <Slic3r/App/Scene/GizmoEventContext.hpp>

namespace Slic3r::App::Scene {
/// <summary>
/// Current state of dragging for button
/// </summary>
enum class DragState
{
    no_drag,
    start_we_will_see, // mouse click, record mouse event
    start_discard, // mouse clic was not a drag
    start, // positive draging condition
    dragging, // mouse move with button down
    finish, // mouse button up
    interupted // mouse move out of window OR esc key appear
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

/// <summary>
/// Only one in the application(centralized place for dragging detection),
/// Sniff mouse events from OS provide Drag object
/// NOTE: Consumers get only const reference to this object
/// </summary>
class MouseDragDetector
{
public:
    MouseDragDetector(int min_time_span, int min_offset);

    // common state scenario:
    // on left mouse button down          on next event after start
    // |       on condition to start drag  |      on left mouse button up
    // |                       |           |              |
    // (*)no_drag -> (N)start_we_will_see -> (1)start -> (*)dragging -> (1)finish -> (*)no_drag
    DragState get_state() const;
    bool is_dragging() const;
    const std::optional<DragStart> get_start() const;

    /// <summary>
    /// Change state by mouse events to detect draging by mouse events
    /// </summary>
    /// <param name="me">Mouse event - mouse coord and type of event</param>
    /// <param name="pick_ray">Stored in case of left button down(potentialy start draggig)</param>
    /// <param name="pick_results">Stored in case of left button down(potentialy start draggig)</param>
    void mouse_event(const GizmoEventContext& ctx);

    /// <summary>
    /// Way to cancel draging e.g. on ESC key
    /// But state is no changed in mouse event
    /// </summary>
    void cancel_drag_event();

private:
    bool can_start_drag();

    // minimal time from button down to validate it as drag in miliseconds
    int m_min_time_span; // [in ms]
    // minimal mouse offset from button down to validate it is a drag
    // unit is micrometers of real world display size
    int m_min_offset; // [in um]

    std::chrono::time_point<std::chrono::steady_clock> m_start_time;

    bool m_right_down  = false;
    bool m_middle_down = false;

    // keep state of left mouse button drag
    DragState m_state = DragState::no_drag;
    // keep data from drag start
    std::optional<DragStart> m_start = std::nullopt;
};
} // namespace Slic3r::App::Scene
