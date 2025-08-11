#pragma once

#include <unordered_map>

#include "Slic3r/Biz/ISlicingInputChangedListener.hpp"
#include "Slic3r/Biz/Platform/ListenerList.hpp"
#include "Slic3r/Biz/ISelectedConfigContainerChangedListener.hpp"
#include "Slic3r/Biz/Platform/WithListeners.hpp"
#include "Slic3r/Domain/Workbench.hpp"
#include "Slic3r/Biz/Preset/PresetInteractorProjectContext.hpp"
#include "Slic3r/Biz/Preset/IConfigInteractor.hpp"
#include "Slic3r/Biz/ConfigBoxInteractor.hpp"
#include "Slic3r/Biz/CBIObservableList.hpp"

#include "Slic3r/Biz/ObservableListWithSelection.hpp"

#include "libslic3r/PresetBundle.hpp"

namespace Slic3r::Biz::Preset {

class IBedPresetValueChangedListener;
class IBedPresetSwitchedListener;

class PresetInteractor;

/**
 * Implements interaction with Preset underlying DynamicPrintConfig
 */
class LegacyPresetConfigInteractor : public IConfigInteractor
{
public:
    LegacyPresetConfigInteractor(
        PresetInteractor& parent,
        Slic3r::Preset::Type preset_type,
        size_t preset_index = 0
    ) :
        m_parent(parent),
        m_preset_type(preset_type),
        m_preset_index(preset_index)
    {}

    const DynamicPrintConfig& config() const override;
    void set_config_value(const std::string& name, const boost::any& value, int opt_index) override;
    void set_config(const DynamicPrintConfig& config) override;
    void set_config_num_extruders(size_t num_extruders) override;
    void modify_config(ModifyFunc mod_fn) override;

