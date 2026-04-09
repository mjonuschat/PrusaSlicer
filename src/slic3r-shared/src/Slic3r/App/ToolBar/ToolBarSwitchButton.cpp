///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/ToolBar/ToolBarSwitchButton.hpp"

using namespace Slic3r::App::Yoga;

namespace Slic3r::App {

ToolBarSwitchButton::ToolBarSwitchButton(
    SwitchPosition switch_position,
    Render::Icon icon,
    const std::string& label,
    const std::string& tooltip
) :
    ToolBarButton(icon, tooltip),
    m_switch_position(switch_position)
{
    set_label(label);

    set_background_color(Platform::Color::Button);

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
