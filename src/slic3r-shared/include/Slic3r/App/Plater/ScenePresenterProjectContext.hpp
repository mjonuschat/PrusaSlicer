#pragma once

#include "Slic3r/App/Scene/Scene.hpp"
#include "Slic3r/App/Scene/SceneChangeSession.hpp"

namespace Slic3r::App::Plater {

struct GeometryElementId
{
    enum class Type : uint8_t
    {
        Volume = 0,
        Bed,
        WipeTower
    };

    Type type;
    size_t id;

    /**
     *
     * @param rhs
     * @return
     */
    bool operator==(const GeometryElementId& rhs)
    { return type == rhs.type && id == rhs.id; }

    bool operator<(const GeometryElementId& rhs)
    { return type < rhs.type || (type == rhs.type && id < rhs.id); }
};

class ScenePresenterProjectContext {
public:
    ScenePresenterProjectContext(): m_selection_scene_change_session(m_scene) {}

    Scene::Scene& scene() { return m_scene; }
    const Scene::Scene& scene() const { return m_scene; }
    Scene::SceneChangeSession& selection_scene_changes()
    {
        return m_selection_scene_change_session;
    }

private:
    Scene::Scene m_scene;
    Scene::SceneChangeSession m_selection_scene_change_session;
};
}
