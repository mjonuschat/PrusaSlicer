#include "Slic3r/Domain/FullConfigFDM.hpp"

namespace Slic3r::Domain {

namespace {
template<typename T>
BoxRefs convert_to_box_refs(
    const std::vector<T>& settings
)
{
    BoxRefs result;
    result.insert(result.end(), settings.cbegin(), settings.cend());
    return result;
}
}

BoxOrBoxesVector as_boxes(const ConfigPackFDM& config_pack) {
    ASSERT(config_pack.filament.size() == config_pack.tool.size());
    BoxOrBoxesVector result;
    result.push_back(config_pack.printer);
    result.push_back(convert_to_box_refs(config_pack.tool));
    result.push_back(config_pack.print);
    result.push_back(convert_to_box_refs(config_pack.filament));
    result.push_back(config_pack.project);
    return result;
}

namespace {
ConfigLocationSizes get_fdm_location_sizes(const std::size_t tools_count, const std::size_t filaments_count) {
    return {
        {FDMConfigLocation::Printer, std::nullopt},
        {FDMConfigLocation::Tool, tools_count},
        {FDMConfigLocation::Print, std::nullopt},
        {FDMConfigLocation::Filament, filaments_count},
        {FDMConfigLocation::Project, std::nullopt},
        {FDMConfigLocation::Object, std::nullopt},
        {FDMConfigLocation::Volume, std::nullopt}
    };
}
}

FullConfigFDM::FullConfigFDM(const ConfigPackFDM& config_pack):
    FullConfig{
        as_boxes(config_pack),
        get_fdm_location_sizes(config_pack.tool.size(), config_pack.filament.size())
    },
    m_tools_count{config_pack.tool.size()},
    m_filaments_count{config_pack.filament.size()}
{}

PartialObjectConfigFDM::PartialObjectConfigFDM(
    const ObjectSettings& object_settings,
    const std::size_t tools_count,
    const std::size_t filaments_count
)
    : PartialConfig{{object_settings}, get_fdm_location_sizes(tools_count, filaments_count)}
{}

PartialVolumeConfigFDM::PartialVolumeConfigFDM(
    const VolumeSettings& object_settings,
    const std::size_t tools_count,
    const std::size_t filaments_count
)
    : PartialConfig{{object_settings}, get_fdm_location_sizes(tools_count, filaments_count)}
{}

} // namespace Slic3r::Domain
