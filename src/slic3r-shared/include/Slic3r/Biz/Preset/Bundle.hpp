#pragma once

#include "Slic3r/Domain/Preset/HwConfig.hpp"
#include "Slic3r/Domain/Preset/PresetTree.hpp"
#include "Slic3r/Domain/Preset/EvaluatedPreset.hpp"

namespace Slic3r::Biz::Preset {

/**
 * @brief Vendor specific preset bundle and hw config definitions.
 */
class VendorBundle
{
public:
    void load_presets();
    void dispose_presets();

    const std::string& id() const { return m_vendor_data.info.id; }
    const Domain::Preset::VendorData& vendor_data() const { return m_vendor_data; }
    const Domain::Preset::PresetCollection& presets() const { return m_presets; }

private:
    Domain::Preset::VendorData m_vendor_data;;
    Domain::Preset::PresetCollection m_presets;
};

using VendorBundles = std::map<std::string, VendorBundle>;
using PrinterConfigs = std::map<std::string, Domain::Preset::HwPrinterConfig>;
using EvaluatedPrinterPresets = std::map<std::string, Domain::Preset::EvaluatedPrinterPreset>;

/**
 * @brief Preset bundle contains all vendor specific preset bundles, printer configurations and
 * evaluated presets with respect to the configurations.
 */
class Bundle
{
public:
    void load_bundles(const std::string& base_dir);

    const VendorBundles& vendor_bundles() const { return m_vendor_bundles; }
    const PrinterConfigs& printer_configs() const { return m_printer_configs; }
    const EvaluatedPrinterPresets& evaluated_presets() const { return m_evaluated_presets; }
private:
    VendorBundles m_vendor_bundles;
    // TODO: user presets
    PrinterConfigs m_printer_configs;
    EvaluatedPrinterPresets m_evaluated_presets;
};

}