    const PresetState& legacy_preset_state() const;

private:
    PresetInteractor& m_parent;
    Slic3r::Preset::Type m_preset_type;
    size_t m_preset_index;
};

enum class PresetItemType
{
    PrinterPreset,
    PrintPreset,
    ToolPrintPreset,
    MaterialPreset
};

struct PresetItem
{
    std::string id;
    std::string name;
    std::string hw_printer_config_id;
    std::string hw_pritner_config_name;
};

using PresetItemObservableList         = ObservableListWithSelection<PresetItem>;
using PresetItemCompoundObservableList = BatchObservableList<PresetItemObservableList>;

/**
 * Manipulates presets associated with config containers.
 */
class PresetInteractor final :
    public ISelectedConfigContainerChangedListener,
    public WithListeners<IBedPresetValueChangedListener, IBedPresetSwitchedListener, ISlicingInputChangedListener>
{
public:
    explicit PresetInteractor(Domain::Workbench& workbench);

    PresetInteractor(PresetInteractor&&) = default;

    void load_preset_bundle(const std::string& preset_bundle_path, const std::string& config_path);

    const PresetInteractorConfigContainerContext& config_container_context(
        Domain::SelectionId project_id,
        Domain::SelectionId config_container_id
    ) const;

    const PresetInteractorConfigContainerContext& selected_config_container_context() const;

    void prepare_config_container_preset(
        Domain::SelectionId project_id,
        Domain::SelectionId config_container_id
    );
    void initialize_config_container(Domain::ConfigContainer& cc);

    const Domain::Preset::EvaluatedPrinterPreset& current_printer_preset() const;
    const Domain::Preset::SelectedPreset& selected_printer_preset() const;

    PresetItemObservableList& printer_presets() {
        return m_printer_presets;
    }

    const PresetItemObservableList& printer_presets() const {
        return m_printer_presets;
    }

    PresetItemObservableList& print_presets() {
        return m_print_presets;
    }

    const PresetItemObservableList& print_presets() const {
        return m_print_presets;
    }

    PresetItemCompoundObservableList& tool_presets() {
        return m_tool_print_presets;
    }

    const PresetItemCompoundObservableList& tool_presets() const {
        return m_tool_print_presets;
    }

    PresetItemCompoundObservableList& material_presets() {
        return m_material_presets;
    }

    const PresetItemCompoundObservableList& material_presets() const {
        return m_material_presets;
    }

    void select_printer_preset(const std::string& printer_preset_id);
    void select_print_preset(const std::string& id);
    void select_tool_print_preset(size_t tool_index, const std::string& id);
    void select_material_preset(size_t material_index, const std::string& id);

    using ConfigItemModifyFn = std::function<void(Domain::ConfigItem&)>;
    void set_preset_value(
        Domain::ConfigLocation location,
        int element_idx,
        const std::string& name,
        ConfigItemModifyFn modify_fn
    );

    void set_legacy_preset_state_value(
        Slic3r::Preset::Type preset_type,
        size_t preset_index,
        const std::string& name,
        const boost::any& value,
        int opt_index = 0
    );
    void set_legacy_preset_state_config_num_extruders(
        Slic3r::Preset::Type preset_type,
        size_t preset_index,
        size_t num_extruders
    );
    void set_legacy_preset_state(
        Slic3r::Preset::Type preset_type,
        size_t preset_index,
        const DynamicPrintConfig& config
    );
    void modify_legacy_preset_state(
        Slic3r::Preset::Type preset_type,
        size_t preset_index,
        IConfigInteractor::ModifyFunc modify_fn
    );

    const PresetCollection& preset_collection(Slic3r::Preset::Type preset_type) const;

    const PresetBundle& preset_bundle_legacy() const;

    void select_legacy_preset(Slic3r::Preset::Type preset_type, size_t preset_index, size_t collection_index);

    void on_selected_config_container_changed(
        Domain::SelectionId project_id,
        Domain::SelectionId bed_id
    ) override;

    ConfigBoxInteractor& printer_cbi();
    ConfigBoxInteractor& print_cbi();
    CBIObservableList& material_cbi_list();
    CBIObservableList& tool_cbi_list();

    void set_item_value(const Domain::ConfigItem& item, const Domain::ConfigValue& value, size_t index = 0);

private:
    friend class LegacyPresetConfigInteractor;
    using ProjectContexts = std::unordered_map<Domain::SelectionId, PresetInteractorProjectContext>;
    PresetInteractorConfigContainerContext& mutable_selected_config_container_context();

    Domain::Preset::SelectedPreset& mutable_selected_printer_presets();

    ProjectContexts::const_iterator get_project_context(Domain::SelectionId project_id) const
    {
        return m_project_contexts.find(project_id);
    }

    ProjectContexts::iterator get_project_context(Domain::SelectionId project_id)
    {
        return m_project_contexts.find(project_id);
    }

    PresetInteractorProjectContext& get_or_create_project_context(Domain::SelectionId project_id);
    PresetInteractorConfigContainerContext& get_or_create_config_container_context(
        Domain::SelectionId project_id,
        Domain::SelectionId config_container_id
    );

    const std::string& selected_hw_config_id() const;
    void fill_config_container_with_selected_preset(
        Domain::ConfigContainer& cc,
        const std::string& printer_preset_id
    );
    void fill_printer_presets();
    void fill_print_presets(
        const Domain::Preset::EvaluatedPrinterPreset& selected_printer_ep,
        Domain::Preset::SelectedPreset& selected_preset
    );
    void fill_tools_presets(
        const Domain::Preset::EvaluatedPrinterPreset& selected_printer_ep,
        const Domain::Preset::EvaluatedPrintPreset& selected_print_ep,
        Domain::Preset::SelectedPreset& selected_preset
    );
    void fill_materials_presets(
        const Domain::Preset::EvaluatedPrintPreset& selected_print_ep,
        Domain::Preset::SelectedPreset& selected_preset
    );

    void invoke_slicing_input_changed();

    void select_legacy_printer_preset(size_t preset_idx);
    void select_legacy_print_preset(size_t preset_idx);
    void select_legacy_extruder_preset(size_t extruder_idx, size_t preset_idx);
    void select_legacy_material_preset(size_t extruder_idx, size_t preset_idx);

    PresetCollection& legacy_preset_collection(Slic3r::Preset::Type preset_type);

    static PresetState create_preset_state(PresetCollection& source_with_selected);
    PresetState create_preset_state(Slic3r::Preset* selected_preset);

    static void set_config_value(
        DynamicPrintConfig& config,
        const std::string& name,
        const boost::any& value,
        int opt_index
    );

private:
    using SetAccessorMap = std::map<const ConfigBoxInteractor*, ConfigBoxInteractor::SetAccessor>;

    Domain::Workbench& m_workbench;

    PresetItemObservableList m_printer_presets;
    PresetItemObservableList m_print_presets;
    PresetItemCompoundObservableList::WriteAccessor m_tool_print_presets_writer;
    PresetItemCompoundObservableList m_tool_print_presets{m_tool_print_presets_writer};
    PresetItemCompoundObservableList::WriteAccessor m_material_presets_writer;
    PresetItemCompoundObservableList m_material_presets{m_material_presets_writer};

    ProjectContexts m_project_contexts;

    ConfigBoxInteractor m_printer_cbi;
    ConfigBoxInteractor m_print_cbi;
    CBIObservableList m_material_cbi_list;
    CBIObservableList m_tool_cbi_list;
    SetAccessorMap m_cbi_accessors; ///< Contains All SetAccessors currently in use

    Domain::SelectionId m_selected_project_id{Domain::INVALID_ID};
};
} // namespace Slic3r::Biz::Preset
