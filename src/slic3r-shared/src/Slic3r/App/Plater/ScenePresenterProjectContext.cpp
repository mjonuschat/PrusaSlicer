#include "Slic3r/App/Plater/ScenePresenterProjectContext.hpp"
#include "Slic3r/App/Plater/ScenePresenter.hpp"

namespace Slic3r::App::Plater {

ScenePresenterProjectContext::ScenePresenterProjectContext()
: m_scene(new Scene::Scene())
, m_selection_scene_change_session(*m_scene)
{
    m_selection_root = ScenePresenter::initialize_selection_root(*m_scene);
    m_scene->add_child(m_selection_root);
}

}