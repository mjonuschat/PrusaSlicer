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

    const EvaluatedPrinterPreset* find_printer_preset_by_id(const std::string& id) const;
};

}
