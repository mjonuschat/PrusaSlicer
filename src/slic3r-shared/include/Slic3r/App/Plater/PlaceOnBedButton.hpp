#pragma once

#include "Slic3r/App/Plater/PlaterScenePresenter.hpp"
#include "Slic3r/App/Yoga/LayoutButton.hpp"
#include "Slic3r/Domain/SelectionId.hpp"
#include "Slic3r/Biz/Scene/SceneInteractor.hpp"

namespace Slic3r::App::Plater {
class PlaceOnBedButton :
    public Yoga::LayoutButton,
    public Biz::ISelectedProjectChangedListener,
    public ISelectionExtentsChangedListener
{
public:
    PlaceOnBedButton(
        App::Plater::PlaterScenePresenter& scene_provider,
        Biz::ProjectInteractor& project_interactor
    );

    ~PlaceOnBedButton();

    void on_selected_project_changed_final(size_t) override;

    void on_scene_selection_bounding_box_changed(
        Domain::SelectionId,
        const std::optional<Biz::Scene::SelectionExtents>&
    ) override;

private:
    void reload();

    App::Plater::PlaterScenePresenter& m_scene_provider;
    Biz::ProjectInteractor& m_project_interactor;
    Biz::Scene::SceneInteractor& m_scene_interactor;
};
} // namespace Slic3r::App::Plater
