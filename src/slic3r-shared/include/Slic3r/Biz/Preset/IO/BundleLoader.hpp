#pragma once
#include "Slic3r/Domain/Preset/Bundle.hpp"

namespace Slic3r::Biz::Preset::IO {

Domain::Preset::Bundle load_bundle(const std::string& bundle_path, const std::string& config_path);
void save_bundle_configs(const Domain::Preset::Bundle& bundle, const std::string& config_path);

void serialize_bundle(
    const std::string& filename,
    const Domain::Preset::Bundle& bundle,
    const std::string& preset_bundle_path,
    const std::string& config_path,
    const std::string& slicer_version);
std::optional<Domain::Preset::Bundle> deserialize_bundle(
    const std::string& filename,
    const std::string& preset_bundle_path,
    const std::string& config_path,
    const std::string& slicer_version);

}
