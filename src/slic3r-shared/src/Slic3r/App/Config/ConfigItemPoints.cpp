///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/Config/ConfigItemPoints.hpp"

#include "Slic3r/App/Yoga/InputTextField.hpp"

using namespace Slic3r::App::Yoga;

namespace Slic3r::App {

ConfigItemPoints::ConfigItemPoints(size_t index, const Domain::ConfigItem &data) : Biz::DataObserver<Domain::ConfigItem>(index, data)
{
    set_orientation(Orientation::Horizontal);
    set_gap(5);

    m_input_x = emplace_back<InputTextField>();
    m_input_y = emplace_back<InputTextField>();

    set_width(150);
}

void ConfigItemPoints::on_data_update()
{
    std::vector<Domain::Vec2d> data = m_state->get<std::vector<Domain::Vec2d>>();

    // m_input_x->set_text(std::to_string(data.front()));
}

}
