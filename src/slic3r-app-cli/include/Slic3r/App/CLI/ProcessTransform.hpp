#pragma once

#include "Slic3r/Domain/Point.hpp"

#include <vector>
#include <variant>

namespace Slic3r::Domain {
struct ConfigPackFDM;
struct ConfigPackSLA;
class Project;

using ConfigPack = std::variant<ConfigPackFDM, ConfigPackSLA>;
} // namespace Slic3r::Domain

namespace Slic3r::App {
class InitParams;
} // namespace Slic3r::App

namespace Slic3r::App::CLI {

Domain::Points get_bed_shape(const Domain::ConfigPack& config_pack);

double min_object_distance(const Domain::ConfigPack& config_pack);

bool process_transform(
    const InitParams& init_params,
    const Domain::ConfigPack& config_pack,
    std::vector<Domain::Project>& projects
);

} // namespace Slic3r::App::CLI
