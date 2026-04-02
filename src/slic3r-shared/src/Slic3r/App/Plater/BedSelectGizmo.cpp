#include "Slic3r/App/Plater/BedSelectGizmo.hpp"
#include "Slic3r/App/Scene/BedNodeTag.hpp"
#include "Slic3r/Biz/ProjectInteractor.hpp"
#include "Slic3r/Biz/Algorithms/Point.hpp"
#include "Slic3r/App/Scene/ISceneProvider.hpp"

namespace Slic3r::App::Plater {

BedSelectGizmo::BedSelectGizmo(
    Biz::ProjectInteractor& project_interactor,
    Scene::ISceneProvider& scene_provider
) :
    m_project_interactor(project_interactor),
    m_scene_interactor(project_interactor.scene_interactor()),
    m_scene_provider(scene_provider)
{ }

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

    if (evt.button() != Platform::MouseButton::Left && evt.button() != Platform::MouseButton::Right)
    {
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
            // Center the camera pivot on the bed
            const Domain::ConfigContainer* cc = m_project_interactor.selected_project().find_config_container(instance.config_container_id);
            DEBUG_ASSERT(cc != nullptr);
            const Domain::BedInstance& inst = cc->find_bed_instance(instance.instance_id);
            m_scene_provider.scene().camera_trackball()
                .set_pivot(Biz::Algorithms::Point::to_3d(cc->bed().center(), 0.0) + inst.transformation.get_offset());
            return Scene::GizmoActivationState::Done;
        }
    }

    return Scene::GizmoActivationState::Inactive;
}

} // namespace Slic3r::App::Plater
