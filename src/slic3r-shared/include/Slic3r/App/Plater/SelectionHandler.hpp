#pragma once

#include "Slic3r/Biz/Scene/SceneInteractor.hpp"
#include "Slic3r/App/Plater/ISceneProvider.hpp"

namespace Slic3r::App::Plater {


class SelectionHandler {
public:
    SelectionHandler(Biz::Scene::SceneInteractor& scene_interactor, ISceneProvider& scene_provider)
        : m_scene_interactor(scene_interactor), m_scene_provider(scene_provider)
    {}

    void mark_selected(Scene::Node& n, bool replace=true);
    void mark_unselected(Scene::Node& n);
    void clear_selection();

private:
    Biz::Scene::SceneInteractor& m_scene_interactor;
    ISceneProvider& m_scene_provider;
};

}
