//
// Created by Jan Bartipan on 07.03.2024.
//

#pragma once

#include "slic3r3/App/Scene/Scene.hpp"

namespace Slic3r::App::Presenter {

class ScenePresenter {
    explicit ScenePresenter(Scene::Scene* scene): m_scene(scene) {}

private:
    Scene::Scene* m_scene;
};

}
