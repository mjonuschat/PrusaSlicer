#include "Slic3r/Domain/ConfigPack.hpp"
#include "Slic3r/Domain/FullConfigFDM.hpp"

namespace Slic3r::Domain {

using ConfigBoxRefs      = std::vector<std::reference_wrapper<ConfigBox>>;
using ConstConfigBoxRefs = std::vector<std::reference_wrapper<const ConfigBox>>;

static void ensure_tool_parity(Domain::MutBoxRef& box, int extruder_count)
{
    for (ConfigItem& item : box.get().items.all_items()) {
        item.visit(
            [&](auto& value)
            {
                if (!item.def().require_tool_parity) {
                    return;
                }
                using Type = std::remove_cvref_t<decltype(value)>;
                if constexpr (Domain::is_std_vector_v<Type>) {
                    value.resize(extruder_count);
                }
            }
        );
    }
}

ConfigPackFDM::ConfigPackFDM(const int extruder_count) :
    tool{std::vector<Domain::ToolPrintSettings>(extruder_count)},
    filament{std::vector<Domain::FilamentSettings>(extruder_count)}
{
    for (auto& box_or_boxes : as_mut_boxes(*this)) {
        std::visit(
            Domain::overloaded{
                [&](Domain::MutBoxRef& box) { ensure_tool_parity(box, extruder_count); },
                [&](Domain::MutBoxRefs& boxes)
                {
                    for (Domain::MutBoxRef& box : boxes) {
                        ensure_tool_parity(box, extruder_count);
                    }
                }
            },
            box_or_boxes
        );
    }
}

ConfigPackFDM::ConfigPackFDM() : ConfigPackFDM{1} {}

FindResult ConfigPackFDM::contains(const std::string& key, size_t slot)
{
    ASSERT(slot < tool.size());
    ASSERT(slot < filament.size());
    for (auto& box : ConfigBoxRefs{filament.at(slot), tool.at(slot), print, printer}) {
        if (auto result = box.get().find(key); result.item)
            return result;
    }
    return {};
}

ConstFindResult ConfigPackFDM::contains(const std::string& key, size_t slot) const
{
    ASSERT(slot < tool.size());
    ASSERT(slot < filament.size());
    for (const auto& box : ConstConfigBoxRefs{filament.at(slot), tool.at(slot), print, printer}) {
        if (const auto result = box.get().find(key); result.item)
            return result;
    }
    return {};
}

FindResult ConfigPackSLA::contains(const std::string& key)
{
    for (auto& box : ConfigBoxRefs{sla_material_settings, sla_print_settings, sla_printer_settings})
    {
        if (const auto result = box.get().find(key); result.item)
            return result;
    }
    return {};
}

ConstFindResult ConfigPackSLA::contains(const std::string& key) const
{
    for (const auto& box :
         ConstConfigBoxRefs{sla_material_settings, sla_print_settings, sla_printer_settings})
    {
        if (const auto result = box.get().find(key); result.item)
            return result;
    }
    return {};
}

} // namespace Slic3r::Domain
