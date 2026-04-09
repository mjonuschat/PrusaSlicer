///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/ToolBar/ToolBarButton.hpp"
#include "Slic3r/App/Render/ImguiTypes.hpp"

namespace Slic3r::App {

class ToolBarSwitchButton : public ToolBarButton
{
public:
    enum class SwitchPosition
    {
        Left,
        Center,
        Right,
    };

    ToolBarSwitchButton(
        SwitchPosition switch_position,
        Render::Icon icon,
        const std::string& label   = {},
        const std::string& tooltip = {}
    );

private:
    SwitchPosition m_switch_position{SwitchPosition::Left};
};

} // namespace Slic3r::App::Yoga
