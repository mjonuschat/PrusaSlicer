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
    HwPrinterConfigs printer_configs;

    template<class Archive> void serialize(Archive& archive)
    {
        archive(vendor_data, presets, printer_configs);
    }

    const HwPrinterConfig* find_printer_config(const std::string& id) const;
    HwPrinterConfig* find_printer_config(const std::string& id);
};

using VendorBundles = std::map<std::string, VendorBundle>;
using PrinterConfigs = std::map<std::string, HwPrinterConfig>;
using EvaluatedPrinterPresets = std::map<std::string, std::vector<EvaluatedPrinterPreset>>;

/**
 * @brief Preset bundle contains all vendor specific preset bundles, printer configurations and
 * evaluated presets with respect to the configurations.
 */
struct Bundle
{
    VendorBundles vendor_bundles;
    // TODO: user presets
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
};

}
