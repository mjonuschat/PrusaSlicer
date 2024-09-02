#pragma once

#include <unordered_map>

#include "Slic3r/Biz/ListenerList.hpp"
#include "Slic3r/Biz/ISelectedConfigContainerChangedListener.hpp"
#include "Slic3r/Domain/Workbench.hpp"
#include "Slic3r/Biz/Preset/PresetInteractorProjectContext.hpp"

namespace Slic3r::Biz::Preset {

class IPresetChangeListener;

class PresetInteractor final : public ISelectedConfigContainerChangedListener
{
public:
    explicit PresetInteractor(Domain::Workbench& workbench) : m_workbench(workbench) {}

    PresetInteractor(PresetInteractor&&) = default;

    bool add_change_listener(IPresetChangeListener* listener)
    {
        return m_change_listeners.add(listener);
    }

    bool remove_change_listener(IPresetChangeListener* listener)
    {
        return m_change_listeners.remove(listener);
    }

    void on_selected_config_container_changed(SelectionId project_id, SelectionId bed_id) override;

private:
    PresetInteractorProjectContext& get_or_create_project_context(SelectionId project_id);
    PresetInteractorConfigContainerContext& get_or_create_config_container_context(SelectionId project_id, SelectionId config_container_id);

private:
    struct Selection
    {
        SelectionId project_id{INVALID_ID};
        SelectionId config_container_id{INVALID_ID};
    };

    Domain::Workbench& m_workbench;
    ListenerList<IPresetChangeListener> m_change_listeners;

    using ProjectContexts = std::unordered_map<SelectionId, PresetInteractorProjectContext>;
    ProjectContexts m_project_contexts;

    Selection m_selection;
};
}
