///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

namespace Slic3r::App::Platform {

/**
 * @brief Color Id enum
 */
enum class Color
{
    Text,
    TextLink,
    WindowBg,
    WindowBgAlternate,
    Control,
    AccentPrimary,
    AccentSecondary,
    Button,
    ButtonTransparent,
    Scrollbar,
    NavCursor,
    ModalWindowDimBg,
    Warning,
    Error,
    SceneBg,
    Transparent
};

/**
 * @brief Color state group, tied with Color itself, consider Colors[Color][ColorGroup]
 */
enum class ColorGroup
{
    Default, ///< Unselected control
    Disabled, ///< Disabled control
    Active, ///< Selected active state
    ActiveDisabled, ///< Activated & Disabled state
    Hovered, ///< Hovered by mouse
};

} // namespace Slic3r::App::Platform
