///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/Config/ConfigItemTextFields.hpp"

#include "Slic3r/App/Yoga/InputTextField.hpp"

#include "Slic3r/Biz/IConfigBoxSetter.hpp"

#include <fmt/format.h>

using namespace Slic3r::App::Yoga;

namespace Slic3r::App {

ConfigItemTextFields::ConfigItemTextFields(
    size_t index,
    const Domain::ConfigItem& data,
    Biz::IConfigBoxSetter& cbi_container,
    size_t cbi_index
) :
    ConfigItemControl(index, data),
    m_cbi_container(cbi_container),
    m_cbi_index(cbi_index)
{
    set_orientation(Orientation::Horizontal);
    set_gap(5);
    set_min_size({200, 0});

    on_data_update();
}

void ConfigItemTextFields::on_data_update()
{
    const size_t value_size = m_state->get<std::vector<double>>().size();

    if (m_fields.size() != value_size) {
        reconstruct_fields();
    } else {
        update_values();
    }
}

void ConfigItemTextFields::reconstruct_fields()
{
    for (size_t child_index = 0; child_index < object_count(); ++child_index) {
        remove(get_item(0));
    }
    m_fields.clear();

    const std::string tooltip = tooltip_text();

    const std::vector<double> values = m_state->get<std::vector<double>>();
    m_fields.reserve(values.size());
    for (double value : std::as_const(values)) {
        Field& field           = m_fields.emplace_back();
        field.double_validator = std::make_unique<DoubleValidator>(
            m_state->def().min.value_or(std::numeric_limits<double>::lowest()),
            m_state->def().max.value_or(std::numeric_limits<double>::max())
        );
        field.textfield = emplace_back<InputTextField>("ConfigItemTextField");
        field.textfield->set_validator(field.double_validator.release());
        field.textfield->set_text(fmt::format("{:.10g}", value));
        field.textfield->set_flex_grow(1);
        field.textfield->callbacks().text_edited = [this]() { send_data(); };
        field.textfield->set_tooltip(tooltip);
    }
}

void ConfigItemTextFields::update_values()
{
    const std::vector<double> values = m_state->get<std::vector<double>>();
    for (size_t i = 0; i < values.size(); ++i) {
        m_fields.at(i).textfield->set_text(fmt::format("{}", values.at(i)));
    }
}

void ConfigItemTextFields::send_data()
{
    std::vector<double> values(m_fields.size());
    for (size_t i = 0; i < values.size(); ++i) {
        values[i] = m_fields.at(i).double_validator->value();
    }
    m_cbi_container.set_item_value(*m_state, Domain::ConfigValue{values}, m_cbi_index);
}

} // namespace Slic3r::App
