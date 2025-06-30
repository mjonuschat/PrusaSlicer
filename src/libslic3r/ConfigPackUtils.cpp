#include "libslic3r/ConfigPackUtils.hpp"
#include "Slic3r/Domain/Types.hpp"

namespace Slic3r::Biz::Slicing {

using ParserConfig = Parser::IO::Config;
using Domain::FloatOrPercentage;
using Domain::Percentage;
using Domain::Vec2d;
using Domain::FullConfig;
using Domain::FullConfigFDM;
using Domain::FullConfigSLA;
using Domain::ConfigPackFDM;
using Domain::ConfigPackSLA;
using Domain::ConfigPack;
using ParserValue = Parser::IO::Value;
using ParserVector = Parser::IO::Vector;
using ParserScalar = Parser::IO::Scalar;
using Domain::EnumWrapper;
using Domain::EnumVectorWrapper;
using Domain::ConfigValue;
using Domain::overloaded;

namespace {
void copy(const std::string& name, const ConfigValue& value, ParserConfig& config)
{
    value.visit([&](auto&& item_value){
        using ValueType = std::remove_cvref_t<decltype(item_value)>;
        if constexpr (std::is_same_v<ValueType, EnumWrapper>) {
            const EnumWrapper& enum_wrapper{item_value};
            config.set(name, std::string{enum_wrapper.get_string()});
        } else if constexpr (std::is_same_v<ValueType, Domain::EnumVectorWrapper>) {
            const EnumVectorWrapper& enums_wrapper{item_value};
            const auto& values{enums_wrapper.get_strings()};
            std::vector<std::string> enum_values;
            enum_values.insert(enum_values.end(), values.begin(), values.end());
            config.set(name, enum_values);
        } else {
            config.set(name, item_value);
        }
    });
}
} // namespace

Parser::IO::Config get_parser_config(const FullConfigFDM& full_config)
{
    ParserConfig result;
    for (const auto& [key, value] : full_config.values()) {
        copy(key, value, result);
    }

    return result;
}

Parser::IO::Config get_parser_config(const FullConfigSLA& full_config)
{
    ParserConfig result;
    for (const auto& [key, value] : full_config.values()) {
        copy(key, value, result);
    }

    return result;
}

} // namespace Slic3r::Biz::Slicing
