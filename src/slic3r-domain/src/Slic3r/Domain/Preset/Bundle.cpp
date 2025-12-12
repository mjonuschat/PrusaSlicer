#include "Slic3r/Domain/Preset/Bundle.hpp"

#include <ranges>

namespace Slic3r::Domain::Preset {

const HwPrinterConfig* VendorBundle::find_printer_config(const std::string& id) const
{
    auto it = std::ranges::
        find_if(printer_configs, [&id](const HwPrinterConfig& config) { return config.id == id; });
    if (it == printer_configs.end())
        return nullptr;
    return &(*it);
}

HwPrinterConfig* VendorBundle::find_printer_config(const std::string& id)
{
    auto it = std::ranges::
        find_if(printer_configs, [&id](const HwPrinterConfig& config) { return config.id == id; });
    if (it == printer_configs.end())
        return nullptr;
    return &(*it);
}


const EvaluatedPrinterPreset* Bundle::find_printer_preset(
    const std::string& printer_hw_config_id,
    const std::string& printer_preset_id
) const
{
    const auto& printer_presets = evaluated_presets.find(printer_hw_config_id);

    if (printer_presets == evaluated_presets.end())
        return nullptr;

    auto it = std::ranges::find_if(printer_presets->second, [&printer_preset_id](const EvaluatedPrinterPreset& p) {
        return p.preset.id == printer_preset_id;
    });
    if (it != printer_presets->second.end())
        return &*it;
    return nullptr;
}

const HwPrinterConfig* Bundle::find_config_with_same_values(const HwPrinterConfig& printer_config) const
{
    auto it = std::ranges::find_if(
        printer_configs,
        [&printer_config](const auto& pair) { return pair.second.has_same_values(printer_config); }
    );
    return it == printer_configs.end() ? nullptr : &(it->second);
}

const EvaluatedPrinterPreset* Bundle::find_printer_preset_with_same_values(
    const std::string& hw_config_id,
    const EvaluatedPrinterPreset::Preset& printer_preset
) const
{
    const auto& evaluated_printer_presets = evaluated_presets.at(hw_config_id);
    return find_preset_with_same_value(printer_preset, evaluated_printer_presets);
}

namespace {
void visit_all_printers(
    EvaluatedPrinterPresets& presets,
    const std::function<void(EvaluatedPrinterPreset&)>& visitor
)
{
    std::ranges::for_each(presets | std::views::values | std::views::join, visitor);
}

void visit_all_printers(
    const EvaluatedPrinterPresets& presets,
    const std::function<void(const EvaluatedPrinterPreset&)>& visitor
)
{
    std::ranges::for_each(presets | std::views::values | std::views::join, visitor);
}

void visit_all_prints(
    EvaluatedPrinterPresets& presets,
    const std::function<void(const EvaluatedPrinterPreset& printer, EvaluatedPrintPreset& print)>&
        visitor
)
{
    visit_all_printers(
        presets,
        [&visitor](EvaluatedPrinterPreset& printer)
        {
            auto bound_visitor = [&](EvaluatedPrintPreset& print) { visitor(printer, print); };
            std::ranges::for_each(printer.prints, bound_visitor);
        }
    );
}

void visit_all_prints(
    const EvaluatedPrinterPresets& presets,
    const std::function<void(const EvaluatedPrinterPreset& printer, const EvaluatedPrintPreset& print)>&
        visitor
)
{
    visit_all_printers(
        presets,
        [&visitor](const EvaluatedPrinterPreset& printer)
        {
            auto bound_visitor = [&](const EvaluatedPrintPreset& print) { visitor(printer, print); };
            std::ranges::for_each(printer.prints, bound_visitor);
        }
    );
}

void visit_all_prints(
    EvaluatedPrinterPresets& presets,
    const std::function<void(EvaluatedPrintPreset& print)>& visitor
)
{
    visit_all_prints(
        presets,
        [&](const EvaluatedPrinterPreset& printer, EvaluatedPrintPreset& print) { visitor(print); }
    );
}

void visit_all_tool_prints(
    EvaluatedPrinterPresets& presets,
    const std::function<void(const EvaluatedPrinterPreset& printer, EvaluatedToolPrintPreset&)>&
        visitor
)
{
    visit_all_prints(
        presets,
        [&visitor](const EvaluatedPrinterPreset& printer, EvaluatedPrintPreset& print)
        {
            auto bound_visitor = [&](EvaluatedToolPrintPreset& tool_print_preset)
            { visitor(printer, tool_print_preset); };
            std::ranges::for_each(print.tools | std::views::join, bound_visitor);
        }
    );
}

void visit_all_tool_prints(
    const EvaluatedPrinterPresets& presets,
    const std::function<void(const EvaluatedPrinterPreset& printer, const EvaluatedToolPrintPreset&)>&
        visitor
)
{
    visit_all_prints(
        presets,
        [&visitor](const EvaluatedPrinterPreset& printer, const EvaluatedPrintPreset& print)
        {
            auto bound_visitor = [&](const EvaluatedToolPrintPreset& tool_print_preset)
            { visitor(printer, tool_print_preset); };
            std::ranges::for_each(print.tools | std::views::join, bound_visitor);
        }
    );
}

void visit_all_tool_prints(
    EvaluatedPrinterPresets& presets,
    const std::function<void(EvaluatedToolPrintPreset&)>& visitor
)
{
    visit_all_tool_prints(
        presets,
        [&](const auto&, EvaluatedToolPrintPreset& preset) { visitor(preset); }
    );
}

void visit_all_materials(
    EvaluatedPrinterPresets& presets,
    const std::function<void(const EvaluatedPrinterPreset& printer, EvaluatedMaterialPreset&)>&
        visitor
)
{
    visit_all_prints(
        presets,
        [&visitor](const EvaluatedPrinterPreset& printer, EvaluatedPrintPreset& print)
        {
            auto bound_visitor = [&](EvaluatedMaterialPreset& material_preset)
            { visitor(printer, material_preset); };
            std::ranges::for_each(print.materials | std::views::join, bound_visitor);
        }
    );
}

void visit_all_materials(
    const EvaluatedPrinterPresets& presets,
    const std::function<void(const EvaluatedPrinterPreset& printer, const EvaluatedMaterialPreset&)>&
        visitor
)
{
    visit_all_prints(
        presets,
        [&visitor](const EvaluatedPrinterPreset& printer, const EvaluatedPrintPreset& print)
        {
            auto bound_visitor = [&](const EvaluatedMaterialPreset& material_preset)
            { visitor(printer, material_preset); };
            std::ranges::for_each(print.materials | std::views::join, bound_visitor);
        }
    );
}

void visit_all_materials(
    EvaluatedPrinterPresets& presets,
    const std::function<void(EvaluatedMaterialPreset&)>& visitor
)
{
    visit_all_materials(
        presets,
        [&](const auto&, EvaluatedMaterialPreset& preset) { visitor(preset); }
    );
}

template <typename T>
auto make_update_visitor(const typename T::Preset& preset)
{
    return [&](T& dest)
    {
        if (dest.preset.id == preset.id)
            dest.preset = preset;
    };
}

template <typename T>
auto make_copy_visitor(const typename T::Preset& preset, const std::string& printer_id)
{
    return [&](std::vector<T>& dest)
    {
        auto source_it = std::ranges::find_if(
            dest,
            [&](const auto& eval_preset) { return eval_preset.preset.id == printer_id; }
        );
        if (source_it != dest.end()) {
            T copy      = *source_it;
            copy.preset = preset;
            dest.emplace_back(std::move(copy));
        }
    };
}

void update_preset_names_from_evaluated_presets(
    VendorBundles& vendor_bundles,
    const EvaluatedPrinterPresets& evaluated_presets
)
{
    using PresetNameMap = std::map<std::string, PresetName>;
    using AllPresetNameMap = std::map<PresetKind, PresetNameMap>;
    using VendorPresetNameMap = std::map<std::string, AllPresetNameMap>;

    VendorPresetNameMap all_preset_names;
    const auto update_name = [&all_preset_names](PresetKind kind, const std::string& vendor_id, const std::string& name, const std::string& id, PresetOrigin origin)
    {
        auto& preset_names = all_preset_names[vendor_id][kind];
        auto it = preset_names.find(name);
        if (it != preset_names.end()) {
            ASSERT(it->second.origin == origin);
            it->second.id.insert(id);
        } else {
            preset_names.emplace(name, PresetName{name, std::set{id}, origin});
        }
    };

    for (const EvaluatedPrinterPreset& printer :
         evaluated_presets | std::views::values | std::views::join)
    {
        const auto& vendor_id = printer.hw_config.vendor_id;
        auto& vendor_bundle   = vendor_bundles.at(vendor_id);
        auto tech             = printer.hw_config.technology;
        update_name(printer_kind(tech), vendor_id, printer.preset.name, printer.preset.id, printer.preset.origin);

        for (const auto& print : printer.prints) {
            update_name(print_kind(tech), vendor_id, print.preset.name, print.preset.id, print.preset.origin);

            for (const auto& tool : print.tools | std::views::join) {
                update_name(tool_print_kind(tech), vendor_id, tool.preset.name, tool.preset.id, tool.preset.origin);
            }

            for (const auto& material : print.materials | std::views::join) {
                update_name(material_kind(tech), vendor_id, material.preset.name, material.preset.id, material.preset.origin);
            }
        }
    }

    for (const auto& [vendor_id, all_presets_names] : all_preset_names) {
        auto& vendor_bundle = vendor_bundles.at(vendor_id);
        for (const auto& [kind, names] : all_presets_names) {
            PresetNames dest_names;
            std::ranges::copy(names | std::views::values, std::back_inserter(dest_names));
            vendor_bundle.preset_names[kind] = std::move(dest_names);
        }
    }
}

}


void Bundle::update_presets(const EvaluatedPrinterPreset::Preset& preset)
{
    visit_all_printers(evaluated_presets, make_update_visitor<EvaluatedPrinterPreset>(preset));
    update_preset_names_from_evaluated_presets(vendor_bundles, evaluated_presets);
}

void Bundle::update_presets(const EvaluatedPrintPreset::Preset& preset)
{
    visit_all_prints(evaluated_presets, make_update_visitor<EvaluatedPrintPreset>(preset));
    update_preset_names_from_evaluated_presets(vendor_bundles, evaluated_presets);
}

void Bundle::update_presets(const EvaluatedToolPrintPreset::Preset& preset)
{
    visit_all_tool_prints(evaluated_presets, make_update_visitor<EvaluatedToolPrintPreset>(preset));
    update_preset_names_from_evaluated_presets(vendor_bundles, evaluated_presets);
}

void Bundle::update_presets(const EvaluatedMaterialPreset::Preset& preset)
{
    visit_all_materials(evaluated_presets, make_update_visitor<EvaluatedMaterialPreset>(preset));
    update_preset_names_from_evaluated_presets(vendor_bundles, evaluated_presets);
}

void Bundle::copy_preset(
    const EvaluatedPrinterPreset::Preset& preset,
    const std::string& printer_id
)
{
    std::ranges::for_each(
        evaluated_presets | std::views::values,
        make_copy_visitor<EvaluatedPrinterPreset>(preset, printer_id)
    );

    update_preset_names_from_evaluated_presets(vendor_bundles, evaluated_presets);
}

void Bundle::copy_preset(
    const EvaluatedPrintPreset::Preset& preset,
    const std::string& print_id
)
{
    auto visitor = make_copy_visitor<EvaluatedPrintPreset>(preset, print_id);
    visit_all_printers(evaluated_presets, [&](EvaluatedPrinterPreset& printer)
    {
        visitor(printer.prints);
    });

    update_preset_names_from_evaluated_presets(vendor_bundles, evaluated_presets);
}

void Bundle::copy_preset(
    const EvaluatedToolPrintPreset::Preset& preset,
    const std::string& tool_print_id
)
{
    auto visitor = make_copy_visitor<EvaluatedToolPrintPreset>(preset, tool_print_id);
    visit_all_prints(evaluated_presets, [&](EvaluatedPrintPreset& print)
    {
        std::ranges::for_each(print.tools, visitor);
    });

    update_preset_names_from_evaluated_presets(vendor_bundles, evaluated_presets);
}

void Bundle::copy_preset(
    const EvaluatedMaterialPreset::Preset& preset,
    const std::string& material_id
)
{
    auto visitor = make_copy_visitor<EvaluatedMaterialPreset>(preset, material_id);
    visit_all_prints(evaluated_presets, [&](EvaluatedPrintPreset& print)
    {
        std::ranges::for_each(print.materials, visitor);
    });

    update_preset_names_from_evaluated_presets(vendor_bundles, evaluated_presets);
}

PresetParentPaths Bundle::find_usage_of_preset(PresetKind kind, const std::string& preset_id) const
{
    PresetParentPaths ret;
    visit_all_printers(evaluated_presets, [&](const EvaluatedPrinterPreset& printer)
    {
        if (is_printer(kind)) {
            if (printer.preset.id == preset_id) {
                ret.emplace_back(printer.hw_config.id);
            }
            return;
        }

        const bool print_selected = is_print(kind);
        const bool tool_selected = is_tool_print(kind);
        const bool material_selected = is_material(kind);
        for (const auto& print : printer.prints) {
            if (print_selected) {
                if (print.preset.id == preset_id) {
                    ret.emplace_back(printer.hw_config.id, printer.preset.id);
                }
            } else {
                if (tool_selected) {
                    for (size_t i = 0, n = print.tools.size(); i < n; ++i) {
                        for (const auto& tool : print.tools.at(i)) {
                            if (tool.preset.id == preset_id) {
                                ret.emplace_back(
                                    printer.hw_config.id,
                                    printer.preset.id,
                                    print.preset.id,
                                    i
                                );
                            }
                        }
                    }
                } else {
                    if (material_selected) {
                        for (size_t i = 0, n = print.materials.size(); i < n; ++i) {
                            for (const auto& mat : print.materials.at(i)) {
                                if (mat.preset.id == preset_id) {
                                    ret.emplace_back(
                                        printer.hw_config.id,
                                        printer.preset.id,
                                        print.preset.id,
                                        i
                                    );
                                }
                            }
                        }
                    }
                }
            }
        }
    });
    return ret;
}

namespace {

template <typename T>
Bundle::HwConfigToolSlots get_preset_used_slots(
    const EvaluatedPrinterPresets& evaluated_presets,
    const std::string& preset_id,
    const std::function<const std::vector<std::vector<T>>&(const EvaluatedPrintPreset&)>& get_preset
)
{
    Bundle::HwConfigToolSlots ret;

    for (const auto& [hw_config_id, printers] : evaluated_presets) {
        for (const auto& printer : printers) {
            for (const auto& print : printer.prints) {

                const auto& all_tools_presets = get_preset(print);
                std::vector<size_t> printer_slots;

                for (size_t i = 0; i < all_tools_presets.size(); ++i) {
                    if (std::ranges::any_of(
                            all_tools_presets.at(i),
                            [&](const auto& p) -> bool { return p.preset.id == preset_id; }
                        ))
                    {
                        printer_slots.push_back(i);
                    }
                }

                if (!printer_slots.empty()) {
                    ret.emplace(
                        Bundle::HwConfigToolKey{hw_config_id, printer.preset.id, print.preset.id},
                        std::move(printer_slots)
                    );
                }

            }
        }
    }

    return ret;
}

template <typename T>
Bundle::UsedSlots get_preset_used_slots(
    const EvaluatedPrinterPresets& evaluated_presets,
    const std::string& hw_config_id,
    const std::string& printer_id,
    const std::string& print_id,
    const std::function<const std::vector<std::vector<T>>&(const EvaluatedPrintPreset&)>& get_preset
)
{
    Bundle::UsedSlots ret;

    const auto& printers = evaluated_presets.at(hw_config_id);
    auto printer_it = std::ranges::find_if(printers, [&](const auto& p) { return p.preset.id == printer_id; });
    ASSERT(printer_it != printers.end());
    const auto& printer = *printer_it;
    auto print_it = std::ranges::find_if(printer.prints, [&](const auto& p) { return p.preset.id == print_id; });
    ASSERT(print_it != printer.prints.end());
    const auto& print = *print_it;
    auto& all_tools_presets = get_preset(print);
    for (size_t i = 0, n = all_tools_presets.size(); i < n; ++i) {
        const auto& tool_presets = all_tools_presets.at(i);
        if (std::ranges::any_of(tool_presets, [&](const auto& p) { return p.preset.id == print_id; })) {
            ret.push_back(i);
        }
    }

    return ret;
}

const AllToolsEvaluatedToolPrintPresets& get_tool_prints(const EvaluatedPrintPreset& p)
{
    return p.tools;
}
const AllToolsEvaluatedMaterialPresets& get_materials(const EvaluatedPrintPreset& p)
{
    return p.materials;
}


} // namespace

Bundle::HwConfigToolSlots Bundle::get_tool_print_preset_used_slots(
    const std::string& preset_id
) const
{
    return get_preset_used_slots<EvaluatedToolPrintPreset>(
        evaluated_presets,
        preset_id,
        get_tool_prints
    );
}

Bundle::HwConfigToolSlots Bundle::get_material_preset_used_slots(
    const std::string& preset_id
) const
{
    return get_preset_used_slots<EvaluatedMaterialPreset>(
        evaluated_presets,
        preset_id,
        get_materials
    );
}

Bundle::UsedSlots Bundle::get_tool_print_preset_used_slots(
    const std::string& hw_config_id,
    const std::string& printer_id,
    const std::string& print_id
) const
{
    return get_preset_used_slots<EvaluatedToolPrintPreset>(
        evaluated_presets,
        hw_config_id,
        printer_id,
        print_id,
        get_tool_prints
    );
}

Bundle::UsedSlots Bundle::get_material_preset_used_slots(
    const std::string& hw_config_id,
    const std::string& printer_id,
    const std::string& print_id
) const
{
    return get_preset_used_slots<EvaluatedMaterialPreset>(
        evaluated_presets,
        hw_config_id,
        printer_id,
        print_id,
        get_materials
    );
}

} // namespace Slic3r::Domain::Preset
