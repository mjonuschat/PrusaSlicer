#include "Slic3r/App/Plater/BedSelectGizmo.hpp"
#include "Slic3r/App/Plater/BedNodeTag.hpp"

namespace Slic3r::App::Plater {

GizmoActivationState BedSelectGizmo::on_mouse(GizmoEventContext& ctx, bool only_active)
{
    // ignore bed picking when the camera is below the bed
    const Scene::Camera& camera = m_scene_provider.scene().camera();
    Scene::CameraProjectionType camera_type = camera.cam_projection().type();
    bool pick_disabled =
        (camera_type == Scene::CameraProjectionType::Perspective) ? camera.position().z() < 0.0 : camera.forward().z() >= 0.0;

    if (pick_disabled)
        return GizmoActivationState::Inactive;

    const auto& evt = ctx.mouse_event();
    auto type = evt.type();

    if (type == Platform::MouseEvent::Type::ButtonDown) {
        if (evt.button() != Platform::MouseButton::Left)
            return GizmoActivationState::Inactive;

        auto it =
            std::find_if(ctx.pick_results().begin(), ctx.pick_results().end(), [&](const auto& item) {
                return item.node->has_tag_of_type<BedNodeTag>();
            });

        if (it == ctx.pick_results().end())
            return GizmoActivationState::Inactive;

        BedNodeTag* tag = it->node->tag_of_type<BedNodeTag>();
        Domain::BedRef instance = { tag->config_container_id, tag->instance_id };
        m_scene_interactor.select_bed_instance(instance);
        return GizmoActivationState::Done;
    }

    return GizmoActivationState::Inactive;
}

} // namespace Slic3r::App::Plater
