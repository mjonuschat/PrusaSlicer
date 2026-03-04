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

    FindResult contains(const std::string& key, size_t slot);
    ConstFindResult contains(const std::string& key, size_t slot) const;
    void resize_tool_parity_items(int extruder_count, bool ensure_down_size_only);
};

struct ConfigPackSLA
{
    Domain::SLAPrinterSettings sla_printer_settings;
    Domain::SLAMaterialSettings sla_material_settings;
    Domain::SLAPrintSettings sla_print_settings;

    bool operator==(const ConfigPackSLA&) const = default;

    FindResult contains(const std::string& key);
    ConstFindResult contains(const std::string& key) const;
};

using ConfigPack = std::variant<ConfigPackFDM, ConfigPackSLA>;
} // namespace Slic3r::Domain
