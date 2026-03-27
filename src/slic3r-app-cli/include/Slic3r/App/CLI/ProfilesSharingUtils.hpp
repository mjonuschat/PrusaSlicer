#pragma once

#include <optional>
#include <string>
#include <vector>

#include "Slic3r/Domain/ConfigPack.hpp"
#include "Slic3r/Domain/PrinterTechnology.hpp"

namespace Slic3r::App::CLI {

std::string get_json_printer_models(std::optional<Domain::PrinterTechnology> printer_technology);
std::string get_json_print_tool_filament_profiles(const std::string& printer_profile);

// Load full print config into config
// Return value is always error string if any exists
// Note, that all appearing warnings are added into log
// When printer_technology is set, then it will be compared with printer technology of the printer_profile and return the error, when they aren't the same
std::string load_full_print_config(
    const std::string& print_preset_name,
    const std::vector<std::string>& material_preset_names_in,
    const std::vector<std::string>& tool_preset_names_in,
    const std::string& printer_preset_name,
    Domain::ConfigPack& config,
    std::optional<Domain::PrinterTechnology> printer_technology = std::nullopt
);

} // namespace Slic3r::App::CLI
