///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include <Slic3r/Domain/Color.hpp>

#include "Slic3r/App/Platform/ThemeTypes.hpp"

#include <imgui.h>

namespace Slic3r::App::Platform {

class AbstractTheme
{
public:
    enum class Style
    {
        Dark
    };

    virtual ~AbstractTheme() = default;

    virtual const Domain::ColorRGBA&
    color(Color color_id, ColorGroup group_id = ColorGroup::Default) const = 0;

    virtual const ImColor&
    color_imgui(Color color_id, ColorGroup group_id = ColorGroup::Default) const = 0;

    virtual void set_style(Style style) = 0;

    virtual void initialize_imgui_style() = 0;
};

} // namespace Slic3r::App::Platform
