///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/Yoga/AbstractButton.hpp"

#include "Slic3r/App/Yoga/Tooltip.hpp"

#include <imgui_internal.h>

namespace Slic3r::App::Yoga {

AbstractButton::Callbacks& AbstractButton::callbacks() { return m_callbacks; }

AbstractButton::AbstractButton(const std::string& tooltip, const std::string& name)
    : Item(), m_tooltip(this, tooltip, "")
{
    set_item_name(name);
}

void AbstractButton::render(Vec2f pos, Vec2f size)
{
    render_item_begin(pos, size);

    ImRect button_bb(to_im(pos), to_im(pos + size));

    ImGuiWindow* window = ImGui::GetCurrentWindow();
    const std::string label = "###" + m_item_name;
    const ImGuiID id = window->GetID(label.c_str());

    ImGui::ItemSize(to_im(size), 0);
    if (!ImGui::ItemAdd(button_bb, id)) {
        return;
    }

    bool pressed = false;
    bool hovered = false;
    bool released = false;

    if (enabled()) {
        // We ignore pressed and hovered here, we already catched them before
        released = ImGui::ButtonBehavior(button_bb, id, &hovered, &pressed, m_flags);

        ImGui::RenderNavCursor(button_bb, id);
    }

    set_hovered(hovered);

    if (released) {
        if (m_checkable) {
            set_checked(!m_checked);
        }
        if (m_callbacks.action) {
            m_callbacks.action();
        }
        pressed_updated_internal();
    }

    set_pressed(pressed);

    render_item_end(pos, size);
}

const std::string& AbstractButton::shortcut() const { return m_shortcut; }

void AbstractButton::set_shortcut(const std::string& shortcut)
{
    m_shortcut = shortcut;
    m_tooltip.set_shortcut(m_shortcut);
}

void AbstractButton::set_tooltip(const std::string& tooltip) { m_tooltip.set_text(tooltip); }

void AbstractButton::set_tooltip_position(Yoga::Position position)
{
    m_tooltip.set_preferred_position(position);
}

bool AbstractButton::has_arrow() const { return m_has_arrow; }

void AbstractButton::set_has_arrow(bool has_arrow) { m_has_arrow = has_arrow; }

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
        if (!m_tooltip.text().empty()) {
            m_hovered ? m_tooltip.open() : m_tooltip.close();
        }
        if (m_callbacks.hovered_changed) {
            m_callbacks.hovered_changed(m_hovered);
        }
    }
}

void AbstractButton::set_pressed(bool pressed)
{
    if (m_pressed != pressed) {
        m_pressed = pressed;
        if (m_callbacks.pressed_changed) {
            m_callbacks.pressed_changed(m_pressed);
        }
    }
}

ImGuiButtonFlags AbstractButton::flags() const { return m_flags; }

void AbstractButton::checked_updated_internal() {}

void AbstractButton::hovered_updated_internal() {}

void AbstractButton::pressed_updated_internal() {}

bool AbstractButton::pressed() const { return m_pressed; }

void AbstractButton::enabled_updated_internal()
{
    if (!enabled()) {
        m_tooltip.close();
    }
}

void AbstractButton::visible_updated_internal()
{
    if (!is_visible()) {
        m_tooltip.close();
    }
}

} // namespace Slic3r::App::Yoga
