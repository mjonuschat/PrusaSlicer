///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/Config/ConfigItemColorPicker.hpp"

#include "Slic3r/Biz/Algorithms/Color.hpp"

using namespace Slic3r::App::Yoga;

namespace Slic3r::App {

ConfigItemColorPicker::ConfigItemColorPicker(size_t index, const Domain::ConfigItem& data) :
    ConfigItemControl(index, data)
{
    on_data_update();
}

void ConfigItemColorPicker::on_data_update()
{
    Domain::ColorRGB color;
    if (Biz::Algorithms::Color::decode_color(m_state->get<std::string>(), color)) {
        set_fill(ImColor(color.r(), color.g(), color.b()));
    }
}

} // namespace Slic3r::App
