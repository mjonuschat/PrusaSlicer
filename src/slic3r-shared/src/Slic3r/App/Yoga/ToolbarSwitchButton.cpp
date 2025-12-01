///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/Yoga/ToolbarSwitchButton.hpp"

namespace Slic3r::App::Yoga {

ToolbarSwitchButton::ToolbarSwitchButton(
    SwitchPosition switch_position,
    Render::Icon icon,
    const std::string& tooltip
) :
    ToolbarButton(icon, tooltip),
    m_switch_position(switch_position)
{
    set_background_color(ImColor(41, 41, 41));
    set_background_color_checked(ImColor(39, 47, 64));

    switch (switch_position) {
    case SwitchPosition::Left:
        set_draw_flags(ImDrawFlags_RoundCornersLeft);
        break;
    case SwitchPosition::Center:
        set_draw_flags(ImDrawFlags_None);
        break;
    case SwitchPosition::Right:
        set_draw_flags(ImDrawFlags_RoundCornersRight);
        break;
    }
}

} // namespace Slic3r::App::Yoga
