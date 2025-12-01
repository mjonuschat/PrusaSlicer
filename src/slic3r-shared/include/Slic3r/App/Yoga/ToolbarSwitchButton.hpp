///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/Yoga/ToolbarButton.hpp"
#include "Slic3r/App/Render/ImguiTypes.hpp"

namespace Slic3r::App::Yoga {

class ToolbarSwitchButton : public ToolbarButton
{
public:
    enum class SwitchPosition
    {
        Left,
        Center,
        Right,
    };

    ToolbarSwitchButton(
        SwitchPosition switch_position,
        Render::Icon icon,
        const std::string& tooltip = {}
    );

private:
    SwitchPosition m_switch_position{SwitchPosition::Left};
};

} // namespace Slic3r::App::Yoga
