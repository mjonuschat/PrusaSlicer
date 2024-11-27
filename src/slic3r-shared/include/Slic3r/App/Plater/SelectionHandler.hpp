#pragma once

#include "Slic3r/Biz/Scene/SceneInteractor.hpp"
#include "Slic3r/App/Plater/ISceneProvider.hpp"

namespace Slic3r::App::Plater {


class SelectionHandler {
public:
    explicit SelectionHandler(Biz::Scene::SceneInteractor& scene_interactor)
        : m_scene_interactor(scene_interactor)
    {}

    void mark_selected(Scene::Node& n, bool replace=true);
    void mark_unselected(Scene::Node& n);
    void clear_selection();

private:
    Biz::Scene::SceneInteractor& m_scene_interactor;
};

}
