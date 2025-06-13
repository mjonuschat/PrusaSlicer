///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/Yoga/AbstractButton.hpp"

#include "Slic3r/App/Yoga/Tooltip.hpp"

#include <imgui_internal.h>

namespace Slic3r::App::Yoga {

AbstractButton::Callbacks& AbstractButton::callbacks() { return m_callbacks; }

AbstractButton::AbstractButton(const std::string& tooltip, const std::string& name) : Item()
{
    set_item_name(name);
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
        pressed_updated_internal();
    }

    Item::process_events(pos, size);
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

    if (m_enabled) {
        bool pressed = false;
        // We ignore pressed and hovered here, we already catched them before
        ImGui::ButtonBehavior(button_bb, id, nullptr, &pressed, m_flags);

        ImGui::RenderNavCursor(button_bb, id);

        set_pressed(pressed);
    }

    render_item_end(pos, size);
}

const std::string& AbstractButton::shortcut() const { return m_shortcut; }

void AbstractButton::set_shortcut(const std::string& shortcut)
{
    m_shortcut = shortcut;
    m_tooltip->set_shortcut(m_shortcut);
}

void AbstractButton::set_tooltip(const std::string& tooltip) { m_tooltip->set_text(tooltip); }

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
        if (m_callbacks.hovered_changed) {
            m_callbacks.hovered_changed(m_hovered);
        }
        if (!m_tooltip->text().empty())
            m_tooltip->set_visible(m_hovered);
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

bool AbstractButton::pressed() const { return m_pressed; }

void AbstractButton::enabled_updated_internal()
{
    if (!m_enabled)
        m_tooltip->set_visible(false);
}

} // namespace Slic3r::App::Yoga
