#include "ConfigPackSLAUtils.hpp"
#include "Slic3r/Domain/Types.hpp"

namespace Slic3r::Biz::Slicing {

using Domain::ConfigItem;
using ParserConfig = Parser::IO::Config;
using Domain::EnumWrapper;
using Domain::EnumVectorWrapper;
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
