#pragma once
#include "Slic3r/Domain/Preset/Bundle.hpp"

namespace Slic3r::Biz::Preset::IO {

Domain::Preset::Bundle load_bundle(const std::string& bundle_path, const std::string& config_path);
void save_bundle_configs(const Domain::Preset::Bundle& bundle, const std::string& config_path);

}
