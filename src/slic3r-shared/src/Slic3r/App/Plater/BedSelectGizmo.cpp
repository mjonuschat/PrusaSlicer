#include "Slic3r/App/Plater/BedSelectGizmo.hpp"
#include "Slic3r/App/Scene/BedNodeTag.hpp"

namespace Slic3r::App::Plater {

BedSelectGizmo::BedSelectGizmo(
    Biz::Scene::SceneInteractor& scene_interactor,
    Scene::ISceneProvider& scene_provider
) :
    m_scene_interactor(scene_interactor),
    m_scene_provider(scene_provider)
{
    m_scene_interactor.add_listener<Biz::Scene::ISceneBedInstanceChangedListener>(this);
}

BedSelectGizmo::~BedSelectGizmo()
{
    m_scene_interactor.remove_listener<Biz::Scene::ISceneBedInstanceChangedListener>(this);
}

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

    if (evt.button() != Platform::MouseButton::Left) {
        return Scene::GizmoActivationState::Inactive;
    }

    const Scene::BedNodeTag* tag{
        ctx.pick_results().empty() ? nullptr :
                                     ctx.pick_results().front().node->tag_of_type<Scene::BedNodeTag>()
    };

    if (tag == nullptr) {
        Biz::Scene::BedSelection& selection{m_scene_interactor.bed_selection()};
        if (selection.select_one(selection.last_selected_bed())) {
            return Scene::GizmoActivationState::Done;
        }
        return Scene::GizmoActivationState::Inactive;
    }

    const Domain::BedRef instance{tag->config_container_id, tag->instance_id};

    const bool shift_down{
        (evt.key_modifiers() & Platform::KeyModifiers(Platform::KeyModifier::Shift)) != 0
    };

    if (shift_down) {
        if (m_scene_interactor.bed_selection().toggle(instance)) {
            return Scene::GizmoActivationState::Done;
        }
    } else {
        if (m_scene_interactor.bed_selection().select_one(instance)) {
            return Scene::GizmoActivationState::Done;
        }
    }

    return Scene::GizmoActivationState::Inactive;
}

} // namespace Slic3r::App::Plater
