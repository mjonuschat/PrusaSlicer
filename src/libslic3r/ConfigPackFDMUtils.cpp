#include "libslic3r/ConfigPackFDMUtils.hpp"
#include "Slic3r/Domain/Types.hpp"

namespace Slic3r::Biz::Slicing {

using Domain::ConfigItem;
using ParserConfig = Parser::IO::Config;
using Domain::ConfigItemType;
using Domain::FloatOrPercentage;
using Domain::Percentage;
using Domain::Vec2d;
using Domain::FullConfigFDM;
using Domain::ConfigPackFDM;
using Domain::ConfigItemDef;
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

void copy(const std::vector<ConfigItem>& items, ParserConfig& config)
{
    ASSERT(items.size() > 0);

    if (std::ranges::any_of(items, [](const ConfigItem& item) { return item.is_null(); })) {
        return;
    }

    ParserVector values;

    switch (items.front().type()) {
    case ConfigItemType::Bool:
        config.set(items.front().name(), extract_values<bool>(items));
        break;
    case ConfigItemType::Int:
        config.set(items.front().name(), extract_values<int>(items));
        break;
    case ConfigItemType::IntOptional:
        config.set(items.front().name(), extract_values<std::optional<int>>(items));
        break;
    case ConfigItemType::Double:
        config.set(items.front().name(), extract_values<double>(items));
        break;
    case ConfigItemType::String:
        config.set(items.front().name(), extract_values<std::string>(items));
        break;
    case ConfigItemType::Enum: {
        std::vector<std::string> enums;
        for (const ConfigItem& item : items) {
            enums.push_back(std::string{item.get_enum_strings().first});
        }
        config.set(items.front().name(), enums);
        break;
    }
    case ConfigItemType::Point:
        config.set(items.front().name(), extract_values<Vec2d>(items));
        break;
    case ConfigItemType::FloatOrPercent:
        config.set(items.front().name(), extract_values<FloatOrPercentage>(items));
        break;
    case ConfigItemType::Percent:
        config.set(items.front().name(), extract_values<Percentage>(items));
        break;
    case ConfigItemType::None:
        PANIC("Invalid config option");
        break;
    default:;
    }
}

std::vector<std::string> get_keys(const std::string& type) {
    std::vector<std::string> result;

    for (const ConfigItemDef& def : Domain::s_defs_fdm.defs()) {
        if (def.location == type || std::ranges::find(def.overrides_in, type) != def.overrides_in.end())
            result.push_back(def.name);
    }
    return result;
}

} // namespace

Parser::IO::Config get_parser_config(const ConfigPackFDM& config_pack)
{
    ParserConfig config;

    for (const ConfigItem& item : config_pack.printer) {
        copy(item, config);
    }
    for (const ConfigItem& item : config_pack.print) {
        copy(item, config);
    }

    const std::vector<std::string> tool_keys{get_keys("toolprint_settings")};
    for (const std::string& key : tool_keys) {
        std::vector<ConfigItem> items;
        for (const Domain::ToolPrintSettings& tool_settings : config_pack.tool) {
            items.push_back(tool_settings.opt(key));
        }
        copy(items, config);
    }

    const std::vector<std::string> filament_keys{get_keys("filament_settings")};
    for (const std::string& key : filament_keys) {
        std::vector<ConfigItem> items;
        for (const Domain::FilamentSettings& filament_settings : config_pack.filament) {
            items.push_back(filament_settings.opt(key));
        }
        copy(items, config);
    }
    for (const ConfigItem& item : config_pack.project) {
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

FullConfigFDM get_full_config(const ConfigPackFDM& config_pack)
{
    return {
        config_pack.printer,
        to_refs(config_pack.tool),
        config_pack.print,
        to_refs(config_pack.filament),
        config_pack.project
    };
}

PrintConfigView get_view(const ConfigPackFDM& config_pack) {
    return {std::make_shared<FullConfigFDM>(get_full_config(config_pack))};
}

} // namespace Slic3r::Biz::Slicing
