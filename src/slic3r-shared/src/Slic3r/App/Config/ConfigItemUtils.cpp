///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/Config/ConfigItemUtils.hpp"

#include "Slic3r/Domain/Config.hpp"
#include "Slic3r/Biz/I18N/I18N.hpp"

#include <fmt/format.h>

namespace Slic3r::App {

std::string ConfigItemUtils::config_item_to_string(const Domain::ConfigItem& config_item)
{
    return config_item_to_string(config_item, config_item.value());
}

std::string ConfigItemUtils::config_item_tooltip(const Domain::ConfigItem& config_item)
{
    const Domain::ConfigItemDef& def = config_item.def();
    std::string text = fmt::format("{}\n\n{}: {}", Biz::_u8(def.tooltip), Biz::_u8L("Parameter name"), def.name);

    if (def.min.has_value()) {
        text += fmt::format("\nMin: {:.10g}", def.min.value());
    }
    if (def.max.has_value()) {
        text += fmt::format("\nMax: {:.10g}", def.max.value());
    }

    return text;
}

std::string ConfigItemUtils::config_item_to_string(
    const Domain::ConfigItem& config_item,
    const Domain::ConfigValue& value
)
{
    std::string result;
    if (*config_item.def().type == typeid(std::string)) {
        result = value.get<std::string>();
    } else if (*config_item.def().type == typeid(int)) {
        result = std::to_string(value.get<int>());
    } else if (*config_item.def().type == typeid(double)) {
        result = fmt::format("{:.10g}", value.get<double>());
    } else if (*config_item.def().type == typeid(Domain::Percentage)) {
        result = fmt::format("{:.10g}", value.get<Domain::Percentage>().value);
    } else if (*config_item.def().type == typeid(Domain::FloatOrPercentage)) {
        Domain::FloatOrPercentage fop = value.get<Domain::FloatOrPercentage>();
        result = fop.is_percentage() ? fmt::format("{:.10g} %", fop.percentage().value) :
                                       fmt::format("{:.10g}", fop.float_value());
    } else if (*config_item.def().type == typeid(Domain::EnumWrapper)) {
        const Domain::EnumWrapper enum_wrapper = value.get<Domain::EnumWrapper>();
        result = enum_wrapper.def().at(enum_wrapper.index_of_value(enum_wrapper.value())).str_ui;
    }

    return result;
}

} // namespace Slic3r::App
