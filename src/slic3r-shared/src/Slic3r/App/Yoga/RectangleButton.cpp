///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/Yoga/RectangleButton.hpp"

#include "Slic3r/App/Imgui/ImguiExtension.hpp"
#include "Slic3r/App/Yoga/Rectangle.hpp"

namespace Slic3r::App::Yoga {

RectangleButton::RectangleButton(const std::string& tooltip) : AbstractButton(tooltip)
{
    std::unique_ptr<Rectangle> rect = std::make_unique<Rectangle>();
    m_background = rect.get();
    AbstractButton::insert(std::move(rect), 0);
    m_background->set_padding(4);
    m_background->set_justify_content(YGJustifyCenter);
    m_background->set_gap(5);
    m_background->set_flex_grow(1);
    m_background->set_disabled_fill(IM_COL32_DISABLE);

    set_background_color_checked(ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
    update_fill();
}

void RectangleButton::append(ItemPtr child) { m_background->append(std::move(child)); }

void RectangleButton::insert(ItemPtr child, size_t index)
{
    m_background->insert(std::move(child), index);
}

ItemPtr RectangleButton::remove(Item* child) { return m_background->remove(child); }

const ImColor& RectangleButton::background_color() const { return m_background->fill(); }

void RectangleButton::set_background_color(const ImColor& color)
{
    m_background_color = color;
    if (color.Value.w == 0.f)
        m_background_color_hover = ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered);
    else
        m_background_color_hover = Imgui::adjust_brightness(m_background_color, 1.2);
    update_fill();
}

const Paddings& RectangleButton::content_padding() { return m_background->padding(); }

void RectangleButton::set_content_padding(const Paddings& padding)
{
    m_background->set_padding(padding);
}

Orientation RectangleButton::content_orientation() const { return m_background->orientation(); }

void RectangleButton::set_content_orientation(Orientation orientation)
{
    m_background->set_orientation(orientation);
}

YGJustify RectangleButton::content_justify_content() const
{
    return m_background->justify_content();
}

YGAlign RectangleButton::content_align_item() const { return m_background->align_items(); }

void RectangleButton::set_content_align_items(YGAlign align)
{
    m_background->set_align_items(align);
}

YGDirection RectangleButton::content_direction() const
{
    return m_direction;
}

void RectangleButton::set_content_direction(YGDirection direction)
{
    m_background->set_direction(direction);
}

const ImColor& RectangleButton::background_color_checked() const
{
    return m_background_color_checked;
}

void RectangleButton::set_background_color_checked(const ImColor& background_color_checked)
{
    m_background_color_checked = background_color_checked;
    m_background_color_checked_hover = Imgui::adjust_brightness(m_background_color_checked, 1.25);
    update_fill();
}

void RectangleButton::set_content_justify_content(YGJustify justify)
{
    m_background->set_justify_content(justify);
}

float RectangleButton::rounding() const { return m_background->rounding(); }

void RectangleButton::set_rounding(float rounding) { m_background->set_rounding(rounding); }

ImDrawFlags RectangleButton::draw_flags() const { return m_background->flags(); }

void RectangleButton::set_draw_flags(ImDrawFlags draw_flags)
{
    m_background->set_flags(draw_flags);
}

void RectangleButton::checked_updated_internal() { update_fill(); }

void RectangleButton::hovered_updated_internal() { update_fill(); }

void RectangleButton::update_fill()
{
    ImColor color = m_background_color;
    if (checked()) {
        color = hovered() ? m_background_color_checked_hover : m_background_color_checked;
    } else {
        color = hovered() ? m_background_color_hover : m_background_color;
    }
    m_background->set_fill(color);
}

} // namespace Slic3r::App::Yoga
