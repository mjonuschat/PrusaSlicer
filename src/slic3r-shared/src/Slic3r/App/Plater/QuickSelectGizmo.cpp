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
        const bool additive = (ctx.mouse_event().key_modifiers() &
                               Platform::KeyModifiers(Platform::KeyModifier::Shift)) != 0;
        const auto& selection = m_scene_interactor.selection();
        const bool selection_empty = selection.empty();

        if (selection_empty) {
            if (it == ctx.pick_results().end())
                return GizmoActivationState::Inactive;

            m_selection_handler.mark_selected(*it->node, !additive);
            return GizmoActivationState::Done;
        } else {
            if (it == ctx.pick_results().end()) {
                if (!additive)
                    m_selection_handler.clear_selection();
                return GizmoActivationState::Done;
            }

            const auto& tag = *it->node->tag_of_type<SceneNodeTag>();

            const bool already_selected =
                std::any_of(selection.elements.begin(), selection.elements.end(), [&](const auto& e) {
                    return e.object_id == tag.object_id && e.volume_id == tag.volume_id &&
                        e.instance_id == tag.instance_id;
                });
            if (additive) {
                if (already_selected) {
                    m_selection_handler.mark_unselected(*it->node);
                } else {
                    m_selection_handler.mark_selected(*it->node, false);
                }
            } else {
                m_selection_handler.mark_selected(*it->node);
            }
            return GizmoActivationState::Done;
        }
    }
    return GizmoActivationState::Inactive;
}

} // namespace Slic3r::App::Plater
