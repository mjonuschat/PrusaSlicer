#pragma once

#include "Slic3r/Domain/SelectionId.hpp"
#include "Slic3r/Biz/ListenerList.hpp"
#include "Slic3r/Biz/Preset/PresetInteractor.hpp"
#include "Slic3r/Biz/Scene/SceneInteractor.hpp"

namespace Slic3r::Domain {
class Project;
class Workbench;
} // namespace Slic3r::Domain

namespace Slic3r::Biz {

class ISelectedProjectChangedListener;
class ISelectedConfigContainerChangedListener;
class IProjectsChangedListener;

class ProjectInteractor final
{
public:
    explicit ProjectInteractor(Domain::Workbench& workbench)
        : m_workbench(workbench), m_preset_interactor(workbench), m_scene_interactor(workbench)
    {
        add_selected_config_container_changed_listener(&m_preset_interactor);
        add_selected_project_changed_listener(&m_scene_interactor);
    }

    /**
     * Select already opened project. If the project is already selected, do nothing.
     * @param project_id An index of project to be selected.
     */
    void select_project(Domain::SelectionId project_id);

    /**
     * Add project to Workbench and select it.
     * @param p Project to move to workbench
     * @return An project_id / index of added project.
     */
    Domain::SelectionId add_project(Domain::Project&& p);

    /**
     * Remove given project from workbench and update selection if needed.
     * @param project_id
     */
    void remove_project(Domain::SelectionId project_id);

    void add_selected_project_changed_listener(ISelectedProjectChangedListener* l)
    {
        m_selected_project_changed_listeners.add(l);
    }

    void remove_selected_project_changed_listener(ISelectedProjectChangedListener* l)
    {
        m_selected_project_changed_listeners.remove(l);
    }

    void add_projects_changed_listener(IProjectsChangedListener* l)
    {
        m_projects_changed_listeners.add(l);
    }

    void remove_projects_changed_listener(IProjectsChangedListener* l)
    {
        m_projects_changed_listeners.remove(l);
    }

    void add_selected_config_container_changed_listener(ISelectedConfigContainerChangedListener* l)
    {
        m_selected_config_container_changed_listener.add(l);
    }

    void remove_selected_config_container_changed_listener(ISelectedConfigContainerChangedListener* l)
    {
        m_selected_config_container_changed_listener.remove(l);
    }

    Domain::SelectionId new_project();

    const Preset::PresetInteractor& preset_interactor() const { return m_preset_interactor; }
    Preset::PresetInteractor& preset_interactor() { return m_preset_interactor; }

    const Scene::SceneInteractor& scene_interactor() const { return m_scene_interactor; }
    Scene::SceneInteractor& scene_interactor() { return m_scene_interactor; }

private:
    void do_select_project(Domain::SelectionId project_id);
    void do_select_config_container(Domain::SelectionId container_id);

    void initialize_new_project(Domain::Project& p);

private:
    struct Selection
    {
        Domain::SelectionId project_id{Domain::INVALID_ID};
        Domain::SelectionId config_container_id{Domain::INVALID_ID};
    };

    ListenerList<ISelectedProjectChangedListener> m_selected_project_changed_listeners;
    ListenerList<ISelectedConfigContainerChangedListener> m_selected_config_container_changed_listener;
    ListenerList<IProjectsChangedListener> m_projects_changed_listeners;

    Domain::Workbench& m_workbench;
    Selection m_selection;

    Preset::PresetInteractor m_preset_interactor;
    Scene::SceneInteractor m_scene_interactor;
};

} // namespace Slic3r::Biz
