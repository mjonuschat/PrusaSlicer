#include "Slic3r/Domain/ConfigPack.hpp"

namespace Slic3r::Domain {

using ConfigBoxRefs      = std::vector<std::reference_wrapper<ConfigBox>>;
using ConstConfigBoxRefs = std::vector<std::reference_wrapper<const ConfigBox>>;

ConfigPackFDM::ConfigPackFDM(const int extruder_count) :
    tool{std::vector<Domain::ToolPrintSettings>(extruder_count)},
    filament{std::vector<Domain::FilamentSettings>(extruder_count)}
{
    printer.items.opt("extruder_offset").set(std::vector<Vec2d>(extruder_count, Vec2d::Zero()));
}

ConfigPackFDM::ConfigPackFDM() : ConfigPackFDM{1} {}

ContainsResult ConfigPackFDM::contains(const std::string& key, size_t slot)
{
    ASSERT(slot < tool.size());
    ASSERT(slot < filament.size());
    for (auto& box : ConfigBoxRefs{filament.at(slot), tool.at(slot), print, printer}) {
        if (auto result = box.get().contains(key); result.item)
            return result;
    }
    return {};
}

ConstContainsResult ConfigPackFDM::contains(const std::string& key, size_t slot) const
{
    ASSERT(slot < tool.size());
    ASSERT(slot < filament.size());
    for (const auto& box : ConstConfigBoxRefs{filament.at(slot), tool.at(slot), print, printer}) {
        if (const auto result = box.get().contains(key); result.item)
            return result;
    }
    return {};
}

ContainsResult ConfigPackSLA::contains(const std::string& key)
{
    for (auto& box : ConfigBoxRefs{sla_material_settings, sla_print_settings, sla_printer_settings})
    {
        if (const auto result = box.get().contains(key); result.item)
            return result;
    }
    return {};
}

ConstContainsResult ConfigPackSLA::contains(const std::string& key) const
{
    for (const auto& box :
         ConstConfigBoxRefs{sla_material_settings, sla_print_settings, sla_printer_settings})
    {
        if (const auto result = box.get().contains(key); result.item)
            return result;
    }
    return {};
}

} // namespace Slic3r::Domain
