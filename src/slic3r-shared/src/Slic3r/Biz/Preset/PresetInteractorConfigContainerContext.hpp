#pragma once

#include <vector>

#include "PresetState.hpp"
#include "PresetBundleRuntime.hpp"

namespace Slic3r::Biz::Preset {

struct PresetInteractorConfigContainerContext
{
    size_t                      config_container_id;
    PresetState                 printer;
    PresetState                 print;
    std::vector<PresetState>    materials;
    std::vector<PresetState>    extruders;
    PresetBundleRuntime         preset_bundle_runtime;
    std::string                 ph_printer_name                 { std::string() };

    PrinterTechnology   printer_technology() const { return printer.edited_preset.printer_technology(); }

    DynamicPrintConfig  full_config() const;

private:
    DynamicPrintConfig  full_fff_config() const;
    DynamicPrintConfig  full_sla_config() const;

};

}
