#pragma once

#include "Slic3r/App/Scene/Scene.hpp"

namespace Slic3r::App::Plater {
class ScenePresenterProjectContext {
public:
    Scene::Scene& scene() { return m_scene; }
private:
    Scene::Scene m_scene;
};
}
