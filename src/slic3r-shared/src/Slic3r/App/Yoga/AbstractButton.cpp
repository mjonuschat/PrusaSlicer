///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/Yoga/AbstractButton.hpp"

#include "Slic3r/App/Yoga/Tooltip.hpp"

#include <imgui_internal.h>

namespace Slic3r::App::Yoga {

AbstractButton::Callbacks& AbstractButton::callbacks() { return m_callbacks; }

AbstractButton::AbstractButton(const std::string& tooltip) : Item()
{
    static size_t button_tooltip_number = 0;

    m_tooltip = emplace_back<
        Tooltip>("button_tooltip_" + std::to_string(button_tooltip_number++), tooltip, "");
    m_tooltip->set_visible(false);
}

void AbstractButton::process_events(Vec2f pos, Vec2f size)
{
    ImRect button_bb(to_im(pos), to_im(pos + size));

    // Check if the button is clicked or hovered
    bool hovered = ImGui::IsMouseHoveringRect(button_bb.Min, button_bb.Max, false);
    bool pressed = m_enabled && hovered && ImGui::IsMouseClicked(0);

    set_hovered(hovered);

    if (pressed) {
        if (m_checkable) {
            set_checked(!m_checked);
        }
        if (m_callbacks.action) {
            m_callbacks.action();
        }
    }

    Item::process_events(pos, size);
}

const std::string& AbstractButton::shortcut() const { return m_shortcut; }

void AbstractButton::set_shortcut(const std::string& shortcut)
{
    m_shortcut = shortcut;
    m_tooltip->set_shortcut(m_shortcut);
}

bool AbstractButton::has_arrow() const { return m_has_arrow; }

void AbstractButton::set_has_arrow(bool has_arrow) { m_has_arrow = has_arrow; }

bool AbstractButton::enabled() const { return m_enabled; }

void AbstractButton::set_enabled(bool enabled) { m_enabled = enabled; }

bool AbstractButton::checkable() const { return m_checkable; }

void AbstractButton::set_checkable(bool checkable) { m_checkable = checkable; }

bool AbstractButton::checked() const { return m_checked; }

void AbstractButton::set_checked(bool checked)
{
    if (m_checked != checked) {
        m_checked = checked;
        checked_updated_internal();
        if (m_callbacks.checked_changed) {
            m_callbacks.checked_changed(m_checked);
        }
    }
}

bool AbstractButton::hovered() const { return m_hovered; }

void AbstractButton::set_hovered(bool hovered)
{
    if (m_hovered != hovered) {
        m_hovered = hovered;
        hovered_updated_internal();
        if (m_callbacks.hovered_changed) {
            m_callbacks.hovered_changed(m_hovered);
        }
        if (!m_tooltip->text().empty())
            m_tooltip->set_visible(m_hovered);
    }
}

} // namespace Slic3r::App::Yoga
