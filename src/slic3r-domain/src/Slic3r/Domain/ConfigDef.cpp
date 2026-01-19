#include "Slic3r/Domain/ConfigDef.hpp"

static const std::string& L(const std::string& s) { return s; }

namespace Slic3r::Domain {

std::string ConfigItemDef::translate_category(Category category, const PrinterTechnology pt)
{
    switch (category) {
    case Domain::ConfigItemDef::Category::General:
        return L("General");
    case Domain::ConfigItemDef::Category::Advanced:
        return L("Advanced");
    case Domain::ConfigItemDef::Category::Cooling:
        return L("Cooling");
    case Domain::ConfigItemDef::Category::CustomGcode:
        return L("Custom Gcode");
    case Domain::ConfigItemDef::Category::Extruders:
        return L("Extruders");
    case Domain::ConfigItemDef::Category::Material: {
        if (pt == PrinterTechnology::FFF)
            return L("Filament");
        return L("Material");
    }
    case Domain::ConfigItemDef::Category::Hollowing:
        return L("Hollowing");
    case Domain::ConfigItemDef::Category::Infill:
        return L("Infill");
    case Domain::ConfigItemDef::Category::LayersAndPerimeters:
        return L("Layers and perimeters");
    case Domain::ConfigItemDef::Category::MachineLimits:
        return L("Machine limits");
    case Domain::ConfigItemDef::Category::Notes:
        return L("Notes");
    case Domain::ConfigItemDef::Category::OutputOptions:
        return L("Output options");
    case Domain::ConfigItemDef::Category::Pad:
        return L("Pad");
    case Domain::ConfigItemDef::Category::SkirtAndBrim:
        return L("Skirt and brim");
    case Domain::ConfigItemDef::Category::Speed:
        return L("Speed");
    case Domain::ConfigItemDef::Category::SupportMaterial:
        return L("Support material");
    case Domain::ConfigItemDef::Category::Supports:
        return L("Supports");
    case Domain::ConfigItemDef::Category::WipeOptions:
        return L("Wipe options");
    case Domain::ConfigItemDef::Category::MultipleExtruders:
        return L("Multiple extruders");
    case Domain::ConfigItemDef::Category::Unkown:
        return L("Unkown");
    case Domain::ConfigItemDef::Category::Hidden:
        return L("Hidden");
    case Domain::ConfigItemDef::Category::SingleExtruderMMSetup:
        return L("Single extruder MM setup");
    case Domain::ConfigItemDef::Category::MaterialPrintingProfile:
        return L("Material printing profile");
    case Domain::ConfigItemDef::Category::Bed:
        return L("Bed");
    case Domain::ConfigItemDef::Category::PreferencesGeneral:
        return L("General");
    case Domain::ConfigItemDef::Category::Appearance:
        return L("Appearance");
    case Domain::ConfigItemDef::Category::Camera:
        return L("Camera");
    case Domain::ConfigItemDef::Category::Services:
        return L("Services");
    }

    return "";
}

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
        [](const AppConfigLocation location) {
            return "app_config_settings";
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
            [](const PhysicalPrinterLocation location) {},
            [](const AppConfigLocation location) {}
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
