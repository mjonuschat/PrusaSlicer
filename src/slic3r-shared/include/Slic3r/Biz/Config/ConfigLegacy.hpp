#pragma once
#include <string>

#include "Slic3r/Domain/Config.hpp"
#include "Slic3r/Domain/ConfigPack.hpp"

namespace Slic3rLegacy { class DynamicPrintConfig; }

namespace Slic3r::Biz {

// Loads config from INI / GCODE / BGCODE produced by PrusaSlicer < 3.0.0 and converts
// all matching keys into the provided ConfigBox.
// May throw!
Domain::ConfigPack load_config_from_legacy_file(const std::string& filename);

Domain::ConfigPack convert_dynamic_print_config_to_new(Slic3rLegacy::DynamicPrintConfig& cfg);
void fill_config_box_from_legacy(const Slic3rLegacy::DynamicPrintConfig& cfg, Domain::ConfigBox& box);

// The following only works for volume and object settings.
Slic3rLegacy::DynamicPrintConfig convert_box_to_dynamic_print_config(const Domain::ConfigBox& box);

// Export the config in the old format.
std::string serialize_as_legacy_config(const Domain::ConfigPackFDM&, bool prepend_semicolons = false);
std::string serialize_as_legacy_config(const Domain::ConfigPackSLA&, bool prepend_semicolons = false);

} // namespace Slic3r::Biz
