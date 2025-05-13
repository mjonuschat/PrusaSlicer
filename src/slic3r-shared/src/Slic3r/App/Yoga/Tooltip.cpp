///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/Yoga/Tooltip.hpp"

#include "Slic3r/App/Yoga/Text.hpp"
#include "Slic3r/Assert.hpp"

#include "imgui_internal.h"

namespace Slic3r::App::Yoga {

Tooltip::Tooltip(
    const std::string& window_name, const std::string& text, const std::string& shortcut, Item* parent
)
    : Window(window_name, parent)
{
    set_orientation(Orientation::Horizontal);
    set_flags(
        ImGuiWindowFlags_Tooltip | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoMove
    );

    set_gap(5);
    set_padding(4);
    m_text = new Yoga::Text(text, this);
    m_text->set_visible(!text.empty());
    m_shortcut = new Yoga::Text(shortcut, this);
    m_shortcut->set_visible(!shortcut.empty());
    m_shortcut->set_text_color(GImGui->Style.Colors[ImGuiCol_TextDisabled]);

    // positioning
    set_position_type(YGPositionType::YGPositionTypeAbsolute);
}

void Tooltip::style_node()
{
    // make some magic here
    if (is_visible()) {
        switch (m_position) {
        case Position::Right:
            set_right(-(m_offset + width()));
            set_top(m_parent->height() * 0.5 - height() * 0.5);
            break;
        case Position::Left:
            set_left(-(m_offset + width()));
            set_top(m_parent->height() * 0.5 - height() * 0.5);
            break;
        case Position::Top:
            set_top(-(m_offset + width()));
            set_left(m_parent->width() * 0.5 - width() * 0.5);
            break;
        case Position::Bottom:
            set_bottom(-(m_offset + height()));
            set_left(m_parent->width() * 0.5 - width() * 0.5);
            break;
        }
    }

    Window::style_node();
}

const std::string& Tooltip::text() const { return m_text->text(); }

void Tooltip::set_text(const std::string& text)
{
    m_text->set_text(text);
    m_text->set_visible(!text.empty());
}

const std::string& Tooltip::shortcut() const { return m_shortcut->text(); }

void Tooltip::set_shortcut(const std::string& shortcut)
{
    m_shortcut->set_text(shortcut);
    m_shortcut->set_visible(!shortcut.empty());
}

float Tooltip::offset() const { return m_offset; }

void Tooltip::set_offset(float offset) { m_offset = offset; }

Tooltip::Position Tooltip::position() const { return m_position; }

void Tooltip::set_position(Position position)
{
    if (m_position != position) {
        m_position = position;
        set_top(YGUndefined);
        set_bottom(YGUndefined);
        set_left(YGUndefined);
        set_right(YGUndefined);
    }
}

} // namespace Slic3r::App::Yoga
