#pragma once
#include <string>
#include <variant>

#include "Slic3r/Domain/Config.hpp"
#include "Slic3r/Domain/ConfigFDM.hpp"
#include "Slic3r/Domain/ConfigSLA.hpp"

namespace Slic3rLegacy { class DynamicPrintConfig; }

namespace Slic3r::Biz {

struct FDMLegacyConfigPack {
	Domain::PrinterSettings printer_settings;
	Domain::PrintSettings print_settings;
	std::vector<Domain::ToolPrintSettings> toolprint_settings;
	std::vector<Domain::FilamentSettings> filament_settings;
};

struct SLALegacyConfigPack {
	Domain::SLAPrinterSettings sla_printer_settings;
	Domain::SLAMaterialSettings sla_material_settings;
	Domain::SLAPrintSettings sla_print_settings;
};

// Loads config from INI / GCODE / BGCODE produced by PrusaSlicer < 3.0.0 and converts
// all matching keys into the provided ConfigBox.
// May throw!
std::variant<FDMLegacyConfigPack, SLALegacyConfigPack> load_config_from_legacy_file(const std::string& filename);

std::variant<FDMLegacyConfigPack, SLALegacyConfigPack> convert_dynamic_print_config_to_new(Slic3rLegacy::DynamicPrintConfig& cfg);
void fill_config_box_from_legacy(const Slic3rLegacy::DynamicPrintConfig& cfg, Domain::ConfigBox& box);

// Export the config in the old format.
std::string serialize_as_legacy_config(const FDMLegacyConfigPack&, bool prepend_semicolons = false);
std::string serialize_as_legacy_config(const SLALegacyConfigPack&, bool prepend_semicolons = false);

} // namespace Slic3r::Biz
