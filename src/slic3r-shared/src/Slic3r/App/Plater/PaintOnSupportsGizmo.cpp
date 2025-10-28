///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/

#include "Slic3r/App/Plater/PaintOnSupportsGizmo.hpp"

#include "Slic3r/App/Plater/PaintOnSupportsDialog.hpp"
#include "Slic3r/App/Plater/PlaterScenePresenter.hpp"

#include "Slic3r/App/Scene/Clipper.hpp"
#include "Slic3r/App/Scene/ClipperPresenter.hpp"
#include "Slic3r/App/Render/Device.hpp"

#include "Slic3r/Biz/ProjectInteractor.hpp"

using namespace Slic3r::App::Yoga;

namespace Slic3r::App::Plater {

PaintOnSupportsGizmo::PaintOnSupportsGizmo(
    Render::Device& device,
    PlaterScenePresenter& scene_presenter,
    Biz::ProjectInteractor* project_interactor
) :
    m_device(device),
    m_scene_presenter(scene_presenter),
    m_project_interactor(project_interactor)
{
    m_dialog = std::make_unique<PaintOnSupportsDialog>();

    m_dialog->callbacks().clipping_view_ratio_changed = [this](double ratio)
    { m_clipper_presenter.set_position_by_ratio(ratio, true); };
}

void PaintOnSupportsGizmo::on_activated()
{
    const Biz::Scene::ObjectSelection& selection =
        m_project_interactor->scene_interactor().object_selection();

    if (selection.empty() || selection.mode != Slic3r::Biz::Scene::SelectionMode::Instance) {
        on_deactivated();
        // We can't perform a paint on supports for multiple objects simultaneously.
        return;
    }

    Domain::Project& project = m_project_interactor->selected_project();

    const Domain::ElementRef& element = selection.elements.front();
    assert(element.volume_id == 0); // is object
    Domain::ModelObject* selected_object = project.find_object_by_id(element.object_id);
    Domain::ModelInstance* selected_instance =
        project.find_instance_by_id(element.object_id, element.instance_id);
    ASSERT(selected_instance && selected_object);

    m_clipper_presenter.activate(&m_scene_presenter.scene(), selected_object, selected_instance);

    m_clipper_presenter.set_behavior(true, true, 0.5);
    m_clipper_presenter.set_position_by_ratio(0.5, false);
}

void PaintOnSupportsGizmo::on_deactivated()
{
    m_clipper_presenter.deactivate();
}

void PaintOnSupportsGizmo::on_project_activated(size_t new_project_id)
{
    on_activated();
}

void PaintOnSupportsGizmo::on_project_deactivated(size_t old_project_id)
{
    on_deactivated();
}

Scene::ToolType PaintOnSupportsGizmo::type() const
{
    return Scene::ToolType::PaintOnSupportsGizmo;
}

Yoga::GizmoDialog* PaintOnSupportsGizmo::ui_dialog()
{
    return m_dialog.get();
}

void PaintOnSupportsGizmo::provide_clipper(Scene::Clipper& clipper)
{
    // fill clipper here
    m_clipper_presenter = Scene::ClipperPresenter(&clipper, &m_device);
}

Scene::GizmoActivationState
PaintOnSupportsGizmo::on_mouse(Scene::GizmoEventContext& ctx, bool only_active)
{
    return Scene::GizmoActivationState::Inactive;
}

} // namespace Slic3r::App::Plater
