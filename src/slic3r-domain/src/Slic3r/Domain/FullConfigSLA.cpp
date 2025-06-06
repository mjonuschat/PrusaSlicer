#include "Slic3r/Domain/FullConfigSLA.hpp"

namespace Slic3r::Domain {

namespace {
BoxOtBoxesVector convert_to_full_config_input(const ConfigPackSLA& config_pack) {
    BoxOtBoxesVector result;
    result.push_back(config_pack.sla_printer_settings);
    result.push_back(config_pack.sla_print_settings);
    result.push_back(config_pack.sla_material_settings);
    return result;
}

ConfigLocationSizes get_sla_location_sizes() {
    return {
        {SLAConfigLocation::Printer, std::nullopt},
        {SLAConfigLocation::Print, std::nullopt},
        {SLAConfigLocation::Material, std::nullopt},
        {SLAConfigLocation::Object, std::nullopt},
    };
}
}

FullConfigSLA::FullConfigSLA(const ConfigPackSLA& config_pack)
    : FullConfig{convert_to_full_config_input(config_pack), get_sla_location_sizes()}
{}

PartialObjectConfigSLA::PartialObjectConfigSLA(
    const SLAObjectSettings& object_settings
)
    : PartialConfig{{object_settings}, get_sla_location_sizes()}
{}
} // namespace Slic3r::Domain
