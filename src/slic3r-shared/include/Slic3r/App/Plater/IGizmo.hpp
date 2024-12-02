#pragma once

#include "Slic3r/App/Plater/GizmoEventContext.hpp"

namespace Slic3r::App::Plater {

/**
 * @brief State of event procession for specific IGizmo.
 */
enum class GizmoActivationState {
    /// Event is not processed, no more event needed until next cycle
    Inactive,
    /// Gizmo is still probing if process the event, expects next event to come
    Probing,
    /// Gizmo is consuming the event
    Active,
    /// Gizmo finished event consuming (next cycle can start).
    Done
};

class IGizmo {
public:
    virtual ~IGizmo() = default;
    virtual GizmoActivationState on_mouse(const GizmoEventContext& ctx, bool only_active) = 0;
    virtual void on_cycle_prepare() {}
    virtual void render_scene(Render::CommandBuffer& cmd_buffer) {}
    virtual void render_imgui() {}
};


class IToolGizmo : public IGizmo {
public:
    virtual void on_activated() = 0;
    virtual void on_deactivated() = 0;
};

}
