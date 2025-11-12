#include "libslic3r/ShrinkageCompensation.hpp"
#include "libslic3r/ExtruderCandidates.hpp"

namespace Slic3r::Biz::Slicing {

template <typename T>
static std::vector<T>
get_filament_option_vector(const Domain::ConfigPackFDM& config, const std::string& key)
{
    std::vector<T> result;
    result.reserve(config.filament.size());

    for (const auto& filament : config.filament) {
        const auto value{filament.items.opt(key).get<T>()};
        result.push_back(value);
    }
    return result;
}

static bool has_same_shrinkage_compensations(
    const std::vector<unsigned int> extruders,
    const Domain::ConfigPackFDM& config
)
{
    if (extruders.empty())
        return false;

    const std::vector<Domain::Percentage> compensation_xy{
        get_filament_option_vector<Domain::Percentage>(config, "filament_shrinkage_compensation_xy")
    };

    const std::vector<Domain::Percentage> compensation_z{
        get_filament_option_vector<Domain::Percentage>(config, "filament_shrinkage_compensation_z")
    };

    for (unsigned int extruder : extruders) {
        if (compensation_xy.front() != compensation_xy.at(extruder)
            || compensation_z.front() != compensation_z.at(extruder))
        {
            return false;
        }
    }

    return true;
}

std::optional<Domain::Vec3d>
get_shrinkage_compensation(const Domain::Model& model, const Domain::ConfigPackFDM& config)

{
    const std::vector<unsigned int> extruders{get_extruder_candidates(model, config)};
    if (extruders.empty()) {
        return std::nullopt;
    }
    if (!has_same_shrinkage_compensations(extruders, config)) {
        return std::nullopt;
    }

    const unsigned int first_extruder{extruders.front()};
    const double xy_compensation_percent{std::clamp(
        config.filament.at(first_extruder)
            .items.opt("filament_shrinkage_compensation_xy")
            .get<Domain::Percentage>()
            .value,
        -99.,
        99.
    )};

    const double z_compensation_percent{std::clamp(
        config.filament.at(first_extruder)
            .items.opt("filament_shrinkage_compensation_z")
            .get<Domain::Percentage>()
            .value,
        -99.,
        99.
    )};

    const double xy_compensation = 100. / (100. - xy_compensation_percent);
    const double z_compensation  = 100. / (100. - z_compensation_percent);

    return Domain::Vec3d{xy_compensation, xy_compensation, z_compensation};
}
} // namespace Slic3r::Biz::Slicing
