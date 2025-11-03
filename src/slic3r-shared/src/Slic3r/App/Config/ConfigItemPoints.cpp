///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/Config/ConfigItemPoints.hpp"

#include "Slic3r/App/Yoga/InputTextField.hpp"
#include "Slic3r/App/Yoga/Validator.hpp"

#include "Slic3r/Biz/Preset/PresetInteractor.hpp"

#include <fmt/format.h>

using namespace Slic3r::App::Yoga;

namespace Slic3r::App {

ConfigItemPoints::ConfigItemPoints(
    size_t index,
    const Domain::ConfigItem& data,
    Biz::Preset::PresetInteractor& preset_interactor,
    size_t cbi_index
) :
    ConfigItemControl(index, data),
    m_preset_interactor(preset_interactor),
    m_cbi_index(cbi_index)
{
    set_orientation(Orientation::Horizontal);
    set_gap(5);
    set_width(150);

    m_validator_x = std::make_unique<DoubleValidator>(
        m_state->def().min.value_or(std::numeric_limits<double>::lowest()),
        m_state->def().max.value_or(std::numeric_limits<double>::max())
    );
    m_input_x = emplace_back<InputTextField>("ConfigItemPointX");
    m_input_x->set_flex_grow(1);
    m_input_x->set_validator(m_validator_x.release());
    m_input_x->callbacks().text_edited = [this]() { send_data(); };

    m_validator_y = std::make_unique<DoubleValidator>(
        m_state->def().min.value_or(std::numeric_limits<double>::lowest()),
        m_state->def().max.value_or(std::numeric_limits<double>::max())
    );
    m_input_y = emplace_back<InputTextField>("ConfigItemPointY");
    m_input_y->set_flex_grow(1);
    m_input_y->set_validator(m_validator_y.release());
    m_input_y->callbacks().text_edited = [this]() { send_data(); };

    on_data_update();
}

void ConfigItemPoints::on_data_update()
{
    std::vector<Domain::Vec2d> data = m_state->get<std::vector<Domain::Vec2d>>();

    m_input_x->set_text(fmt::format("{:.10g}", data.front().x()));
    m_input_y->set_text(fmt::format("{:.10g}", data.front().y()));
}

void ConfigItemPoints::send_data()
{
    std::vector<Domain::Vec2d> data = {
        Domain::Vec2d(m_validator_x->value(), m_validator_y->value())
    };

    m_preset_interactor.set_item_value(*m_state, Domain::ConfigValue{data}, m_cbi_index);
}

} // namespace Slic3r::App
