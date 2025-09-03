#pragma once

#include <string>

namespace Slic3r::Domain {
class Bed;
class BedContainer;

namespace Preset {
struct SelectedPreset;
} // namespace Preset
} // namespace Slic3r::Domain

namespace Slic3r::Biz::Scene {

Domain::Bed&
get_or_create_bed(Domain::BedContainer& bed_container, const Domain::Preset::SelectedPreset& preset, const std::string& assets_path);

} // namespace Slic3r::Biz::Scene
