#pragma once

#include <vector>

#include "PresetState.hpp"

namespace Slic3r::Biz::Preset {

struct PresetInteractorConfigContainerContext
{
    size_t config_container_id;
    PresetState printer;
    PresetState print;
    std::vector<PresetState> materials;
    std::vector<PresetState> extruders;

    PrinterTechnology   printer_technology() { return printer.edited_preset.printer_technology(); }

    DynamicPrintConfig  full_config() const;

private:
    DynamicPrintConfig  full_fff_config() const;
    DynamicPrintConfig  full_sla_config() const;

};

}
