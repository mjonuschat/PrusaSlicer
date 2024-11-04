#include "Slic3r/Biz/Preset/PresetInteractor.hpp"
#include "Slic3r/Biz/Preset/IBedPresetValueChangedListener.hpp"
#include "Slic3r/Biz/Preset/IBedPresetSwitchedListener.hpp"
#include "Slic3r/Domain/ConfigContainer.hpp"

#include <vector>
#include <string>
#include <boost/algorithm/string.hpp>

namespace Slic3r::Biz::Preset {


void PresetInteractor::on_selected_config_container_changed(Domain::SelectionId project_id, Domain::SelectionId container_id)
{
    m_selected_project_id = project_id;
    get_or_create_project_context(project_id).selected_config_container_id = container_id;

    // update selected config
    auto& ccc = get_or_create_config_container_context(m_selected_project_id, container_id);

    ccc.preset_bundle_runtime.update_compatible_prints(m_workbench.preset_bundle(), *ccc.printer.selected_preset);
    ccc.preset_bundle_runtime.update_compatible_materials(m_workbench.preset_bundle(), *ccc.printer.selected_preset, *ccc.print.selected_preset);

    // notify listeners on changes
    m_bed_preset_value_changed_listeners.invoke([&ccc](auto* l) {
        l->on_bed_preset_value_changed(Slic3r::Preset::Type::TYPE_PRINT, ccc.print);
        l->on_bed_preset_value_changed(Slic3r::Preset::Type::TYPE_PRINTER, ccc.printer);
        if (!ccc.materials.empty())
            l->on_bed_preset_value_changed(Slic3r::Preset::Type::TYPE_FILAMENT, ccc.materials[0]);
    });
}

void PresetInteractor::set_preset_state_value(
    Slic3r::Preset::Type preset_type,
    size_t preset_index,
    const std::string& name,
    const boost::any& value,
    int opt_index
)
{
    auto& ccc = selected_config_container_context();
    auto& preset_state = ccc.preset_state(preset_type, preset_index);
    auto& config = preset_state.edited_preset.config;

    set_config_value(config, name, value, opt_index);

    m_bed_preset_value_changed_listeners.invoke([preset_type, &preset_state](auto* l) {
        l->on_bed_preset_value_changed(preset_type, preset_state); });
}

void PresetInteractor::set_preset_state_config_num_extruders(
    Slic3r::Preset::Type preset_type, size_t preset_index, size_t num_extruders
)
{
    auto& ccc = selected_config_container_context();
    auto& preset_state = ccc.preset_state(preset_type, preset_index);
    auto& config = preset_state.edited_preset.config;

    config.set_num_extruders(num_extruders);

    m_bed_preset_value_changed_listeners.invoke([preset_type, &preset_state](auto* l) {
        l->on_bed_preset_value_changed(preset_type, preset_state);
    });
}

void PresetInteractor::set_preset_state(Slic3r::Preset::Type preset_type, size_t preset_index, const DynamicPrintConfig& config)
{
    auto& ccc = selected_config_container_context();
    auto& preset_state = ccc.preset_state(preset_type, preset_index);

    if (preset_state.edited_preset.config.diff(config).empty())
        return;

    preset_state.edited_preset.config = config;

    m_bed_preset_value_changed_listeners.invoke([preset_type, &preset_state](auto* l) {
        l->on_bed_preset_value_changed(preset_type, preset_state); });
}

void PresetInteractor::modify_preset_state(
    Slic3r::Preset::Type preset_type, size_t preset_index, IConfigInteractor::ModifyFunc modify_fn
)
{
    auto& ccc = selected_config_container_context();
    auto& preset_state = ccc.preset_state(preset_type, preset_index);
    auto& config = preset_state.edited_preset.config;
    auto orig_config = config;

    modify_fn(config);

    if (!orig_config.diff(config).empty()) {
        m_bed_preset_value_changed_listeners.invoke([preset_type, &preset_state](auto* l) {
            l->on_bed_preset_value_changed(preset_type, preset_state);
        });
    }
}

void PresetInteractor::select_printer_preset(size_t preset_idx) 
{
    auto& ccc = selected_config_container_context();
    PresetBundle& preset_bundle = m_workbench.preset_bundle();
    preset_bundle.printers.select_preset(preset_idx);
    ccc.printer = create_preset_state(preset_bundle.printers);

    // update PresetBundleRuntime
    ccc.preset_bundle_runtime.update_compatible_prints(preset_bundle, *ccc.printer.selected_preset);
    
    m_bed_preset_switched_listeners.invoke([](auto* l){
        l->on_bed_preset_switched(Slic3r::Preset::TYPE_PRINTER); 
    });

    // TODO: select relevant print if needed
}

void PresetInteractor::select_print_preset(size_t preset_idx) 
{
    auto& ccc = selected_config_container_context();
    PresetBundle& preset_bundle = m_workbench.preset_bundle();
    bool is_sla = ccc.printer_technology() == ptSLA;
    PresetCollection& collection = is_sla ? preset_bundle.sla_prints : preset_bundle.prints;
    collection.select_preset(preset_idx);
    ccc.print = create_preset_state(collection);
    
    // TODO: update PresetBundleRuntime
    
    m_bed_preset_switched_listeners.invoke([](auto* l){
        l->on_bed_preset_switched(Slic3r::Preset::TYPE_PRINT); 
    });
}

void PresetInteractor::select_extruder_preset(size_t extruder_idx, size_t preset_idx) 
{
    // TODO: implement once extruder preset is available
}

void PresetInteractor::select_material_preset(size_t extruder_idx, size_t preset_idx) 
{
    // TODO: update PresetBundleRuntime
    auto& ccc = selected_config_container_context();
    PresetBundle& preset_bundle = m_workbench.preset_bundle();
    bool is_sla = ccc.printer_technology() == ptSLA;
    PresetCollection& collection = is_sla ? preset_bundle.sla_materials : preset_bundle.filaments;
    collection.select_preset(preset_idx);
    ccc.materials[extruder_idx] = create_preset_state(collection);
    
    m_bed_preset_switched_listeners.invoke([](auto* l){
        l->on_bed_preset_switched(Slic3r::Preset::TYPE_FILAMENT); 
    });
    
}

void PresetInteractor::set_config_value(
    DynamicPrintConfig& config, const std::string& name, const boost::any& value, int opt_index
)
{
    if (config.def()->get(name)->type == coBools && config.def()->get(name)->nullable) {
        ConfigOptionBoolsNullable* vec_new = new ConfigOptionBoolsNullable{ boost::any_cast<unsigned char>(value) };
        config.option<ConfigOptionBoolsNullable>(name)->set_at(vec_new, opt_index, 0);
        return;
    }

    const ConfigOptionDef* opt_def = config.def()->get(name);
    switch (opt_def->type) {
    case coFloatOrPercent: {
        std::string str = boost::any_cast<std::string>(value);
        bool percent = false;
        if (str.back() == '%') {
            str.pop_back();
            percent = true;
        }
        double val = std::stod(str); // locale-dependent (on purpose - the input is the actual content of the field)
        config.set_key_value(name, new ConfigOptionFloatOrPercent(val, percent));
        break; }
    case coPercent:
        config.set_key_value(name, new ConfigOptionPercent(boost::any_cast<double>(value)));
        break;
    case coFloat: {
        double& val = config.opt_float(name);
        val = boost::any_cast<double>(value);
        break;
    }
    case coFloatsOrPercents: {
        if (typeid(std::string) == value.type()) {
            std::string str = boost::any_cast<std::string>(value);
            bool percent = false;
            if (str.back() == '%') {
                str.pop_back();
                percent = true;
            }
            double val = std::stod(str
            ); // locale-dependent (on purpose - the input is the actual content of the field)
            ConfigOptionFloatsOrPercents* vec_new = new ConfigOptionFloatsOrPercents({{val, percent}
            });
            config.option<ConfigOptionFloatsOrPercents>(name)->set_at(vec_new, opt_index, opt_index);
        } else {
            auto val = boost::any_cast<std::vector<FloatOrPercent>>(value);
            config.option<ConfigOptionFloatsOrPercents>(name)->values = val;
        }
        break;
    }
    case coPercents: {
        ConfigOptionPercents* vec_new = new ConfigOptionPercents{ boost::any_cast<double>(value) };
        config.option<ConfigOptionPercents>(name)->set_at(vec_new, opt_index, opt_index);
        break;
    }
    case coFloats: {
        ConfigOptionFloats* vec_new = new ConfigOptionFloats{ boost::any_cast<double>(value) };
        config.option<ConfigOptionFloats>(name)->set_at(vec_new, opt_index, opt_index);
        break;
    }
    case coString:
        config.set_key_value(name, new ConfigOptionString(boost::any_cast<std::string>(value)));
        break;
    case coStrings: {
        if (name == "compatible_prints" || name == "compatible_printers" || name == "gcode_substitutions") {
            config.option<ConfigOptionStrings>(name)->values =
                boost::any_cast<std::/*__1::*/vector<std::string>>(value);
        }
        else if (config.def()->get(name)->gui_flags.compare("serialized") == 0) {
            std::string str = boost::any_cast<std::string>(value);
            std::/*__1::*/vector<std::string> values{};
            if (!str.empty()) {
                if (str.back() == ';') str.pop_back();
                // Split a string to multiple strings by a semi - colon.This is the old way of storing multi - string values.
                // Currently used for the post_process config value only.
                boost::split(values, str, boost::is_any_of(";"));
                if (values.size() == 1 && values[0] == "")
                    values.resize(0);
            }
            config.option<ConfigOptionStrings>(name)->values = values;
        }
        else {
            ConfigOptionStrings* vec_new = new ConfigOptionStrings{ boost::any_cast<std::string>(value) };
            config.option<ConfigOptionStrings>(name)->set_at(vec_new, opt_index, 0);
        }
    }
    break;
    case coBool:
        config.set_key_value(name, new ConfigOptionBool(boost::any_cast<bool>(value)));
        break;
    case coBools: {
        ConfigOptionBools* vec_new = new ConfigOptionBools{ boost::any_cast<unsigned char>(value) != 0 };
        config.option<ConfigOptionBools>(name)->set_at(vec_new, opt_index, 0);
        break; }
    case coInt: {
        //config.set_key_value(name, new ConfigOptionInt(boost::any_cast<int>(value)));
        int& val_new = config.opt_int(name);
        val_new = boost::any_cast<int>(value);
        break;
    }
    case coInts: {
        ConfigOptionInts* vec_new = new ConfigOptionInts{ boost::any_cast<int>(value) };
        config.option<ConfigOptionInts>(name)->set_at(vec_new, opt_index, 0);
    }
    break;
    case coEnum: {
        auto* opt = opt_def->default_value.get()->clone();
        opt->setInt(boost::any_cast<int>(value));
        config.set_key_value(name, opt);
    }
    break;
    case coEnums: {
        ConfigOptionEnumsGeneric* vec_new = new ConfigOptionEnumsGeneric(1, boost::any_cast<int>(value));;
        config.option<ConfigOptionEnumsGeneric>(name)->set_at(vec_new, opt_index, 0);
        break; }
    case coPoints: {
        if (name == "bed_shape") {
            config.option<ConfigOptionPoints>(name)->values = boost::any_cast<std::vector<Vec2d>>(value);
            break;
        }
        ConfigOptionPoints* vec_new = new ConfigOptionPoints{ boost::any_cast<Vec2d>(value) };
        config.option<ConfigOptionPoints>(name)->set_at(vec_new, opt_index, 0);
    }
    break;
    case coNone:
        break;
    default:
        break;
    }
}

PresetInteractorProjectContext& PresetInteractor::get_or_create_project_context(Domain::SelectionId project_id)
{
    auto it = m_project_contexts.find(project_id);
    if (it != m_project_contexts.end())
        return it->second;


    bool _;
    std::tie(it, _) = m_project_contexts.emplace(project_id, PresetInteractorProjectContext{project_id});

    return it->second;
}

PresetInteractorConfigContainerContext& PresetInteractor::get_or_create_config_container_context(Domain::SelectionId project_id, Domain::SelectionId config_container_id)
{
    const auto& project = m_workbench.project(project_id);
    auto& project_context = get_or_create_project_context(project_id);
    auto it = project_context.config_containers.find(config_container_id);
    if (it != project_context.config_containers.end())
        return it->second;

    PresetInteractorConfigContainerContext ccc{config_container_id};
    Domain::ConfigContainer& cc = **project.find_config_container(config_container_id);

    // fill ccc presets from cc.print_config
    PresetBundle& preset_bundle = m_workbench.preset_bundle();
    auto pt = preset_bundle.printers.get_selected_preset().printer_technology();
    preset_bundle.load_config_model(project.file_name(), cc.get_print_config());
    ccc.printer = create_preset_state(preset_bundle.printers);
    ccc.print = create_preset_state(pt == PrinterTechnology::ptFFF ? preset_bundle.prints : preset_bundle.sla_prints);
    ccc.materials = {create_preset_state(preset_bundle.materials(pt))};
    ccc.preset_bundle_runtime.update_compatible_prints(preset_bundle, ccc.printer.edited_preset);
    ccc.preset_bundle_runtime.update_compatible_materials(preset_bundle, ccc.printer.edited_preset, ccc.print.edited_preset);

    bool _;
    std::tie(it, _) = project_context.config_containers.emplace(config_container_id, std::move(ccc));
    return it->second;
}

PresetState PresetInteractor::create_preset_state(Slic3r::Preset* selected_preset)
{
    auto& collection = m_workbench.preset_bundle().get_presets(selected_preset->type);
    return {selected_preset, collection.get_preset_parent(*selected_preset)};
}

PresetState PresetInteractor::create_preset_state(PresetCollection& source_with_selected)
{
    auto& preset = source_with_selected.get_selected_preset();
    return {&preset, source_with_selected.get_selected_preset_parent()};
}


const DynamicPrintConfig& PresetConfigInteractor::config() const
{
    return m_parent.selected_config_container_context()
        .preset_state(m_preset_type, m_preset_index)
        .edited_preset.config;
}

const PresetState& PresetConfigInteractor::preset_state() const
{
    return m_parent.selected_config_container_context().preset_state(m_preset_type, m_preset_index);
}

void PresetConfigInteractor::set_config_value(
    const std::string& name, const boost::any& value, int opt_index
)
{
    m_parent.set_preset_state_value(m_preset_type, m_preset_index, name, value, opt_index);
}

void PresetConfigInteractor::set_config_num_extruders(size_t num_extruders)
{
    m_parent.set_preset_state_config_num_extruders(m_preset_type, m_preset_index, num_extruders);
}

void PresetConfigInteractor::set_config(const Slic3r::DynamicPrintConfig& config)
{
    m_parent.set_preset_state(m_preset_type, m_preset_index, config);
}

void PresetConfigInteractor::modify_config(IConfigInteractor::ModifyFunc mod_fn)
{
    m_parent.modify_preset_state(m_preset_type, m_preset_index, mod_fn);
}

}