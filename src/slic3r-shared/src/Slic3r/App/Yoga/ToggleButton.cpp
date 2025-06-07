///|/ Copyright (c) Prusa Research 2018 - 2025 Oleksandra Iushchenko @YuSanka, Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/Yoga/ToggleButton.hpp"

#include "Slic3r/App/Yoga/Tooltip.hpp"
#include "Slic3r/App/Yoga/Toggler.hpp"
#include "Slic3r/App/Yoga/Text.hpp"

namespace Slic3r::App::Yoga {

ToggleButton::ToggleButton(const std::string& label, const std::string& tooltip)
    : AbstractButton(tooltip)
{
    set_orientation(Orientation::Horizontal);
    set_align_items(YGAlignCenter);

    set_checkable(true);

    m_toggler = emplace_back<Toggler>();

    m_label = emplace_back<Text>(label);
    m_label->set_margin({ 5.f });
    m_label->set_visible(!label.empty());

    m_tooltip->set_position(Position::Bottom);
    m_tooltip->set_offset(0);
}

void ToggleButton::process_events(Vec2f pos, Vec2f size)
{
    AbstractButton::process_events(pos, size);
}

void ToggleButton::set_label(const std::string& label)
{
    m_label->set_text(label);
    m_label->set_visible(!label.empty());
}

std::string ToggleButton::get_label()
{
    return m_label->text();
}

void ToggleButton::set_font_type(Render::ImguiFontType font_type)
{
    if (m_label)
        m_label->set_font_type(font_type);
}

void ToggleButton::checked_updated_internal()
{
    AbstractButton::checked_updated_internal();
    m_toggler->set_checked(checked());
}

} // namespace Slic3r::App::Yoga
