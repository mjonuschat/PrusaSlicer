#include "PresetInteractor.hpp"
#include "IPresetChangeListener.hpp"
#include "Slic3r/Domain/ConfigContainer.hpp"

namespace Slic3r::Biz::Preset {


void PresetInteractor::on_selected_config_container_changed(SelectionId project_id, SelectionId container_id)
{
    m_selection.project_id = project_id;
    m_selection.config_container_id = container_id;

    // update selected config
    auto& ccc = get_or_create_config_container_context(m_selection.project_id, container_id);


    // notify listeners on changes
    m_change_listeners.invoke([&ccc](auto* l) {
        l->on_bed_preset_changed(Slic3r::Preset::Type::TYPE_PRINT, ccc.print);
        l->on_bed_preset_changed(Slic3r::Preset::Type::TYPE_PRINTER, ccc.printer);
        if (!ccc.materials.empty())
            l->on_bed_preset_changed(Slic3r::Preset::Type::TYPE_FILAMENT, ccc.materials[0]);
    });
}

PresetInteractorProjectContext& PresetInteractor::get_or_create_project_context(SelectionId project_id)
{
    auto it = m_project_contexts.find(project_id);
    if (it != m_project_contexts.end())
        return it->second;


    bool _;
    std::tie(it, _) = m_project_contexts.emplace(project_id, PresetInteractorProjectContext{project_id});

    return it->second;
}

PresetInteractorConfigContainerContext& PresetInteractor::get_or_create_config_container_context(SelectionId project_id, SelectionId config_container_id)
{
    auto& project = get_or_create_project_context(project_id);
    auto it = project.config_containers.find(config_container_id);
    if (it != project.config_containers.end())
        return it->second;

    PresetInteractorConfigContainerContext ccc{config_container_id};
    Domain::ConfigContainer& cc = **m_workbench.projects()[m_selection.project_id].find_config_container(config_container_id);
    bool _;
    std::tie(it, _) = project.config_containers.emplace(config_container_id, std::move(ccc));
    return it->second;
}


}