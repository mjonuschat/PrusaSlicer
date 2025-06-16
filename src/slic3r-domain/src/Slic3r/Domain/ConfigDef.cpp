#include "Slic3r/Domain/ConfigDef.hpp"

namespace Slic3r::Domain {

std::string get_location_name(const ConfigLocation& location) {
    return std::visit(overloaded{
        [](const FDMConfigLocation location) {
            switch(location) {
                case FDMConfigLocation::Printer: return "printer_settings";
                case FDMConfigLocation::Tool: return "toolprint_settings";
                case FDMConfigLocation::Print: return "print_settings";
                case FDMConfigLocation::Filament: return "filament_settings";
                case FDMConfigLocation::Project: return "project_settings";
                case FDMConfigLocation::Object: return "object_settings";
                case FDMConfigLocation::Volume: return "volume_settings";
                default: PANIC("Unknown location");
            }
        },
        [](const SLAConfigLocation location) {
            switch(location) {
                case SLAConfigLocation::Printer: return "sla_printer_settings";
                case SLAConfigLocation::Print: return "sla_print_settings";
                case SLAConfigLocation::Material: return "sla_material_settings";
                case SLAConfigLocation::Object: return "sla_object_settings";
                default: PANIC("Unknown location");
            }
        },
        [](const PhysicalPrinterLocation location) {
            return "physical_printer_settings";
        },
    }, location);
}

ConfigDefinitions::ConfigDefinitions(
    const std::set<ConfigLocation>& acceptable_boxes,
    std::function<void(ConfigDefinitions&)> init_fn
): m_acceptable_boxes{acceptable_boxes}
{
    init_fn(*this);
    std::sort(m_defs.begin(), m_defs.end());
    this->check_valid();
    m_finalized = true;
}

ConfigItemDef* ConfigDefinitions::add(const std::string_view name, const std::type_info& type)
{
    ASSERT(!m_finalized);
    return &m_defs.emplace_back(ConfigItemDef{std::string(name), &type});
}

void ConfigDefinitions::check_valid() const
{
    ASSERT(std::is_sorted(m_defs.begin(), m_defs.end()));
    ASSERT(
        std::adjacent_find(
            m_defs.begin(), m_defs.end(), // check for duplicates
            [](const auto& a, const auto& b) { return a.name == b.name; }
        ) == m_defs.end()
    );

    for (const ConfigItemDef& def : m_defs) {
        ASSERT(def.type != nullptr);

        std::visit(overloaded{
            [](const FDMConfigLocation location) {ASSERT(location != FDMConfigLocation::None);},
            [](const SLAConfigLocation location) {ASSERT(location != SLAConfigLocation::None);},
            [](const PhysicalPrinterLocation location) {}
        }, def.location);

        ASSERT(!def.overrides_in.contains(def.location));

        // Check that all items are assigned to valid boxes.
        ASSERT(m_acceptable_boxes.contains(def.location));

        ASSERT(std::all_of(def.overrides_in.begin(), def.overrides_in.end(), [this](const auto& box) {
            return m_acceptable_boxes.contains(box);
        }));

        if (def.init_fn) {
            const ConfigValue value{def.init_fn()};
            value.visit([&](auto&& value) { ASSERT(typeid(decltype(value)) == *def.type); });
        } else if (def.init_fn_ex) {
            const ConfigValue value{def.init_fn_ex(def.location)};
            value.visit([&](auto&& value) { ASSERT(typeid(decltype(value)) == *def.type); });

            for (const ConfigLocation& override_location : def.overrides_in) {
                const ConfigValue value{def.init_fn_ex(override_location)};
                value.visit([&](auto&& value) { ASSERT(typeid(decltype(value)) == *def.type); });
            }
        } else {
            PANIC("init_fn or init_fn_ex must be defined");
        }

        // Check that all choices (if used) have the same key type and that it matches the item type.
        if (!def.choices.empty()) {
            for (const auto& [value, str] : def.choices) {
                ASSERT(
                    (*def.type == typeid(std::string) && std::holds_alternative<std::string>(value)
                    ) ||
                    (*def.type == typeid(int) && std::holds_alternative<int>(value)) ||
                    (*def.type == typeid(double) && std::holds_alternative<double>(value)) ||
                    (*def.type == typeid(Percentage) && std::holds_alternative<double>(value)) ||
                    (*def.type == typeid(FloatOrPercentage) && std::holds_alternative<double>(value))
                );
            }
        }
    }
}
} // namespace Slic3r::Domain
