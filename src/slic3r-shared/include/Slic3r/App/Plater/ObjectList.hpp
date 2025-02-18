#pragma once
#include "imgui/imgui.h"

namespace Slic3r {
class Model;
}

namespace Slic3r::Biz::Scene {
class SceneInteractor;
}

namespace Slic3r::App::Plater {

class ObjectList
{
public:
    ObjectList() {}
    
    void init(Biz::Scene::SceneInteractor& scene_interactor, const Slic3r::Model& model) {
        m_scene_interactor = &scene_interactor;
        m_model = &model;
    }

    void render(ImVec2 pos, ImVec2 size);

protected:

private:
//    bool handle_selection(const Domain::ElementRef& id)
    void propagate_selection();

private:
    Biz::Scene::SceneInteractor*    m_scene_interactor  { nullptr };
    const Slic3r::Model*            m_model             { nullptr };
};

} // namespace Slic3r::App::Plater