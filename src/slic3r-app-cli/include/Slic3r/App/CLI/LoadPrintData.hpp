#pragma once

#include "Slic3r/Domain/PrinterTechnology.hpp"

#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace Slic3r::Domain {
struct ConfigPackFDM;
struct ConfigPackSLA;
class Model;

using ConfigPack = std::variant<ConfigPackFDM, ConfigPackSLA>;
} // namespace Slic3r::Domain

namespace Slic3r::App {
class InitParams;
} // namespace Slic3r::App

namespace Slic3r::App::CLI {

std::optional<Domain::PrinterTechnology> get_printer_technology(const InitParams& init_params);

Domain::PrinterTechnology get_printer_technology(const Domain::ConfigPack& config_pack);

bool load_print_data(
    std::vector<Domain::Model>& models,
    Domain::ConfigPack& config_pack,
    std::optional<Domain::PrinterTechnology>& printer_technology,
    InitParams& init_params
);

bool is_needed_post_processing(const Domain::ConfigPack& config_pack);

} // namespace Slic3r::App::CLI
