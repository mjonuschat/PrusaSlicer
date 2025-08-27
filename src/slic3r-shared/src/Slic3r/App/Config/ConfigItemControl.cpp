///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/Config/ConfigItemControl.hpp"

#include "Slic3r/App/I18N/I18N.hpp"

namespace Slic3r::App {

ConfigItemControl::ConfigItemControl(size_t index, const Domain::ConfigItem& data) :
    Biz::DataObserver<Domain::ConfigItem>(index, data)
{}

std::string ConfigItemControl::tooltip_text() const
{
    const Domain::ConfigItemDef& def = m_state->def();
    std::string text = fmt::format("{}\n\nParameter name: {}", def.tooltip, def.name);

    std::optional<std::string> default_val = default_value();
    if (default_val.has_value()) {
        text += "\nDefault value: " + default_val.value();
    }

    if (def.min.has_value()) {
        text += fmt::format("\nMin: {:.10g}", def.min.value());
    }
    if (def.max.has_value()) {
        text += fmt::format("\nMax: {:.10g}", def.max.value());
    }

    return text;
}

std::optional<std::string> ConfigItemControl::default_value() const
{
    if (!m_state->def().init_fn) {
        return {};
    }

    Domain::ConfigValue value = m_state->def().init_fn();

    if (*m_state->def().type == typeid(std::string)) {
        return m_state->value().get<std::string>();
    } else if (*m_state->def().type == typeid(double)) {
        return fmt::format("{:.10g}", m_state->value().get<double>());
    } else if (*m_state->def().type == typeid(int)) {
        return std::to_string(m_state->value().get<int>());
    } else if (*m_state->def().type == typeid(bool)) {
        return m_state->value().get<bool>() ? "true" : "false";
    } else if (*m_state->def().type == typeid(Domain::Percentage)) {
        return fmt::format("{:.10g} %", m_state->value().get<Domain::Percentage>().value);
    } else if (*m_state->def().type == typeid(Domain::FloatOrPercentage)) {
        Domain::FloatOrPercentage value = m_state->value().get<Domain::FloatOrPercentage>();
        return value.is_percentage() ? fmt::format("{:.10g} %", value.percentage().value) :
                                       fmt::format("{:.10g}", value.float_value());
    } else if (*m_state->def().type == typeid(std::vector<double>)) {
        std::vector<double> values = m_state->get<std::vector<double>>();

        fmt::memory_buffer buffer;
        buffer.push_back('[');
        for (size_t i = 0; i < values.size(); ++i) {
            if (i > 0) {
                buffer.push_back(',');
            }

            fmt::format_to(std::back_inserter(buffer), "{:.10g}", values[i]);
        }
        buffer.push_back(']');

        return fmt::to_string(buffer);

    } else if (*m_state->def().type == typeid(Domain::EnumWrapper)) {
        const Domain::EnumWrapper enum_wrapper = m_state->get<Domain::EnumWrapper>();

        return enum_wrapper.def().at(enum_wrapper.index_of_value(enum_wrapper.value())).str_ui;
    }

    return {};
}

} // namespace Slic3r::App
