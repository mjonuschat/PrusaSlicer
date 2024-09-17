#pragma once

#include <unordered_map>

#include "Slic3r/Biz/ListenerList.hpp"
#include "Slic3r/Biz/ISelectedConfigContainerChangedListener.hpp"
#include "Slic3r/Domain/Workbench.hpp"
#include "Slic3r/Biz/Preset/PresetInteractorProjectContext.hpp"
#include "Slic3r/Biz/Preset/IConfigInteractor.hpp"

namespace Slic3r::Biz::Preset {

class IBedPresetValueChangedListener;
class IBedPresetSwitchedListener;

class PresetInteractor;

/**
 * Implements interaction with Preset underlying DynamicPrintConfig
 */
class PresetConfigInteractor : public IConfigInteractor
{
public:
    PresetConfigInteractor(PresetInteractor& parent, Slic3r::Preset::Type preset_type, size_t preset_index = 0)
        : m_parent(parent), m_preset_type(preset_type), m_preset_index(preset_index)
    {}
    const DynamicPrintConfig& config() const override;
    void set_config_value(const std::string& name, const boost::any& value, int opt_index) override;
    void set_config(const DynamicPrintConfig& config) override;
    void set_config_num_extruders(size_t num_extruders) override;
    void modify_config(ModifyFunc mod_fn) override;

    const PresetState& preset_state() const;
private:
    PresetInteractor& m_parent;
    Slic3r::Preset::Type m_preset_type;
    size_t m_preset_index;
};

/**
 * Manipulates presets associated with config containers.
 */
class PresetInteractor final : public ISelectedConfigContainerChangedListener
{
public:
    explicit PresetInteractor(Domain::Workbench& workbench) : m_workbench(workbench) {}

    PresetInteractor(PresetInteractor&&) = default;

    PresetInteractorConfigContainerContext& selected_config_container_context()
    {
        auto& project_ctx = get_project_context(m_selected_project_id)->second;
        auto& cccs = project_ctx.config_containers;
        return cccs.find(project_ctx.selected_config_container_id)->second;
    }


    void set_preset_state_value(Slic3r::Preset::Type preset_type, size_t preset_index, const std::string& name, const boost::any& value, int opt_index = 0);
    void set_preset_state_config_num_extruders(Slic3r::Preset::Type preset_type, size_t preset_index, size_t num_extruders);
    void set_preset_state(Slic3r::Preset::Type preset_type, size_t preset_index, const DynamicPrintConfig& config);
    void modify_preset_state(Slic3r::Preset::Type preset_type, size_t preset_index, IConfigInteractor::ModifyFunc modify_fn);

    const PresetCollection& preset_collection(Slic3r::Preset::Type preset_type) const
    {
        const auto& pb = m_workbench.preset_bundle();
        return pb.get_presets(preset_type);
    }
    
    void select_preset(Slic3r::Preset::Type preset_type, size_t preset_index, size_t collection_index)
    {
        auto& ccc = selected_config_container_context();
        auto& collection = preset_collection(preset_type);
        auto it = collection.begin() + collection_index;
        auto& preset = *it;
        ccc.preset_state(preset_type, preset_index) = create_preset_state(&preset);
    }

    
    
    bool add_bed_preset_value_changed_listener(IBedPresetValueChangedListener* listener)
    {
        return m_bed_preset_value_changed_listeners.add(listener);
    }

    bool remove_bed_preset_value_changed_listener(IBedPresetValueChangedListener* listener)
    {
        return m_bed_preset_value_changed_listeners.remove(listener);
    }

    bool add_bed_preset_switched_listener(IBedPresetSwitchedListener* listener)
    {
        return m_bed_preset_switched_listeners.add(listener);
    }

    bool remove_bed_preset_switched_listener(IBedPresetSwitchedListener* listener)
    {
        return m_bed_preset_switched_listeners.remove(listener);
    }

    void on_selected_config_container_changed(SelectionId project_id, SelectionId bed_id) override;

private:
    using ProjectContexts = std::unordered_map<SelectionId, PresetInteractorProjectContext>;

    ProjectContexts::const_iterator get_project_context(SelectionId project_id) const
    {
        return m_project_contexts.find(project_id);
    }

    ProjectContexts::iterator get_project_context(SelectionId project_id)
    {
        return m_project_contexts.find(project_id);
    }

    PresetInteractorProjectContext& get_or_create_project_context(SelectionId project_id);
    PresetInteractorConfigContainerContext& get_or_create_config_container_context(SelectionId project_id, SelectionId config_container_id);
    
    void select_printer_preset(size_t preset_idx);
    void select_print_preset(size_t preset_idx);
    void select_extruder_preset(size_t extruder_idx, size_t preset_idx);
    void select_material_preset(size_t extruder_idx, size_t preset_idx);

    PresetCollection& preset_collection(Slic3r::Preset::Type preset_type)
    {
        auto& pb = m_workbench.preset_bundle();
        return pb.get_presets(preset_type);
    }


    static PresetState create_preset_state(PresetCollection& source_with_selected);
    PresetState create_preset_state(Slic3r::Preset* selected_preset);

    static void set_config_value(
        DynamicPrintConfig& config, const std::string& name, const boost::any& value, int opt_index
    );

private:

    Domain::Workbench& m_workbench;
    ListenerList<IBedPresetValueChangedListener> m_bed_preset_value_changed_listeners;
    ListenerList<IBedPresetSwitchedListener> m_bed_preset_switched_listeners;

    ProjectContexts m_project_contexts;

    SelectionId m_selected_project_id{INVALID_ID};
};
}
