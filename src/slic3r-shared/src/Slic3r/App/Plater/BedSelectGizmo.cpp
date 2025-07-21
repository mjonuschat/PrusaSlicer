#include "Slic3r/App/Plater/BedSelectGizmo.hpp"
#include "Slic3r/App/Scene/BedNodeTag.hpp"

namespace Slic3r::App::Plater {

Scene::GizmoActivationState BedSelectGizmo::on_mouse(Scene::GizmoEventContext& ctx, bool only_active)
{
    // ignore bed picking when the camera is below the bed
    const Scene::Camera& camera{m_scene_provider.scene().camera()};
    const Scene::CameraProjectionType camera_type{camera.cam_projection().type()};
    const bool pick_disabled{
        (camera_type == Scene::CameraProjectionType::Perspective) ? camera.position().z() < 0.0 :
                                                                    camera.forward().z() >= 0.0
    };

    if (pick_disabled)
        return Scene::GizmoActivationState::Inactive;

    const Platform::MouseEvent& evt{ctx.mouse_event()};
    const Platform::MouseEvent::Type type{evt.type()};

    if (type != Platform::MouseEvent::Type::ButtonDown) {
        return Scene::GizmoActivationState::Inactive;
    }

    if (evt.button() != Platform::MouseButton::Left || ctx.pick_results().empty()) {
        return Scene::GizmoActivationState::Inactive;
    }

    const Scene::BedNodeTag* tag{ctx.pick_results().front().node->tag_of_type<Scene::BedNodeTag>()};

    if (!tag) {
        return Scene::GizmoActivationState::Inactive;
    }

    const Domain::BedRef instance{tag->config_container_id, tag->instance_id};

    const bool ctrl_down{
        (evt.key_modifiers() & Platform::KeyModifiers(Platform::KeyModifier::Ctrl)) != 0
    };

    if (ctrl_down) {
        if (m_scene_interactor.toggle_bed_instance(instance)) {
            return Scene::GizmoActivationState::Done;
        }
    } else {
        if (m_scene_interactor.select_one_bed_instance(instance)) {
            return Scene::GizmoActivationState::Done;
        }
    }

    return Scene::GizmoActivationState::Inactive;
}

} // namespace Slic3r::App::Plater
