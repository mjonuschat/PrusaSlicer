#include "ConfigPackSLAUtils.hpp"
#include "Slic3r/Domain/Types.hpp"

namespace Slic3r::Biz::Slicing {

using Domain::ConfigItem;
using ParserConfig = Parser::IO::Config;
using Domain::ConfigItemType;
using Domain::FloatOrPercentage;
using Domain::Percentage;
using Domain::Vec2d;
using Domain::FullConfigSLA;
using Domain::ConfigPackSLA;
using ParserValue = Parser::IO::Value;
using ParserVector = Parser::IO::Vector;
using ParserScalar = Parser::IO::Scalar;

namespace {
void copy(const ConfigItem& item, ParserConfig& config)
{
    if (item.is_null()) {
        return;
    }

    switch (item.type()) {
    case ConfigItemType::Bool:
        config.set(item.name(), item.get<bool>());
        break;
    case ConfigItemType::Int:
        config.set(item.name(), item.get<int>());
        break;
    case ConfigItemType::IntOptional:
        config.set(item.name(), item.get<std::optional<int>>());
        break;
    case ConfigItemType::Double:
        config.set(item.name(), item.get<double>());
        break;
    case ConfigItemType::String:
        config.set(item.name(), item.get<std::string>());
        break;
    case ConfigItemType::Enum:
        config.set(item.name(), std::string{item.get_enum_strings().first});
        break;
    case ConfigItemType::Point:
        config.set(item.name(), item.get<Vec2d>());
        break;
    case ConfigItemType::FloatOrPercent:
        config.set(item.name(), item.get<FloatOrPercentage>());
        break;
    case ConfigItemType::Percent:
        config.set(item.name(), item.get<Percentage>());
        break;
    case ConfigItemType::Bools:
        config.set(item.name(), item.get<std::vector<bool>>());
        break;
    case ConfigItemType::Ints:
        config.set(item.name(), item.get<std::vector<int>>());
        break;
    case ConfigItemType::Doubles:
        config.set(item.name(), item.get<std::vector<double>>());
        break;
    case ConfigItemType::Strings:
        config.set(item.name(), item.get<std::vector<std::string>>());
        break;
    case ConfigItemType::Points:
        config.set(item.name(), item.get<std::vector<Vec2d>>());
        break;
    case ConfigItemType::Enums: {
        std::vector<std::string> enum_values;
        for (const auto& pair : item.get_enums_strings()) {
            enum_values.push_back(std::string{pair.first});
        }
        config.set(item.name(), enum_values);
        break;
    }
    case ConfigItemType::None:
        PANIC("Invalid config option");
        break;
    }
}

template <typename T>
std::vector<T> extract_values(const std::vector<ConfigItem>& items) {
    std::vector<T> result;
    for (const ConfigItem& item : items) {
        result.push_back(item.get<T>());
    }
    return result;
}

} // namespace

Parser::IO::Config get_parser_config(const ConfigPackSLA& config_pack)
{
    ParserConfig config;

    for (const ConfigItem& item : config_pack.sla_printer_settings) {
        copy(item, config);
    }
    for (const ConfigItem& item : config_pack.sla_print_settings) {
        copy(item, config);
    }

    for (const ConfigItem& item : config_pack.sla_material_settings) {
        copy(item, config);
    }
    return config;
}

FullConfigSLA get_full_config(const ConfigPackSLA& config_pack)
{
    return {
        config_pack.sla_printer_settings,
        config_pack.sla_print_settings,
        config_pack.sla_material_settings
    };
}

SLAPrintConfigView get_view(const ConfigPackSLA& config_pack) {
    return {std::make_shared<FullConfigSLA>(get_full_config(config_pack))};
}

} // namespace Slic3r::Biz::Slicing
