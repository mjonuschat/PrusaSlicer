#pragma once

#include "Slic3r/App/Yoga/LayoutButton.hpp"
#include "Slic3r/Biz/ProjectInteractor.hpp"
#include "Slic3r/Domain/SelectionId.hpp"
#include "Slic3r/Biz/Scene/SceneInteractor.hpp"

namespace Slic3r::App::Plater {
class PlaceOnBedButton :
    public Yoga::LayoutButton,
    public Biz::ISelectedProjectChangedListener,
    public Biz::Scene::ISceneSelectionChangedListener
{
public:
    PlaceOnBedButton(Biz::ProjectInteractor& project_interactor);

    ~PlaceOnBedButton();

    void on_selected_project_changed_final(size_t) override;

    void on_scene_selection_changed(
        Domain::SelectionId project_id,
        const Biz::Scene::ObjectSelection&
    ) override
    {}

    void on_scene_selection_bounding_box_updated(
        Domain::SelectionId,
        const Biz::Scene::ObjectSelection&
    ) override;

private:
    void reload();

    Biz::ProjectInteractor& m_project_interactor;
    Biz::Scene::SceneInteractor& m_scene_interactor;
};
} // namespace Slic3r::App::Plater
