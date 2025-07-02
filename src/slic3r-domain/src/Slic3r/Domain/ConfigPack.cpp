#include "Slic3r/Domain/ConfigPack.hpp"

namespace Slic3r::Domain {

ConfigPackFDM::ConfigPackFDM(const int extruder_count):
    tool{std::vector<Domain::ToolPrintSettings>(extruder_count)},
    filament{std::vector<Domain::FilamentSettings>(extruder_count)}
{
    printer.items.opt("extruder_offset").set(std::vector<Vec2d>(extruder_count, Vec2d::Zero()));
}

ConfigPackFDM::ConfigPackFDM(): ConfigPackFDM{1} {}

} // namespace Slic3r::Domain
