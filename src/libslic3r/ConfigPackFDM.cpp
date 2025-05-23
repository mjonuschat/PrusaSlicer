#include "libslic3r/ConfigPackFDM.hpp"
#include "Slic3r/Domain/Types.hpp"

namespace Slic3r::Biz::Slicing {

using Domain::ConfigItem;
using ParserConfig = Parser::IO::Config;
using Domain::ConfigItemType;
using Domain::FloatOrPercentage;
using Domain::Percentage;
using Domain::Vec2d;
using Domain::FullConfigFDM;

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
} // namespace

Parser::IO::Config ConfigPackFDM::get_parser_config() const
{
    ParserConfig config;

    for (const ConfigItem& item : printer) {
        copy(item, config);
    }
    for (const ConfigItem& item : print) {
        copy(item, config);
    }
    for (const Domain::ToolPrintSettings& tool_settings : tool) {
        for (const ConfigItem& item : tool_settings) {
            copy(item, config);
        }
    }
    for (const Domain::FilamentSettings& filament_settings : filament) {
        for (const ConfigItem& item : filament_settings) {
            copy(item, config);
        }
    }
    for (const ConfigItem& item : project) {
        copy(item, config);
    }
    return config;
}

template <typename T>
using Refs = std::vector<std::reference_wrapper<const T>>;

template<typename T>
static Refs<T> to_refs(const std::vector<T>& items) {
    Refs<T> result;
    result.insert(result.end(), items.begin(), items.end());
    return result;
}

FullConfigFDM ConfigPackFDM::get_full_config() const
{
    return {
        printer,
        to_refs(tool),
        print,
        to_refs(filament),
        project
    };
}

PrintConfigView ConfigPackFDM::get_view() const {
    return {std::make_shared<FullConfigFDM>(get_full_config())};
}

} // namespace Slic3r::Biz::Slicing
