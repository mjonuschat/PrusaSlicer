#pragma once

#include <string>

namespace Slic3r::Domain {

class ConfigDefinitions;

enum class PrinterTechnology
{
    FFF,
    SLA
};
enum class GCodeThumbnailsFormat {
    PNG, JPG, QOI
};

void init_common_fdm_sla_config_items(ConfigDefinitions& defs, const std::string& technology);
	
}
