#include "Slic3r/App/Plater/QuickSelectGizmo.hpp"
#include "Slic3r/App/Scene/Node.hpp"

namespace Slic3r::App::Plater {

GizmoActivationState QuickSelectGizmo::on_mouse(const GizmoEventContext& ctx, bool only_active)
{
    using namespace std::chrono_literals;
    if (ctx.mouse_event().button() != Platform::MouseButton::Left)
        return GizmoActivationState::Inactive;

    const auto type = ctx.mouse_event().type();

    constexpr static Clock::duration max_click_duration = Clock::duration {500ms};

    auto it =
        std::find_if(ctx.pick_results().begin(), ctx.pick_results().end(), [&](const auto& item) {
            const Scene::Node& n = *item.node;
            return n.has_tag_of_type<SceneNodeTag>();
        });
    if (type == Platform::MouseEvent::Type::ButtonDown) {
        m_click_start = Clock::now();
        if (it == ctx.pick_results().end()) {
            clear_selection();
            return GizmoActivationState::Inactive;
        }
        m_processing = true;

        return only_active ? GizmoActivationState::Active : GizmoActivationState::Probing;
    }

    if (Clock::now() - m_click_start >= max_click_duration)
        m_processing = false;
    if (!m_processing)
        return GizmoActivationState::Inactive;

    if (type == Platform::MouseEvent::Type::Move) {
        return GizmoActivationState::Probing;
    } else if (type == Platform::MouseEvent::Type::ButtonUp) {
        if (it != ctx.pick_results().end())
            mark_selected(*it->node);
        return GizmoActivationState::Done;
    }
    return GizmoActivationState::Inactive;
}

void QuickSelectGizmo::mark_selected(Scene::Node& n)
{
    m_selection_scene_change_session.roll_back();
    m_selection_scene_change_session.change(n).set_material_override(
        Scene::Material{}.set_uniform("uniform_color", ColorRGBA{1.0f, 1.0f, 1.0f, 1.0f})
    );
}

void QuickSelectGizmo::clear_selection()
{
    m_selection_scene_change_session.roll_back();
}

} // namespace Slic3r::App::Plater
