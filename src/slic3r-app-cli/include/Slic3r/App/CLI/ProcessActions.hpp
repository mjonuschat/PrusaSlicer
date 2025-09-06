#pragma once

#include <vector>
#include <variant>

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

bool has_full_config_from_profiles(const InitParams& init_params);

bool process_actions(
    const InitParams& init_params,
    const Domain::ConfigPack& config_pack,
    std::vector<Domain::Model>& models
);

bool process_profiles_sharing(const InitParams& init_params);

} // namespace Slic3r::App::CLI
