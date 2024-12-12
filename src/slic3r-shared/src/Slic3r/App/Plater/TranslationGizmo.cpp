#include "Slic3r/App/Plater/TranslationGizmo.hpp"
#include "Slic3r/App/Plater/ScenePresenter.hpp"
#include "Slic3r/App/Plater/GizmoDataFactory.hpp"
#include "Slic3r/App/Plater/GizmoNodeTag.hpp"

namespace Slic3r::App::Plater {

void TranslationGizmo::on_cycle_prepare()
{
    m_activated = false;
}


GizmoActivationState TranslationGizmo::on_mouse(GizmoEventContext& ctx, bool only_active)
{
    const auto event_type = ctx.mouse_event().type();
    if (event_type != Platform::MouseEvent::Type::ButtonDown &&
        event_type != Platform::MouseEvent::Type::Move &&
        event_type != Platform::MouseEvent::Type::ButtonUp) {
        m_activated = false;
        return GizmoActivationState::Inactive;
    }

    const auto& pick_ray = ctx.pick_ray();

    if (event_type == Platform::MouseEvent::Type::ButtonDown) {
        const Scene::Node* node = ctx.pick_result_node_with_tag_of_type<GizmoNodeTag>();
        if (node == nullptr) {
            m_activated = false;
            return GizmoActivationState::Inactive;
        }

        const GizmoNodeTag& tag = *node->tag_of_type<GizmoNodeTag>();
        m_translation_ray.origin = m_scene_provider.selection_root().world_transform().col(3).head(
            3
        );
        m_translation_ray.direction = tag.primary_axis_dir();
    }

    double t;
    if (!m_translation_ray.closest_point_from_ray(pick_ray, t)) {
        m_activated = false;
        return GizmoActivationState::Inactive;
    }

    if (event_type == Platform::MouseEvent::Type::ButtonDown) {
        m_start_t = t;
        m_activated = true;
        return GizmoActivationState::Active;
    }

    if (!m_activated) {
        return GizmoActivationState::Inactive;
    }

    Vec3d delta = m_translation_ray.point_at(t) - m_translation_ray.point_at(m_start_t);

    Matrix4d translation_matrix = Matrix4d::Identity();
    translation_matrix.col(3).head(3) = delta;

    // SPDLOG_INFO("Translation: ({}, {}, {}) (t0: {}  t1: {})", delta.x(), delta.y(), delta.z(),
    // m_start_t, t);

    m_scene_interactor.transform_selection(translation_matrix, m_xform_memento);

    if (event_type == Platform::MouseEvent::Type::ButtonUp) {
        m_scene_interactor.finalize_transform_selection(m_xform_memento, false);
        m_activated = false;
        clear_highlight();
        return GizmoActivationState::Done;
    }

    return GizmoActivationState::Active;
}

void TranslationGizmo::clear_highlight()
{
    if (m_highlighted)
        visit(
            m_scene_provider.selection_root(), [](Scene::Node& node) { node.set_enabled(true); },
            true
        );
    m_highlighted = false;
}
void TranslationGizmo::on_transient_mouse(GizmoEventContext& ctx)
{
    if (m_activated)
        return;
    auto* n = ctx.pick_result_node_with_tag_of_type<GizmoNodeTag>();
    if (n == nullptr) {
        clear_highlight();
    } else {
        auto* p = n->parent();
        for (auto& child : p->children())
            child->set_enabled(child.get() == n);
        m_highlighted = true;
    }
}


void TranslationGizmo::on_activated()
{
    auto& scene = m_scene_provider.scene();
    auto& selection_root = m_scene_provider.selection_root();

    auto x_handle = m_data_factory.create_node(
        scene, GizmoDataId::ConeHandle, GizmoDataVariant::Red, GizmoDataTransform::PointX
    );
    auto y_handle = m_data_factory.create_node(
        scene, GizmoDataId::ConeHandle, GizmoDataVariant::Green, GizmoDataTransform::PointY
    );
    auto z_handle = m_data_factory.create_node(
        scene, GizmoDataId::ConeHandle, GizmoDataVariant::Blue, GizmoDataTransform::PointZ
    );

    x_handle->set_tag(GizmoNodeTag{AxisType::XAxis});
    y_handle->set_tag(GizmoNodeTag{AxisType::YAxis});
    z_handle->set_tag(GizmoNodeTag{AxisType::ZAxis});

    auto center = m_data_factory.create_node(scene, GizmoDataId::AxesLines);
    scene.add_child(x_handle.release(), &selection_root);
    scene.add_child(y_handle.release(), &selection_root);
    scene.add_child(z_handle.release(), &selection_root);
    scene.add_child(center.release(), &selection_root);
}

void TranslationGizmo::on_deactivated()
{
    auto& scene = m_scene_provider.scene();
    auto& selection_root = m_scene_provider.selection_root();
    scene.remove_children([](const Scene::Node*) { return true; }, &selection_root);
}

}
