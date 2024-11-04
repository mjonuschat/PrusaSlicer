#include "Slic3r/Biz/Scene/SceneInteractor.hpp"

namespace Slic3r::Biz::Scene {

void SceneInteractor::on_selected_project_changed(size_t index)
{
    auto& project = m_workbench.project(index);
    if (m_projects.count(index) == 0)
        m_projects.emplace(index, SceneInteractorProjectContext{project});
    m_selected_project_id = index;
}
}
