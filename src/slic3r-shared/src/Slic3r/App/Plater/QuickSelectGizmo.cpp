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

            mark_selected(*it->node, !additive);
            return GizmoActivationState::Done;
        } else {
            if (it == ctx.pick_results().end()) {
                if (!additive)
                    clear_selection();
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
                    mark_unselected(*it->node);
                } else {
                    mark_selected(*it->node, false);
                }
            } else {
                mark_selected(*it->node);
            }
            return GizmoActivationState::Done;
        }
    }
    return GizmoActivationState::Inactive;
}

void QuickSelectGizmo::mark_selected(Scene::Node& n, bool replace)
{
    Biz::Scene::Selection selection = replace ? Biz::Scene::Selection{} : m_scene_interactor.selection();

    if (replace)
        m_selection_scene_change_session.roll_back();
    m_selection_scene_change_session.change(n).set_material_override(
        Scene::Material{}.set_uniform("uniform_color", ColorRGBA{1.0f, 1.0f, 1.0f, 1.0f})
    );
    const auto* tag = n.tag_of_type<SceneNodeTag>();
    if (tag)
    {
        selection.elements.push_back({tag->object_id, tag->instance_id, tag->volume_id});
    }

    m_scene_interactor.set_selection(selection);
}

void QuickSelectGizmo::mark_unselected(Scene::Node& n)
{
    Biz::Scene::Selection selection = m_scene_interactor.selection();
    m_selection_scene_change_session.roll_back_node(&n);
}

void QuickSelectGizmo::clear_selection()
{
    m_selection_scene_change_session.roll_back();
    m_scene_interactor.set_selection({});
}

} // namespace Slic3r::App::Plater
