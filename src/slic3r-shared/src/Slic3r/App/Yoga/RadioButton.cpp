///|/ Copyright (c) Prusa Research 2018 - 2025 Oleksandra Iushchenko @YuSanka, Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/Yoga/RadioButton.hpp"

#include "Slic3r/App/Yoga/Tooltip.hpp"
#include "Slic3r/App/Yoga/Circle.hpp"
#include "Slic3r/App/Yoga/Text.hpp"

#include "imgui/imgui_internal.h"

namespace Slic3r::App::Yoga {

RadioButton::RadioButton(const std::string& label, const std::string& tooltip) :
    AbstractButton(tooltip)
{
    set_orientation(Orientation::Horizontal);
    set_align_items(YGAlignCenter);
    set_gap(10.f);

    set_checkable(true);

    m_knob = emplace_back<Circle>();
    m_knob->set_fill(GImGui->Style.Colors[ImGuiCol_WindowBg]);
    m_knob->set_disabled_fill(ImColor(20, 20, 20));
    m_knob->set_border_width(1);
    m_knob->set_min_size({12, 12});

    m_label = emplace_back<Text>(label);
    m_label->set_visible(!label.empty());

    set_tooltip_position(Position::Bottom);
}

Text* RadioButton::label() const
{
    return m_label;
}

void RadioButton::set_label(const std::string& label)
{
    m_label->set_text(label);
    m_label->set_visible(!label.empty());
}

const std::string& RadioButton::get_label() const
{
    return m_label->text();
}

void RadioButton::set_font_type(Render::ImguiFontType font_type)
{
    if (m_label)
        m_label->set_font_type(font_type);
}

void RadioButton::checked_updated_internal()
{
    AbstractButton::checked_updated_internal();

    m_knob->set_fill(GImGui->Style.Colors[checked() ? ImGuiCol_ButtonActive : ImGuiCol_WindowBg]);
    m_knob->set_border_width(checked() ? 3 : 1);
}

} // namespace Slic3r::App::Yoga
