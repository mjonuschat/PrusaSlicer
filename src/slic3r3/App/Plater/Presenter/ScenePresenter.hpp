#pragma once

#include "slic3r3/App/Scene/Scene.hpp"

namespace Slic3r::App::Plater::Presenter {

class ScenePresenter {
    explicit ScenePresenter(Scene::Scene* scene): m_scene(scene) {}

private:
    Scene::Scene* m_scene;
};

}
