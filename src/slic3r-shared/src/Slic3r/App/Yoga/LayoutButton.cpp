///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/Yoga/LayoutButton.hpp"

#include "Slic3r/App/Imgui/ImguiExtension.hpp"
#include "Slic3r/App/Yoga/Icon.hpp"
#include "Slic3r/App/Yoga/Text.hpp"
#include "Slic3r/App/Yoga/Rectangle.hpp"

namespace Slic3r::App::Yoga {

Slic3r::App::Yoga::LayoutButton::LayoutButton(const std::string& label)
    : LayoutButton(label, '\0')
{}

LayoutButton::LayoutButton(const std::string& label, wchar_t icon)
    : LayoutButton(label, icon, "")
{}

Slic3r::App::Yoga::LayoutButton::LayoutButton(
    const std::string& label, wchar_t icon, const std::string& tooltip
)
    : AbstractButton(icon, tooltip)
{
    set_orientation(Orientation::Horizontal);

    m_background = emplace_back<Rectangle>();
    m_background->set_padding(3);
    m_background->set_justify_content(YGJustifyCenter);
    m_background->set_align_items(YGAlignCenter);
    m_background->set_gap(5);
    m_background->set_flex_grow(1);

    m_icon = m_background->emplace_back<Icon>(icon);
    m_icon->set_visible(icon != '\0');
    // m_icon->set_height_percent(100);

    m_text = m_background->emplace_back<Text>(label);
    m_text->set_visible(!label.empty());

    set_background_color({41, 41, 41});
}

void LayoutButton::process_events(Vec2f pos, Vec2f size)
{
    AbstractButton::process_events(pos, size);

    m_background->set_fill(m_hovered ? m_background_color_hover : m_background_color);
}

const std::string& Slic3r::App::Yoga::LayoutButton::label() const { return m_text->text(); }

void Slic3r::App::Yoga::LayoutButton::set_label(const std::string& label)
{
    m_text->set_text(label);
    m_text->set_visible(!label.empty());
}

const ImColor& LayoutButton::background_color() const { return m_background->fill(); }

void LayoutButton::set_background_color(const ImColor& color)
{
    m_background_color = color;
    m_background_color_hover = Imgui::adjust_brightness(m_background_color, 1.2);
}

Render::ImguiFontType LayoutButton::label_font_type() const { return m_text->font_type(); }

void LayoutButton::set_label_font_type(Render::ImguiFontType label_font_type)
{
    m_text->set_font_type(label_font_type);
}

const Paddings& LayoutButton::content_padding() { return m_background->padding(); }

void LayoutButton::set_content_padding(const Paddings& padding)
{
    m_background->set_padding(padding);
}

} // namespace Slic3r::App::Yoga
