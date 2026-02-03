#pragma once

#include "Slic3r/App/Plater/PlaceOnBedButton.hpp"
#include "Slic3r/App/Plater/PlaterScenePresenter.hpp"
#include "Slic3r/App/Plater/ReferenceFramePicker.hpp"
#include "Slic3r/App/Yoga/GizmoWindow.hpp"
#include "Slic3r/App/Yoga/ToggleButton.hpp"
#include "Slic3r/Biz/Scene/SceneInteractor.hpp"

namespace Slic3r::Biz {
    class ProjectInteractor;
}

namespace Slic3r::App::Plater {
class TripleInput;

class ScaleDialog final :
    public Yoga::GizmoWindow,
    public Biz::Scene::ISceneSelectionChangedListener,
    public App::Plater::ISelectionBoundingBoxChangedListener,
    public Biz::ISelectedProjectChangedListener
{
public:
    ScaleDialog(
        App::Plater::PlaterScenePresenter& scene_provider,
        Biz::ProjectInteractor& project_interactor
    );

    ~ScaleDialog();


    void on_scene_selection_changed(
        Domain::SelectionId project_id,
        const Biz::Scene::ObjectSelection&
    ) override;

    void on_scene_selection_transformed(
        Domain::SelectionId project_id,
        const Biz::Scene::ObjectSelection&
    ) override;

    void on_scene_selection_bounding_box_changed(
        Domain::SelectionId project_id,
        const std::optional<Scene::OrientedBoundingBox>&
    ) override;

    void on_selected_project_changed_final(size_t index) override;

    void on_activated(Domain::SelectionId project_id);
    void on_deactivated();

    PlaceOnBedButton& place_on_bed_button();

private:
    std::optional<Domain::Vec3d> get_current_absolute_scale() const;
    std::optional<Domain::Vec3d> get_current_relative_scale() const;
    App::Plater::PlaterScenePresenter& m_scene_provider;
    Biz::ProjectInteractor& m_project_interactor;
    TripleInput* m_absolute_input{nullptr};
    TripleInput* m_absolute_percent_input{nullptr};
    Yoga::Item* m_absolute_percent_input_item{nullptr};
    Yoga::LayoutButton* m_revert_button{nullptr};
    TripleInput* m_relative_input{nullptr};
    Yoga::Item* m_relative_input_item{nullptr};
    Yoga::ToggleButton* m_lock{nullptr};
    PlaceOnBedButton* m_place_on_bed_button{nullptr};
    ReferenceFramePicker* m_reference_frame_picker;

    struct ProjectContext {
        bool activated{false};
        Biz::Scene::SceneInteractor::ElementTransforms reset_scale_candidates;
    };

    using ProjectContexts = Biz::ProjectScoped<ProjectContext>;
    ProjectContexts m_projects;

    void reload(std::optional<Domain::SelectionId> project_id = std::nullopt);
    void apply_relative_scale(const Domain::Vec3d& scale);
    Biz::Scene::SceneInteractor::ElementTransforms get_reset_scale_candidates() const;
};
}
