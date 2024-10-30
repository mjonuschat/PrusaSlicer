#include "Slic3r/App/Plater/QuickDragGizmo.hpp"

namespace Slic3r::App::Plater {

GizmoActivationState QuickDragGizmo::on_mouse(const GizmoEventContext& ctx, bool only_active)
{
    return GizmoActivationState::Inactive;
}
}
