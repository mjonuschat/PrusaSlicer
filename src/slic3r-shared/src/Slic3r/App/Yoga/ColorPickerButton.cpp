///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/Yoga/ColorPickerButton.hpp"

#include "Slic3r/App/Yoga/ContextPopup.hpp"
#include "Slic3r/App/Yoga/ColorPicker.hpp"

namespace Slic3r::App::Yoga {

ColorPickerButton::ColorPickerButton(const std::string& name)
{
    set_object_name(name.empty() ? "ColorPickerButton" : name);
    set_width(30);
    set_height(25);

    m_popup = emplace_back<ContextPopup>("ColorPickerPopup");
    m_popup->set_orientation(Orientation::Vertical);
    m_popup->set_padding(5);
    m_color_picker_internal = m_popup->emplace_back<ColorPicker>();
    m_color_picker_internal->set_flex_grow(1);
    m_color_picker_internal->callbacks().color_edited = [this](const ImColor& color)
    { on_color_edited(color); };
    m_popup->set_position(Position::Right);

    AbstractButton::callbacks().action = [this]
    {
        if (m_popup->opened()) {
            m_popup->close();
        } else {
            m_popup->open();
        }
    };

    set_color(m_color);
}

ColorPickerButton::Callbacks& ColorPickerButton::callbacks()
{
    return m_callbacks;
}

const ImColor& ColorPickerButton::color() const
{
    return m_color;
}

void ColorPickerButton::set_color(const ImColor& color)
{
    m_color = color;
    m_color_picker_internal->set_color(color);
    set_background_color(color);
}

ImGuiColorEditFlags ColorPickerButton::flags() const
{
    return m_color_picker_internal->flags();
}

void ColorPickerButton::set_flags(ImGuiColorEditFlags flags)
{
    return m_color_picker_internal->set_flags(flags);
}

void ColorPickerButton::on_color_edited(const ImColor& color)
{
    m_color = color;
    set_background_color(color);
    if (m_callbacks.color_edited) {
        m_callbacks.color_edited(m_color);
    }
}

}
