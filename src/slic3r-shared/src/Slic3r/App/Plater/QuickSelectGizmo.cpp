#include "Slic3r/App/Plater/QuickSelectGizmo.hpp"

#include "Slic3r/App/Render/Device.hpp"
#include "Slic3r/App/Scene/Node.hpp"
#include "Slic3r/App/Render/GeometryBuilder.hpp"

namespace Slic3r::App::Plater {

GizmoActivationState QuickSelectGizmo::on_mouse(const GizmoEventContext& ctx, bool only_active)
{
    using namespace std::chrono_literals;
    const auto type = ctx.mouse_event().type();

    constexpr static Clock::duration max_click_duration = Clock::duration {500ms};

    auto it =
        std::find_if(ctx.pick_results().begin(), ctx.pick_results().end(), [&](const auto& item) {
            const Scene::Node& n = *item.node;
            return n.has_tag_of_type<SceneNodeTag>();
        });

    if (type == Platform::MouseEvent::Type::ButtonDown) {
        if (ctx.mouse_event().button() != Platform::MouseButton::Left) {
            m_processing = false;
            return GizmoActivationState::Inactive;
        }

        m_click_start = Clock::now();
        m_processing = true;

        return only_active ? GizmoActivationState::Active : GizmoActivationState::Probing;
    }

    if (m_processing && Clock::now() - m_click_start >= max_click_duration) {
        m_processing = false;
        SPDLOG_INFO("QuickSelectGizmo activation timed out");
    }
    if (!m_processing)
        return GizmoActivationState::Inactive;

    if (type == Platform::MouseEvent::Type::Move) {
        return  GizmoActivationState::Probing;
    } else if (type == Platform::MouseEvent::Type::ButtonUp) {
        const bool additive = (ctx.mouse_event().key_modifiers() &
                               Platform::KeyModifiers(Platform::KeyModifier::Shift)) != 0;
        const auto& selection = m_scene_interactor.selection();

        if (selection.empty()) {
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

void QuickSelectGizmo::update_selection_rect(float x, float y, float w, float h)
{
    Render::GeometryBuilder<Render::VertexP3> builder;


    auto* shader = m_device.context().shader_manager().get_shader("dashed_thick_lines");

    builder
        .add_vertex({{x, y, 0}})
        .add_vertex({{x + w, y, 0}})
        .add_vertex({{x + w, y + h, 0}})
        .add_vertex({{x, y + h, 0}})
    .add_draw_command({Render::PrimitiveType::LineLoop, 0, 4, Render::Material{}.set_shader(shader)});
    builder.update(m_selection_rect);
}


void QuickSelectGizmo::render_scene(Render::CommandBuffer& cmd_buffer)
{
    if (!m_selection_rect_shown)
        return;

    Render::Material mat;
    Matrix4f vm = Matrix4f::Identity();
    // TODO: setup ortho projection (use m_screen_info to get physical size---i.e. in real pixels)
    mat
        .set_uniform("view_model_matrix", vm)
        .set_uniform("projection_matrix", Matrix4f{});
    cmd_buffer.bind_and_draw(m_selection_rect, mat);
}

} // namespace Slic3r::App::Plater
