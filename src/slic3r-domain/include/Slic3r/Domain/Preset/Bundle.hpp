#pragma once

#include "Slic3r/Domain/Preset/HwConfig.hpp"
#include "Slic3r/Domain/Preset/PresetTree.hpp"
#include "Slic3r/Domain/Preset/EvaluatedPreset.hpp"

namespace Slic3r::Domain::Preset {

/**
 * @brief Vendor specific preset bundle and hw config definitions.
 */
struct VendorBundle
{
    VendorData vendor_data;
    PresetCollection presets;
    PresetNamesCollection preset_names;
    HwPrinterConfigs printer_configs;

    template<class Archive> void serialize(Archive& archive)
    {
        archive(vendor_data, presets, preset_names, printer_configs);
    }

    const HwPrinterConfig* find_printer_config(const std::string& id) const;
    HwPrinterConfig* find_printer_config(const std::string& id);
};

using VendorBundles = std::map<std::string, VendorBundle>;
using PrinterConfigs = std::map<std::string, HwPrinterConfig>;
using EvaluatedPrinterPresets = std::map<std::string, std::vector<EvaluatedPrinterPreset>>;

struct PresetParentPath
{
    using OptString = std::optional<std::string>;
    using OptSlot = std::optional<size_t>;

    std::string hw_config_id;
    OptString printer_id;
    OptString print_id;
    OptSlot slot;
};

using PresetParentPaths = std::vector<PresetParentPath>;

/**
 * @brief Preset bundle contains all vendor specific preset bundles, printer configurations and
 * evaluated presets with respect to the configurations.
 */
struct Bundle
{
    VendorBundles vendor_bundles;
    PrinterConfigs printer_configs;
    EvaluatedPrinterPresets evaluated_presets;

    template<class Archive> void serialize(Archive& archive)
    {
        archive(vendor_bundles, printer_configs, evaluated_presets);
    }

    const EvaluatedPrinterPreset* find_printer_preset(
        const std::string& printer_hw_config_id,
        const std::string& printer_preset_id
    ) const;

    const HwPrinterConfig* find_config_with_same_values(const HwPrinterConfig& printer_config) const;
    const EvaluatedPrinterPreset* find_printer_preset_with_same_values(const std::string& hw_config_id, const EvaluatedPrinterPreset::Preset& printer_preset) const;

    /**
     * @brief Updates all evaluated printer preset with given @a preset if its IDs match.
     * @param preset preset to update stored evaluated presets with.
     */
    void update_presets(const EvaluatedPrinterPreset::Preset& preset);

    /**
     * @brief Updates all evaluated print preset with given @a preset if its IDs match.
     * @param preset preset to update stored evaluated presets with.
     */
    void update_presets(const EvaluatedPrintPreset::Preset& preset);

    /**
     * @brief Updates all evaluated tool-print preset with given @a preset if its IDs match.
     * @param preset preset to update stored evaluated presets with.
     */
    void update_presets(const EvaluatedToolPrintPreset::Preset& preset);

    /**
     * @brief Updates all evaluated material preset with given @a preset if its IDs match.
     * @param preset preset to update stored evaluated presets with.
     */
    void update_presets(const EvaluatedMaterialPreset::Preset& preset);

    void copy_preset(
        const EvaluatedPrinterPreset::Preset& preset,
        const std::string& printer_id
    );

    void copy_preset(
        const EvaluatedPrintPreset::Preset& preset,
        const std::string& print_id
    );

    void copy_preset(
        const EvaluatedToolPrintPreset::Preset& preset,
        const std::string& tool_print_id
    );

    void copy_preset(
        const EvaluatedMaterialPreset::Preset& preset,
        const std::string& material_id
    );

    using HwConfigToolKey = std::tuple<
        // config id
        std::string,
        // printer id
        std::string,
        // print id
        std::string
    >;
    using UsedSlots = std::vector<size_t>;
    using HwConfigToolSlots = std::map<HwConfigToolKey, UsedSlots>;

    PresetParentPaths find_usage_of_preset(PresetKind kind, const std::string& preset_id) const;

    HwConfigToolSlots get_tool_print_preset_used_slots(
        const std::string& preset_id
    ) const;

    HwConfigToolSlots get_material_preset_used_slots(
        const std::string& preset_id
    ) const;

    UsedSlots get_tool_print_preset_used_slots(
        const std::string& hw_config_id,
        const std::string& printer_id,
        const std::string& print_id
    ) const;

    UsedSlots get_material_preset_used_slots(
        const std::string& hw_config_id,
        const std::string& printer_id,
        const std::string& print_id
    ) const;
};

}
