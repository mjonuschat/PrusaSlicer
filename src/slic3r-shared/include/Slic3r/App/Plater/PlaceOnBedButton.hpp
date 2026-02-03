#pragma once

#include "Slic3r/App/Plater/PlaterScenePresenter.hpp"
#include "Slic3r/App/Yoga/LayoutButton.hpp"
#include "Slic3r/Biz/Scene/Selection.hpp"
#include "Slic3r/Domain/SelectionId.hpp"
#include "Slic3r/Biz/Scene/SceneInteractor.hpp"

namespace Slic3r::App::Plater {
class PlaceOnBedButton :
    public Yoga::LayoutButton,
    public Biz::Scene::ISceneSelectionChangedListener,
    public Biz::ISelectedProjectChangedListener
{
public:
    PlaceOnBedButton(
        App::Plater::PlaterScenePresenter& scene_provider,
        Biz::ProjectInteractor& project_interactor
    );

    ~PlaceOnBedButton();

    void on_scene_selection_changed(
        Domain::SelectionId project_id,
        const Biz::Scene::ObjectSelection&
    ) override;

    void on_scene_selection_transformed(
        Domain::SelectionId,
        const Biz::Scene::ObjectSelection&
    ) override;

    void on_selected_project_changed_final(size_t) override;

    bool is_floating{false};
    void trigger();

private:
    void reload();

    App::Plater::PlaterScenePresenter& m_scene_provider;
    Biz::ProjectInteractor& m_project_interactor;
    Biz::Scene::SceneInteractor& m_scene_interactor;
};
} // namespace Slic3r::App::Plater
