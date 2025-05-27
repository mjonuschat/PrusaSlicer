#include "libslic3r/ConfigPackFDMUtils.hpp"
#include "Slic3r/Domain/Types.hpp"

namespace Slic3r::Biz::Slicing {

using Domain::ConfigItem;
using ParserConfig = Parser::IO::Config;
using Domain::FloatOrPercentage;
using Domain::Percentage;
using Domain::Vec2d;
using Domain::FullConfigFDM;
using Domain::ConfigPackFDM;
using Domain::ConfigItemDef;
using ParserValue = Parser::IO::Value;
using ParserVector = Parser::IO::Vector;
using ParserScalar = Parser::IO::Scalar;
using Domain::EnumWrapper;
using Domain::EnumVectorWrapper;

namespace {
void copy(const ConfigItem& item, ParserConfig& config)
{
    if (item.is_null()) {
        return;
    }

    item.visit([&](auto&& item_value){
        using ValueType = std::remove_cvref_t<decltype(item_value)>;
        if constexpr (std::is_same_v<ValueType, EnumWrapper>) {
            const EnumWrapper& enum_wrapper{item_value};
            config.set(item.name(), std::string{enum_wrapper.get_string()});
        } else if constexpr (std::is_same_v<ValueType, Domain::EnumVectorWrapper>) {
            const EnumVectorWrapper& enums_wrapper{item_value};
            const auto& values{enums_wrapper.get_strings()};
            std::vector<std::string> enum_values;
            enum_values.insert(enum_values.end(), values.begin(), values.end());
            config.set(item.name(), enum_values);
        } else {
            config.set(item.name(), item_value);
        }
    });
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

    items.front().visit([&](auto&& item_value){
        using ValueType = std::remove_cvref_t<decltype(item_value)>;
        if constexpr (
            Domain::is_std_vector_v<ValueType>
            || std::is_same_v<ValueType, EnumVectorWrapper>
        ) {
            PANIC("Vector of vector of values is not supported!");
        } else if constexpr (std::is_same_v<ValueType, EnumWrapper>) {
            std::vector<std::string> enums;
            for (const ConfigItem& item : items) {
                enums.push_back(std::string{item.get<EnumWrapper>().get_string()});
            }
            config.set(items.front().name(), enums);
        } else {
            config.set(items.front().name(), extract_values<ValueType>(items));
        }
    });
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
