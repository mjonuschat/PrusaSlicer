#pragma once

#include "Slic3r/Domain/ConfigBoxesFDM.hpp"
#include "Slic3r/Domain/ConfigBoxesSLA.hpp"

namespace Slic3r::Domain {

struct ConfigPackFDM
{
    explicit ConfigPackFDM(const int extruder_count)
        : tool{std::vector<Domain::ToolPrintSettings>(extruder_count)}
        , filament{std::vector<Domain::FilamentSettings>(extruder_count)}
    {}
    ConfigPackFDM() : ConfigPackFDM{1} {}

    Domain::PrinterSettings printer;
    Domain::PrintSettings print;
    std::vector<Domain::ToolPrintSettings> tool;
    std::vector<Domain::FilamentSettings> filament;
    Domain::ProjectSettings project;
};

struct ConfigPackSLA
{
    Domain::SLAPrinterSettings sla_printer_settings;
    Domain::SLAMaterialSettings sla_material_settings;
    Domain::SLAPrintSettings sla_print_settings;
};

using ConfigPack = std::variant<ConfigPackFDM, ConfigPackSLA>;
} // namespace Slic3r::Domain
