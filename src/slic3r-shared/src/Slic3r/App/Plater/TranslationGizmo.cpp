#include "Slic3r/App/Plater/TranslationGizmo.hpp"
#include "Slic3r/App/Plater/ScenePresenter.hpp"
#include "Slic3r/App/Plater/GizmoDataFactory.hpp"
#include "Slic3r/App/Plater/GizmoNodeTag.hpp"

namespace Slic3r::App::Plater {

GizmoActivationState TranslationGizmo::on_mouse(const GizmoEventContext& ctx, bool only_active)
{
    const auto event_type = ctx.mouse_event().type();
    if (event_type != Platform::MouseEvent::Type::ButtonDown &&
        event_type != Platform::MouseEvent::Type::Move &&
        event_type != Platform::MouseEvent::Type::ButtonUp)
        return GizmoActivationState::Inactive;

    const Scene::Node* node = ctx.pick_result_node_with_tag_of_type<GizmoNodeTag>();
    if (node == nullptr) {
        return GizmoActivationState::Inactive;
    }

    const GizmoNodeTag& tag = *node->tag_of_type<GizmoNodeTag>();

    if (event_type == Platform::MouseEvent::Type::ButtonDown) {

        m_xform_memento.reset();
        const auto& pick_ray = ctx.pick_ray();
        m_translation_ray.origin = m_scene_provider.selection_root().world_transform().col(3).head(3);
        //m_translation_ray.direction = 0;

        if (!pick_ray.closest_point_from_ray(m_translation_ray, m_start_t))
            return GizmoActivationState::Inactive;
    }

    //m_scene_interactor.transform_selection(x, m_xform_memento);
    return GizmoActivationState::Active;
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
    x_handle->set_tag(GizmoNodeTag{AxisType::YAxis});
    x_handle->set_tag(GizmoNodeTag{AxisType::ZAxis});

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
