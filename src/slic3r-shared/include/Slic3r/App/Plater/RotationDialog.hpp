#pragma once

#include "Slic3r/App/Plater/PlaceOnBedButton.hpp"
#include "Slic3r/App/Plater/PlaterScenePresenter.hpp"
#include "Slic3r/App/Plater/ReferenceFramePicker.hpp"
#include "Slic3r/App/Yoga/GizmoWindow.hpp"

namespace Slic3r::Biz {
    class ProjectInteractor;
}

namespace Slic3r::App::Plater {
class TripleInput;

class RotationDialog final :
    public Yoga::GizmoWindow,
    public App::Plater::ISelectionBoundingBoxChangedListener
{
public:
    RotationDialog(
        App::Plater::PlaterScenePresenter& scene_provider,
        Biz::ProjectInteractor& project_interactor
    );

    ~RotationDialog();

    void on_scene_selection_bounding_box_changed(
        Domain::SelectionId project_id,
        const std::optional<Scene::OrientedBoundingBox>&
    ) override;

    void on_activated(Domain::SelectionId project_id);
    void on_deactivated();
    PlaceOnBedButton& place_on_bed_button();

private:
    std::optional<Domain::Vec3d> get_obb_rotation() const;
    void reload(std::optional<Domain::SelectionId> project_id = std::nullopt);
    App::Plater::PlaterScenePresenter& m_scene_provider;
    Biz::ProjectInteractor& m_project_interactor;
    TripleInput* m_relative_input;
    bool m_activated{false};
    Yoga::LayoutButton* m_revert_button{nullptr};
    PlaceOnBedButton* m_place_on_bed_button{nullptr};
    ReferenceFramePicker* m_reference_frame_picker;
    Biz::Scene::SceneInteractor::ElementTransforms m_reset_rotation_candidates;

    void add_rotation(Domain::Vec3d rotate_by_rads);
    Biz::Scene::SceneInteractor::ElementTransforms get_reset_rotation_candidates() const;
};
}
