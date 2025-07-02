#pragma once

#include "Slic3r/Domain/ConfigBoxesFDM.hpp"
#include "Slic3r/Domain/ConfigBoxesSLA.hpp"

namespace Slic3r::Domain {

struct ConfigPackFDM
{
    explicit ConfigPackFDM(const int extruder_count);
    ConfigPackFDM();

    Domain::PrinterSettings printer;
    Domain::PrintSettings print;
    std::vector<Domain::ToolPrintSettings> tool;
    std::vector<Domain::FilamentSettings> filament;
    Domain::ProjectSettings project;

    bool operator==(const ConfigPackFDM&) const = default;
};

struct ConfigPackSLA
{
    Domain::SLAPrinterSettings sla_printer_settings;
    Domain::SLAMaterialSettings sla_material_settings;
    Domain::SLAPrintSettings sla_print_settings;

    bool operator==(const ConfigPackSLA&) const = default;
};

using ConfigPack = std::variant<ConfigPackFDM, ConfigPackSLA>;
} // namespace Slic3r::Domain
