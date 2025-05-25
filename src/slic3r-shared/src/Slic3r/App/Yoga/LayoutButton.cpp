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
    : LayoutButton(label, Render::Icon::None)
{}

LayoutButton::LayoutButton(const std::string& label, Render::Icon icon)
    : LayoutButton(label, icon, "")
{}

Slic3r::App::Yoga::LayoutButton::LayoutButton(
    const std::string& label, Render::Icon icon, const std::string& tooltip
)
    : AbstractButton(tooltip)
{
    set_orientation(Orientation::Horizontal);

    m_background = emplace_back<Rectangle>();
    m_background->set_padding(4);
    m_background->set_justify_content(YGJustifyCenter);
    m_background->set_gap(5);
    m_background->set_flex_grow(1);

    m_icon = m_background->emplace_back<Icon>(icon);
    m_icon->set_visible(icon != Render::Icon::None);
    m_icon->set_aspect_ratio(1);

    m_text = m_background->emplace_back<Text>(label);
    m_text->set_self_align(YGAlign::YGAlignCenter);
    m_text->set_visible(!label.empty());

    set_background_color(ImGui::GetStyleColorVec4(ImGuiCol_Button));
    set_background_color_checked(ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));

    update_fill();
}

Item* LayoutButton::insert_into_content(std::unique_ptr<Item> item, std::optional<size_t> index)
{
    size_t id = index.value_or(m_background->item_count());
    m_background->insert(std::move(item), id);
    return m_background->items()[id];
}

void LayoutButton::checked_updated_internal() { update_fill(); }

void LayoutButton::hovered_updated_internal() { update_fill(); }

void LayoutButton::update_fill()
{
    ImColor color = m_background_color;
    if (checked()) {
        color = hovered() ? m_background_color_checked_hover : m_background_color_checked;
    } else {
        color = hovered() ? m_background_color_hover : m_background_color;
    }
    m_background->set_fill(color);
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
    update_fill();
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

const ImColor& LayoutButton::background_color_checked() const { return m_background_color_checked; }

void LayoutButton::set_background_color_checked(const ImColor& background_color_checked)
{
    m_background_color_checked = background_color_checked;
    m_background_color_checked_hover = Imgui::adjust_brightness(m_background_color_checked, 1.25);
    update_fill();
}

void LayoutButton::align_content(Align align)
{
    switch (align) {
    case Align::Left:
        m_background->set_justify_content(YGJustifyFlexStart);
        break;
    case Align::Center:
        m_background->set_justify_content(YGJustifyCenter);
        break;
    case Align::Right:
        m_background->set_justify_content(YGJustifyFlexEnd);
        break;
    default:
        break;
    }
}

void LayoutButton::expand_label(bool expand)
{
    m_text->set_flex_grow(expand ? 1.f : 0.f);
}

Render::Icon LayoutButton::icon() const
{
    return m_icon->icon();
}

void LayoutButton::set_icon(Render::Icon icon)
{
    if (m_icon->icon() != icon) {
        m_icon->set_icon(icon);
        m_icon->set_visible(icon != Render::Icon::None);
    }
}

} // namespace Slic3r::App::Yoga
