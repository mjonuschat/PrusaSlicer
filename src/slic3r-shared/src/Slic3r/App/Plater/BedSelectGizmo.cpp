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
        if (evt.button() != Platform::MouseButton::Left || ctx.pick_results().empty())
            return GizmoActivationState::Inactive;

        BedNodeTag* tag = ctx.pick_results().front().node->tag_of_type<BedNodeTag>();
        if (tag) {
            Domain::BedRef instance = { tag->config_container_id, tag->instance_id };
            if (m_scene_interactor.selected_bed_instance() != instance) {
                m_scene_interactor.select_bed_instance(instance);
                return GizmoActivationState::Done;
            }
        }
    }

    return GizmoActivationState::Inactive;
}

} // namespace Slic3r::App::Plater
