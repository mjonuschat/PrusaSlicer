///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/Config/ConfigItemTextFields.hpp"

#include "Slic3r/App/Yoga/InputTextField.hpp"

#include <fmt/format.h>

using namespace Slic3r::App::Yoga;

namespace Slic3r::App {

ConfigItemTextFields::ConfigItemTextFields(
    size_t index,
    const Domain::ConfigItem& data,
    Biz::Preset::PresetInteractor& preset_interactor
) :
    Biz::DataObserver<Domain::ConfigItem>(index, data),
    m_preset_interactor(preset_interactor)
{
    set_orientation(Orientation::Horizontal);
    set_gap(5);
    set_min_size({200, 0});

    const std::vector<double> values = data.get<std::vector<double>>();
    m_fields.reserve(values.size());
    for (double value : values) {
        InputTextField* field = emplace_back<InputTextField>("ConfigItemTextField");
        field->set_text(fmt::format("{}", value));
        m_fields.push_back(field);
        field->set_flex_grow(1);
    }
}

void ConfigItemTextFields::on_data_update()
{
    const std::vector<double> values = m_state->get<std::vector<double>>();
    for (size_t i = 0; i < values.size(); ++i) {
        m_fields.at(i)->set_text(fmt::format("{}", values.at(i)));
    }
}

} // namespace Slic3r::App
