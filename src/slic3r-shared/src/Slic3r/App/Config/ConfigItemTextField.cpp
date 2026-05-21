///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/Config/ConfigItemTextField.hpp"

#include "Slic3r/Biz/IConfigBoxSetter.hpp"
#include "Slic3r/Biz/I18N/I18N.hpp"

#include "Slic3r/App/Config/ConfigItemUtils.hpp"

#include <boost/algorithm/string.hpp>
#include <imgui_internal.h>
#include <fmt/format.h>

using namespace Slic3r::App::Yoga;

namespace Slic3r::App {

ConfigItemTextField::ConfigItemTextField(
    size_t index,
    const Domain::ConfigItem& data,
    Biz::IConfigBoxSetter& cbi_container,
    size_t cbi_index
) :
    ConfigItemControl(index, data),
    m_cbi_container(cbi_container),
    m_cbi_index(cbi_index)
{
    m_tooltip->set_text_wrap(true);
    m_tooltip->content_item()->set_width(350);

    on_data_update();

    callbacks().text_edited = [this]()
    {
        if (*m_state->def().type == typeid(std::string)) {
            m_cbi_container.set_item_value(*m_state, Domain::ConfigValue{text()}, m_cbi_index);
        } else if (*m_state->def().type == typeid(std::vector<std::string>)) {
            std::vector<std::string> new_strings;
            boost::split(new_strings, text(), boost::is_any_of("\n"));
            std::erase_if(new_strings, [](const std::string& s) { return s.empty(); });
            m_cbi_container.set_item_value(*m_state, Domain::ConfigValue{new_strings}, m_cbi_index);
        } else if (*m_state->def().type == typeid(double)) {
            m_cbi_container.set_item_value(
                *m_state,
                Domain::ConfigValue{m_double_validator->value()},
                m_cbi_index
            );
        } else if (*m_state->def().type == typeid(Domain::Percentage)) {
            m_cbi_container.set_item_value(
                *m_state,
                Domain::ConfigValue{Domain::Percentage{m_percentage_validator->value()}},
                m_cbi_index
            );
        } else if (*m_state->def().type == typeid(Domain::FloatOrPercentage)) {
            const std::string value_text = text();
            if (m_percentage_validator->entered_percentage_symbol()) {
                m_cbi_container.set_item_value(
                    *m_state,
                    Domain::ConfigValue{Domain::FloatOrPercentage{
                        Domain::Percentage{m_percentage_validator->value()}
                    }},
                    m_cbi_index
                );
            } else {
                m_cbi_container.set_item_value(
                    *m_state,
                    Domain::ConfigValue{Domain::FloatOrPercentage{m_percentage_validator->value()}},
                    m_cbi_index
                );
            }
        } else {
            PANIC("Item is used for unexpected parameter type");
        }
    };
}

void ConfigItemTextField::on_data_update()
{
    if (m_last_item != m_state) {
        if (!m_is_multiline || m_is_multiline.value() != m_state->def().multiline) {
            if (m_state->def().multiline) {
                set_flags(flags() | ImGuiInputTextFlags_Multiline);
                set_resizable(true);
                set_height(YGUndefined);
                input_text()->set_min_height(80);
            } else {
                set_flags(flags() & ~ImGuiInputTextFlags_Multiline);
                set_resizable(false);
                set_height(YGUndefined);
                input_text()->set_min_height(0);
            }
            m_is_multiline = m_state->def().multiline;
        }

        if (!m_is_full_width || m_is_full_width.value() != m_state->def().full_width) {
            set_width(m_state->def().full_width ? YGUndefined : 80);
            m_is_full_width = m_state->def().full_width;
        }

        m_last_item = m_state;

        set_tooltip(ConfigItemUtils::config_item_tooltip(*m_state));

        if (*m_state->def().type == typeid(double)) {
            m_double_validator = std::make_unique<DoubleValidator>(
                m_state->def().min.value_or(std::numeric_limits<double>::lowest()),
                m_state->def().max.value_or(std::numeric_limits<double>::max())
            );
            set_validator(m_double_validator.release());
        } else if (
            *m_state->def().type == typeid(Domain::Percentage)
            || *m_state->def().type == typeid(Domain::FloatOrPercentage)
        )
        {
            m_percentage_validator = std::make_unique<PercentageValidator>(
                m_state->def().min.value_or(std::numeric_limits<double>::lowest()),
                m_state->def().max.value_or(std::numeric_limits<double>::max())
            );
            m_percentage_validator->set_visible_percentage_symbol(
                *m_state->def().type == typeid(Domain::FloatOrPercentage)
            );
            set_validator(m_percentage_validator.release());
        }
    }

    if (mixed()) {
        set_override_label(Biz::_u8L("Mixed"));
        set_font_type(Render::ImguiFontType::Italic);
        return;
    }

    set_override_label(std::string());
    set_font_type(Render::ImguiFontType::Regular);
    if (!overriden().value_or(true)) {
        update_value(*m_cbi_container.get_override_original_value(*m_state, location_index()));
    } else {
        update_value(m_state->value());
    }
}

void ConfigItemTextField::update_value(const Domain::ConfigValue& value)
{
    if (*m_state->def().type == typeid(std::string)) {
        set_text(m_state->value().get<std::string>());
    } else if (*m_state->def().type == typeid(std::vector<std::string>)) {
        std::vector<std::string> old_strings = m_state->value().get<std::vector<std::string>>();
        std::string new_string{};
        for (const std::string& str : old_strings) {
            new_string += str + "\n";
        }
        if (!new_string.empty()) {
            new_string.pop_back();
        }
        set_text(new_string);
    } else if (*m_state->def().type == typeid(double)) {
        set_text(fmt::format("{:.10g}", m_state->value().get<double>()));
    } else if (*m_state->def().type == typeid(Domain::Percentage)) {
        set_text(fmt::format("{:.10g}", m_state->value().get<Domain::Percentage>().value));
    } else if (*m_state->def().type == typeid(Domain::FloatOrPercentage)) {
        Domain::FloatOrPercentage value = m_state->value().get<Domain::FloatOrPercentage>();
        set_text(
            value.is_percentage() ? fmt::format("{:.10g} %", value.percentage().value) :
                                    fmt::format("{:.10g}", value.float_value())
        );
    } else {
        PANIC("Item is used for unexpected parameter type");
    }
}

} // namespace Slic3r::App
