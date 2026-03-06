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

template<typename T>
MutBoxRefs convert_to_mut_box_refs(
    std::vector<T>& settings
)
{
    MutBoxRefs result;
    result.insert(result.end(), settings.begin(), settings.end());
    return result;
}
}

BoxOrBoxesVector as_boxes(const ConfigPackFDM& config_pack) {
    ASSERT(config_pack.filament.size() == config_pack.tool.size() || config_pack.tool.size() == 1);
    BoxOrBoxesVector result;
    result.push_back(config_pack.print);
    result.push_back(convert_to_box_refs(config_pack.tool));
    result.push_back(config_pack.printer);
    result.push_back(convert_to_box_refs(config_pack.filament));
    result.push_back(config_pack.project);
    return result;
}

MutBoxOrBoxesVector as_mut_boxes(ConfigPackFDM& config_pack) {
    ASSERT(config_pack.filament.size() == config_pack.tool.size() || config_pack.tool.size() == 1);
    MutBoxOrBoxesVector result;
    result.push_back(config_pack.print);
    result.push_back(convert_to_mut_box_refs(config_pack.tool));
    result.push_back(config_pack.printer);
    result.push_back(convert_to_mut_box_refs(config_pack.filament));
    result.push_back(config_pack.project);
    return result;
}

namespace {
ConfigLocationSizes get_fdm_location_sizes(const std::size_t material_slot_count) {
    return {
        {FDMConfigLocation::Print, std::nullopt},
        {FDMConfigLocation::Tool, material_slot_count},
        {FDMConfigLocation::Printer, std::nullopt},
        {FDMConfigLocation::Filament, material_slot_count},
        {FDMConfigLocation::Project, std::nullopt},
        {FDMConfigLocation::Object, std::nullopt},
        {FDMConfigLocation::Volume, std::nullopt}
    };
}
}

FullConfigFDM::FullConfigFDM(
    const ConfigPackFDM& config_pack,
    const std::vector<unsigned>& extruder_candidates,
    const Preset::HwPrinterConfig& hw_config
) :
    FullConfig{
        as_boxes(config_pack),
        extruder_candidates,
        get_fdm_location_sizes(hw_config.material_slot_count())
    },
    m_hw_config{hw_config}
{}

PartialObjectConfigFDM::PartialObjectConfigFDM(
    const ObjectSettings& object_settings,
    const std::size_t material_slot_count
) :
    PartialConfig{
        {object_settings},
        get_fdm_location_sizes(material_slot_count)
    }
{}

PartialVolumeConfigFDM::PartialVolumeConfigFDM(
    const VolumeSettings& volume_settings,
    const std::size_t material_slot_count
) :
    PartialConfig{
        {volume_settings},
        get_fdm_location_sizes(material_slot_count)
    }
{}

} // namespace Slic3r::Domain
