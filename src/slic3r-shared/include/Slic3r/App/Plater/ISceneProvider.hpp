#pragma once
#include "Slic3r/App/Scene/Scene.hpp"
#include "Slic3r/App/Scene/SceneChangeSession.hpp"


namespace Slic3r::App::Plater {

class ISceneProvider {
public:
    virtual ~ISceneProvider() = default;

    virtual Scene::Scene& scene() = 0;
    virtual const Scene::Scene& scene() const = 0;
    virtual Scene::SceneChangeSession& selection_scene_changes() = 0;
    virtual Scene::Node& selection_root() = 0;
};

}
