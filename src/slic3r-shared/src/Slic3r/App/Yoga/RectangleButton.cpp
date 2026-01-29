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
    m_background                    = rect.get();
    AbstractButton::insert(std::move(rect), 0);
    m_background->set_padding(4);
    m_background->set_justify_content(YGJustifyCenter);
    m_background->set_gap(5);
    m_background->set_flex_grow(1);
    m_background->set_disabled_fill(IM_COL32_DISABLE);
    m_background->set_flex_shrink(0);

    set_background_color_checked(ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
    update_colors();
}

void RectangleButton::append(ObjectPtr child)
{
    m_background->append(std::move(child));
}

void RectangleButton::insert(ObjectPtr child, size_t index)
{
    m_background->insert(std::move(child), index);
}

ObjectPtr RectangleButton::remove(Object* child)
{
    return m_background->remove(child);
}

const ImColor& RectangleButton::background_color() const
{
    return m_background->fill();
}

void RectangleButton::set_background_color(const ImColor& color, bool adjust_hover)
{
    m_background_color = color;
    if (adjust_hover && color.Value.w == 0.f)
        m_background_color_hover = ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered);
    else {
        m_background_color_hover =
            adjust_hover ? Imgui::adjust_brightness(m_background_color, 1.2f) : m_background_color;
        m_background_color_disabled = Imgui::adjust_brightness(m_background_color, 0.85f);
        m_background->set_disabled_fill(m_background_color_disabled);
    }
    update_colors();
}

const ImColor& RectangleButton::background_color_checked() const
{
    return m_background_color_checked;
}

void RectangleButton::set_background_color_checked(
    const ImColor& background_color_checked,
    bool adjust_hover
)
{
    m_background_color_checked       = background_color_checked;
    m_background_color_checked_hover = adjust_hover ?
        Imgui::adjust_brightness(m_background_color_checked, 1.25f) :
        m_background_color_checked;
    m_background_color_checked_disabled =
        Imgui::adjust_brightness(m_background_color_checked, 0.85f);
    update_colors();
}

const ImColor& RectangleButton::background_color_border() const
{
    return m_background_color_border;
}

void RectangleButton::set_background_color_border(
    const ImColor& background_color_border,
    bool adjust_hover
)
{
    m_background_color_border          = background_color_border;
    m_background_color_border_hover    = adjust_hover ?
           Imgui::adjust_brightness(m_background_color_border, 1.25f) :
           m_background_color_border;
    m_background_color_border_disabled = Imgui::adjust_brightness(m_background_color_border, 0.85f);
    update_colors();
}

const Paddings& RectangleButton::content_padding()
{
    return m_background->padding();
}

void RectangleButton::set_content_padding(const Paddings& padding)
{
    m_background->set_padding(padding);
}

Orientation RectangleButton::content_orientation() const
{
    return m_background->orientation();
}

void RectangleButton::set_content_orientation(Orientation orientation)
{
    m_background->set_orientation(orientation);
}

YGJustify RectangleButton::content_justify_content() const
{
    return m_background->justify_content();
}

YGAlign RectangleButton::content_align_item() const
{
    return m_background->align_items();
}

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

float RectangleButton::background_border_width() const
{
    return m_background->border_width();
}

void RectangleButton::set_background_border_width(float width)
{
    m_background->set_border_width(width);
}

void RectangleButton::set_content_justify_content(YGJustify justify)
{
    m_background->set_justify_content(justify);
}

float RectangleButton::rounding() const
{
    return m_background->rounding();
}

void RectangleButton::set_rounding(float rounding)
{
    m_background->set_rounding(rounding);
}

ImDrawFlags RectangleButton::draw_flags() const
{
    return m_background->flags();
}

void RectangleButton::set_draw_flags(ImDrawFlags draw_flags)
{
    m_background->set_flags(draw_flags);
}

void RectangleButton::checked_updated_internal()
{
    update_colors();
    m_background->set_disabled_fill(
        checked() ? m_background_color_checked_disabled : m_background_color
    );
}

void RectangleButton::hovered_updated_internal()
{
    update_colors();
}

void RectangleButton::update_colors()
{
    ImColor fill_color   = m_background_color;
    ImColor border_color = hovered() ? m_background_color_border_hover : m_background_color_border;
    if (checked()) {
        fill_color = hovered() ? m_background_color_checked_hover : m_background_color_checked;
    } else {
        fill_color = hovered() ? m_background_color_hover : m_background_color;
    }
    m_background->set_fill(fill_color);
    m_background->set_border_color(border_color);
}

} // namespace Slic3r::App::Yoga
