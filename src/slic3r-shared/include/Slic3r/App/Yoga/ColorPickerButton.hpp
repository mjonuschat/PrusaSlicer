///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/Yoga/RectangleButton.hpp"

namespace Slic3r::App::Yoga {

class ContextPopup;
class ColorPicker;

class ColorPickerButton : public RectangleButton
{
public:
    struct Callbacks
    {
        std::function<void(const ImColor& color)> color_edited{nullptr};
    };

    ColorPickerButton(const std::string& name = {});

    Callbacks& callbacks();

    const ImColor& color() const;
    void set_color(const ImColor& color);

    ImGuiColorEditFlags flags() const;
    void set_flags(ImGuiColorEditFlags flags);

protected:
    void on_color_edited(const ImColor& color);

private:
    Callbacks m_callbacks;
    ImColor m_color                      = IM_COL32_BLACK;
    ContextPopup* m_popup                = nullptr;
    ColorPicker* m_color_picker_internal = nullptr;
};

} // namespace Slic3r::App::Yoga
