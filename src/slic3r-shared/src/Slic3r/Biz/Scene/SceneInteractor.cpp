#include "Slic3r/Biz/Scene/SceneInteractor.hpp"
#include "libslic3r/Model.hpp"

namespace Slic3r::Biz::Scene {

void SceneInteractor::on_selected_project_changed(size_t index)
{
    auto& project = m_workbench.project(index);
    if (m_projects.count(index) == 0)
        m_projects.emplace(index, SceneInteractorProjectContext{project});
    m_selected_project_id = index;
}


const Selection& SceneInteractor::selection() const
{
    ASSERT(m_selected_project_id != Domain::INVALID_ID);
    const auto it = m_projects.find(m_selected_project_id);
    ASSERT(it != m_projects.end());
    return it->second.selection;
}

void SceneInteractor::set_selection(const Selection& selection)
{
    const auto it = m_projects.find(m_selected_project_id);
    ASSERT(it != m_projects.end());
    it->second.selection = selection;
    m_selection_changed_listeners.invoke([&](auto* l){
        l->on_scene_selection_changed(m_selected_project_id, selection);
    });
}

void SceneInteractor::modify_selection(const std::function<void(Selection&)>& modifier)
{
    const auto it = m_projects.find(m_selected_project_id);
    ASSERT(it != m_projects.end());
    auto& selection = it->second.selection;
    modifier(selection);
    m_selection_changed_listeners.invoke([&](auto* l){
        l->on_scene_selection_changed(m_selected_project_id, selection);
    });

}

void SceneInteractor::new_object_from_mesh(TriangleMesh&& mesh)
{
    auto& project = m_workbench.project(m_selected_project_id);
    auto& obj = *project.model().add_object();
    auto& vol = *obj.add_volume(std::move(mesh));
    auto& inst = *obj.add_instance();
    const Domain::ElementRefs updated {{obj.id().id, inst.id().id, vol.id().id}};

    m_changed_listeners.invoke([&](auto* l) {
        l->on_instance_added(m_selected_project_id, updated);
    });
}

} // namespace Slic3r::Biz::Scene
