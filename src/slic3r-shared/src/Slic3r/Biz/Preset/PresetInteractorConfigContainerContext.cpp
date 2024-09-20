///|/ Copyright (c) Prusa Research 2017 - 2023 Oleksandra Iushchenko @YuSanka, David Kocík @kocikdav, Enrico Turri @enricoturri1966, Lukáš Matěna @lukasmatena, Vojtěch Bubník @bubnikv, Vojtěch Král @vojtechkral
///|/ Copyright (c) SuperSlicer 2021 Remi Durand @supermerill
///|/ Copyright (c) 2019 John Drake @foxox
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/

#include "PresetInteractorConfigContainerContext.hpp"
#include "libslic3r/Preset.hpp"
#include <boost/algorithm/clamp.hpp>
#include "Slic3r/Assert.hpp"

namespace Slic3r::Biz::Preset {

PresetState& PresetInteractorConfigContainerContext::preset_state(Slic3r::Preset::Type preset_type, size_t preset_index)
{
    switch(preset_type) {
    case Slic3r::Preset::TYPE_FILAMENT:
    case Slic3r::Preset::TYPE_SLA_MATERIAL:
        return materials[preset_index];

    case Slic3r::Preset::TYPE_PRINTER:
        return printer;

    case Slic3r::Preset::TYPE_PRINT:
    case Slic3r::Preset::TYPE_SLA_PRINT:
        return print;

    default:
        PANIC("Unsupported preset type");

    // TODO: Extruders
    }

    PANIC("Wrong preset state type");
}

const PresetState& PresetInteractorConfigContainerContext::preset_state(Slic3r::Preset::Type preset_type, size_t preset_index) const
{
    switch(preset_type) {
    case Slic3r::Preset::TYPE_FILAMENT:
    case Slic3r::Preset::TYPE_SLA_MATERIAL:
        return materials[preset_index];

    case Slic3r::Preset::TYPE_PRINTER:
        return printer;

    case Slic3r::Preset::TYPE_PRINT:
    case Slic3r::Preset::TYPE_SLA_PRINT:
        return print;

    default:
        PANIC("Unsupported preset type");

        // TODO: Extruders
    }

    PANIC("Wrong preset state type");
}

DynamicPrintConfig PresetInteractorConfigContainerContext::full_config() const
{
    return (printer.edited_preset.printer_technology() == ptFFF) ?
        full_fff_config() :
        full_sla_config();
}

DynamicPrintConfig PresetInteractorConfigContainerContext::full_fff_config() const
{    
    DynamicPrintConfig out;
    out.apply(FullPrintConfig::defaults());
    out.apply(print.edited_preset.config);
    // Add the default filament preset to have the "filament_preset_id" defined.
//!    out.apply(this->filaments.default_preset().config);
	out.apply(printer.edited_preset.config);
//!    out.apply(project_config);

    auto   *nozzle_diameter = dynamic_cast<const ConfigOptionFloats*>(out.option("nozzle_diameter"));
    size_t  num_extruders   = nozzle_diameter->values.size();
    // Collect the "compatible_printers_condition" and "inherits" values over all presets (print, filaments, printers) into a single vector.
    std::vector<std::string> compatible_printers_condition;
    std::vector<std::string> compatible_prints_condition;
    std::vector<std::string> inherits;
    compatible_printers_condition.emplace_back(print.edited_preset.compatible_printers_condition());
    inherits                     .emplace_back(print.edited_preset.inherits());

    if (num_extruders <= 1) {
        out.apply(materials[0].edited_preset.config);
        compatible_printers_condition.emplace_back(materials[0].edited_preset.compatible_printers_condition());
        compatible_prints_condition  .emplace_back(materials[0].edited_preset.compatible_prints_condition());
        inherits                     .emplace_back(materials[0].edited_preset.inherits());
    } else {
        // Retrieve filament presets and build a single config object for them.
        // First collect the filament configurations based on the user selection of this->filament_presets.
        // Here this->filaments.find_preset() and this->filaments.first_visible() return the edited copy of the preset if active.
        std::vector<const DynamicPrintConfig*> filament_configs;
        for (const auto& material : materials)
            filament_configs.emplace_back(&material.edited_preset.config);
//! ?		while (filament_configs.size() < num_extruders)
//! ?            filament_configs.emplace_back(&this->filaments.first_visible().config);
        for (const DynamicPrintConfig *cfg : filament_configs) {
            // The compatible_prints/printers_condition() returns a reference to configuration key, which may not yet exist.
            DynamicPrintConfig &cfg_rw = *const_cast<DynamicPrintConfig*>(cfg);
            compatible_printers_condition.emplace_back(Slic3r::Preset::compatible_printers_condition(cfg_rw));
            compatible_prints_condition  .emplace_back(Slic3r::Preset::compatible_prints_condition(cfg_rw));
            inherits                     .emplace_back(Slic3r::Preset::inherits(cfg_rw));
        }
/*  //! Is it need now, when we use separate confids for each extruder ?
        // Option values to set a ConfigOptionVector from.
        std::vector<const ConfigOption*> filament_opts(num_extruders, nullptr);
        // loop through options and apply them to the resulting config.
        for (const t_config_option_key &key : this->filaments.default_preset().config.keys()) {
			if (key == "compatible_prints" || key == "compatible_printers")
				continue;
            // Get a destination option.
            ConfigOption *opt_dst = out.option(key, false);
            if (opt_dst->is_scalar()) {
                // Get an option, do not create if it does not exist.
                const ConfigOption *opt_src = filament_configs.front()->option(key);
                if (opt_src != nullptr)
                    opt_dst->set(opt_src);
            } else {
                // Setting a vector value from all filament_configs.
                for (size_t i = 0; i < filament_opts.size(); ++ i)
                    filament_opts[i] = filament_configs[i]->option(key);
                static_cast<ConfigOptionVectorBase*>(opt_dst)->set(filament_opts);
            }
        }
*/
    }

	// Don't store the "compatible_printers_condition" for the printer profile, there is none.
    inherits.emplace_back(printer.edited_preset.inherits());

    // These value types clash between the print and filament profiles. They should be renamed.
    out.erase("compatible_prints");
    out.erase("compatible_prints_condition");
    out.erase("compatible_printers");
    out.erase("compatible_printers_condition");
    out.erase("inherits");
    
    static const char *keys[] = { "perimeter", "infill", "solid_infill", "support_material", "support_material_interface" };
    for (size_t i = 0; i < sizeof(keys) / sizeof(keys[0]); ++ i) {
        std::string key = std::string(keys[i]) + "_extruder";
        auto *opt = dynamic_cast<ConfigOptionInt*>(out.option(key, false));
        assert(opt != nullptr);
        opt->value = boost::algorithm::clamp<int>(opt->value, 0, int(num_extruders));
    }

    out.option<ConfigOptionString >("print_settings_id",    true)->value  = print.selected_preset->name;
    auto& filament_settings_id = out.option<ConfigOptionStrings>("filament_settings_id", true)->values;
    filament_settings_id.clear();
    for (const auto& material : materials)
        filament_settings_id.emplace_back(material.selected_preset->name);
    out.option<ConfigOptionString >("printer_settings_id",  true)->value  = this->printer.selected_preset->name;
//!    out.option<ConfigOptionString >("physical_printer_settings_id", true)->value = this->physical_printers.get_selected_printer_name();

    // Serialize the collected "compatible_printers_condition" and "inherits" fields.
    // There will be 1 + num_exturders fields for "inherits" and 2 + num_extruders for "compatible_printers_condition" stored.
    // The vector will not be stored if all fields are empty strings.
    auto add_if_some_non_empty = [&out](std::vector<std::string> &&values, const std::string &key) {
        bool nonempty = false;
        for (const std::string &v : values)
            if (! v.empty()) {
                nonempty = true;
                break;
            }
        if (nonempty)
            out.set_key_value(key, new ConfigOptionStrings(std::move(values)));
    };
    add_if_some_non_empty(std::move(compatible_printers_condition), "compatible_printers_condition_cummulative");
    add_if_some_non_empty(std::move(compatible_prints_condition),   "compatible_prints_condition_cummulative");
    add_if_some_non_empty(std::move(inherits),                      "inherits_cummulative");

	out.option<ConfigOptionEnumGeneric>("printer_technology", true)->value = ptFFF;
    return out;
}

DynamicPrintConfig PresetInteractorConfigContainerContext::full_sla_config() const
{    
    DynamicPrintConfig out;
    out.apply(SLAFullPrintConfig::defaults());
    out.apply(print.edited_preset.config);
    out.apply(materials[0].edited_preset.config);
    out.apply(printer.edited_preset.config);
    // There are no project configuration values as of now, the project_config is reserved for FFF printers.
//    out.apply(this->project_config);

    // Collect the "compatible_printers_condition" and "inherits" values over all presets (sla_prints, sla_materials, printers) into a single vector.
    std::vector<std::string> compatible_printers_condition;
	std::vector<std::string> compatible_prints_condition;
    std::vector<std::string> inherits;
    compatible_printers_condition.emplace_back(print.edited_preset.compatible_printers_condition());
	inherits					 .emplace_back(print.edited_preset.inherits());
    compatible_printers_condition.emplace_back(materials[0].edited_preset.compatible_printers_condition());
	compatible_prints_condition  .emplace_back(materials[0].edited_preset.compatible_prints_condition());
    inherits                     .emplace_back(materials[0].edited_preset.inherits());
    inherits                     .emplace_back(printer.edited_preset.inherits());

    // These two value types clash between the print and filament profiles. They should be renamed.
    out.erase("compatible_printers");
    out.erase("compatible_printers_condition");
    out.erase("inherits");
    
    out.option<ConfigOptionString >("sla_print_settings_id",    true)->value  = print.selected_preset->name;
    out.option<ConfigOptionString >("sla_material_settings_id", true)->value  = materials[0].selected_preset->name;
    out.option<ConfigOptionString >("printer_settings_id",      true)->value  = printer.selected_preset->name;
//!    out.option<ConfigOptionString >("physical_printer_settings_id", true)->value = this->physical_printers.get_selected_printer_name();

    // Serialize the collected "compatible_printers_condition" and "inherits" fields.
    // There will be 1 + num_exturders fields for "inherits" and 2 + num_extruders for "compatible_printers_condition" stored.
    // The vector will not be stored if all fields are empty strings.
    auto add_if_some_non_empty = [&out](std::vector<std::string> &&values, const std::string &key) {
        bool nonempty = false;
        for (const std::string &v : values)
            if (! v.empty()) {
                nonempty = true;
                break;
            }
        if (nonempty)
            out.set_key_value(key, new ConfigOptionStrings(std::move(values)));
    };
    add_if_some_non_empty(std::move(compatible_printers_condition), "compatible_printers_condition_cummulative");
    add_if_some_non_empty(std::move(compatible_prints_condition),   "compatible_prints_condition_cummulative");
    add_if_some_non_empty(std::move(inherits),                      "inherits_cummulative");

	out.option<ConfigOptionEnumGeneric>("printer_technology", true)->value = ptSLA;
	return out;
}

} // namespace Slic3r::Biz::Preset
