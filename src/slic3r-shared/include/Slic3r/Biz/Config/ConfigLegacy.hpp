#pragma once
#include <string>
#include <variant>

#include "Slic3r/Domain/Config.hpp"
#include "Slic3r/Domain/ConfigFDM.hpp"

namespace Slic3r::Biz {

struct FDMLegacyConfigPack {
	Domain::PrinterSettings printer_settings;
	Domain::PrintSettings print_settings;
	std::vector<Domain::ToolPrintSettings> toolprint_settings;
	std::vector<Domain::FilamentSettings> filament_settings;
};

struct SLALegacyConfigPack {
	// TODO
};

// Loads config from INI / GCODE / BGCODE produced by PrusaSlicer < 3.0.0 and converts
// all matching keys into the provided ConfigBox.
// May throw!
std::variant<FDMLegacyConfigPack, SLALegacyConfigPack> load_config_from_legacy_file(const std::string& filename);

// Export the config in the old format.
std::string serialize_as_legacy_config(const FDMLegacyConfigPack& cfg);


} // namespace Slic3r::Biz
