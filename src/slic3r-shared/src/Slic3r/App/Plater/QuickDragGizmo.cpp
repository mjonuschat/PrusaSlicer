#include "Slic3r/App/Plater/QuickDragGizmo.hpp"
#include "Slic3r/App/Plater/SceneNodeTag.hpp"

namespace Slic3r::App::Plater {

GizmoActivationState QuickDragGizmo::on_mouse(const GizmoEventContext& ctx, bool only_active)
{
    const auto& e = ctx.mouse_event();
    if (e.key_modifiers() != 0 || e.button() != Platform::MouseButton::Left)
        return GizmoActivationState::Inactive;

    if (e.type() == Platform::MouseEvent::Type::ButtonDown) {
        auto* n = ctx.pick_result_node_with_tag_of_type<SceneNodeTag>();
        if (n == nullptr)
            return GizmoActivationState::Inactive;

        return GizmoActivationState::Probing;
    } else if (e.type() == Platform::MouseEvent::Type::Move) {
        return GizmoActivationState::Inactive;
    } else if (e.type() == Platform::MouseEvent::Type::ButtonUp) {
        return GizmoActivationState::Inactive;
    } else {
        return GizmoActivationState::Inactive;
    }
}

} // namespace Slic3r::App::Plater
