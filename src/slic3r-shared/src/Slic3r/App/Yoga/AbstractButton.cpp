///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/Yoga/AbstractButton.hpp"

#include "Slic3r/App/Yoga/Tooltip.hpp"

#include <imgui_internal.h>

namespace Slic3r::App::Yoga {

AbstractButton::Callbacks& AbstractButton::callbacks()
{
    return m_callbacks;
}

AbstractButton::AbstractButton(const std::string& tooltip, const std::string& name)
{
    m_tooltip = emplace_back<Tooltip>(this, tooltip, std::string{});
    set_object_name(name.empty() ? "Button" : name);
}

void AbstractButton::render(Vec2f pos, Vec2f size)
{
    render_item_begin(pos, size);

    ImGui::SetCursorScreenPos(to_im(pos));

    ImGui::PushID(object_name().c_str());
    if (m_allow_overlap) {
        ImGui::SetNextItemAllowOverlap();
    }

    bool pressed = ImGui::InvisibleButton("##btn", to_im(size.cwiseMax(10)), m_flags);
    bool hovered = ImGui::IsItemHovered() && ImGui::IsItemVisible();
    bool held    = ImGui::IsItemActive();

    bool primary_pressed   = pressed && ImGui::IsMouseReleased(m_primary_button);
    bool secondary_pressed = pressed && ImGui::IsMouseReleased(m_secondary_button);

    bool primary_held   = held && ImGui::IsMouseDown(m_primary_button);
    bool secondary_held = held && ImGui::IsMouseDown(m_secondary_button);

    ImGui::PopID();

    if (enabled()) {
        set_hovered(hovered);
        set_pressed_primary(primary_held);
        set_pressed_secondary(secondary_held);

        if (m_hovered || held) {
            ImGui::SetMouseCursor(m_cursor);
        }

        if (primary_pressed) {
            if (m_checkable) {
                set_checked(!m_checked);
            }
            if (m_callbacks.action) {
                m_callbacks.action();
            }
            action_internal();
        }
        if (secondary_pressed) {
            if (m_callbacks.secondary_action) {
                m_callbacks.secondary_action();
            }
            secondary_action_internal();
        }
    }

    render_item_end(pos, size);
}

const std::string& AbstractButton::shortcut() const
{
    return m_shortcut;
}

void AbstractButton::set_shortcut(const std::string& shortcut)
{
    if (m_shortcut != shortcut) {
        m_shortcut = shortcut;
        m_tooltip->set_shortcut(m_shortcut);
        set_shortcut_internal(m_shortcut);
    }
}

void AbstractButton::set_tooltip(const std::string& tooltip)
{
    m_tooltip->set_text(tooltip);
}

void AbstractButton::set_tooltip_position(Yoga::Position position)
{
    m_tooltip->set_preferred_position(position);
}

bool AbstractButton::has_arrow() const
{
    return m_has_arrow;
}

void AbstractButton::set_has_arrow(bool has_arrow)
{
    m_has_arrow = has_arrow;
}

bool AbstractButton::checkable() const
{
    return m_checkable;
}

void AbstractButton::set_checkable(bool checkable)
{
    m_checkable = checkable;
}

bool AbstractButton::checked() const
{
    return m_checked;
}

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

bool AbstractButton::hovered() const
{
    return m_hovered;
}

void AbstractButton::set_hovered(bool hovered)
{
    if (!is_visible()) {
        hovered = false; // force not hovered while invisible
    }

    if (m_hovered != hovered) {
        m_hovered = hovered;
        hovered_updated_internal();
        if (!m_tooltip->text().empty()) {
            m_hovered&& is_visible() ? m_tooltip -> open() : m_tooltip->close();
        }
        if (m_callbacks.hovered_changed) {
            m_callbacks.hovered_changed(m_hovered);
        }
    }
}

bool AbstractButton::pressed() const
{
    return m_pressed_primary;
}

void AbstractButton::set_pressed_primary(bool pressed)
{
    if (m_pressed_primary != pressed) {
        m_pressed_primary = pressed;
        pressed_primary_updated_internal();
        if (m_callbacks.pressed_primary_changed) {
            m_callbacks.pressed_primary_changed(pressed);
        }
    }
}

void AbstractButton::set_pressed_secondary(bool pressed)
{
    if (m_pressed_secondary != pressed) {
        m_pressed_secondary = pressed;
        pressed_secondary_updated_internal();
        if (m_callbacks.pressed_secondary_changed) {
            m_callbacks.pressed_secondary_changed(pressed);
        }
    }
}

void AbstractButton::update_flags()
{
    auto convert = [](ImGuiMouseButton button) -> ImGuiButtonFlags
    {
        switch (button) {
        case ImGuiMouseButton_Right:
            return ImGuiButtonFlags_MouseButtonRight;
        case ImGuiMouseButton_Middle:
            return ImGuiButtonFlags_MouseButtonMiddle;
        case ImGuiMouseButton_Left:
        default:
            return ImGuiButtonFlags_MouseButtonLeft;
        }
    };

    m_flags = ImGuiButtonFlags_EnableNav | convert(m_primary_button) | convert(m_secondary_button);
}

ImGuiMouseButton AbstractButton::primary_button() const
{
    return m_primary_button;
}

void AbstractButton::set_primary_button(ImGuiMouseButton primary_button)
{
    if (m_pressed_primary != primary_button) {
        m_primary_button = primary_button;
        update_flags();
    }
}

ImGuiMouseButton AbstractButton::secondary_button() const
{
    return m_secondary_button;
}

void AbstractButton::set_secondary_button(ImGuiMouseButton secondary_button)
{
    if (m_secondary_button != secondary_button) {
        m_secondary_button = secondary_button;
        update_flags();
    }
}

ImGuiMouseCursor AbstractButton::cursor() const
{
    return m_cursor;
}

void AbstractButton::set_cursor(ImGuiMouseCursor cursor)
{
    m_cursor = cursor;
}

bool AbstractButton::allow_overlap() const
{
    return m_allow_overlap;
}

void AbstractButton::set_allow_overlap(bool allow_overlap)
{
    m_allow_overlap = allow_overlap;
}

void AbstractButton::checked_updated_internal() {}

void AbstractButton::hovered_updated_internal() {}

void AbstractButton::pressed_primary_updated_internal() {}

void AbstractButton::pressed_secondary_updated_internal() {}

void AbstractButton::action_internal() {}

void AbstractButton::secondary_action_internal() {}

void AbstractButton::set_shortcut_internal(const std::string& shortcut) {}

void AbstractButton::enabled_updated_internal()
{
    if (!enabled()) {
        m_tooltip->close();
    }
}

void AbstractButton::visible_updated_internal()
{
    if (!is_visible()) {
        m_tooltip->close();
    }
}

Platform::ColorGroup AbstractButton::button_color_group() const
{
    if (enabled()) {
        if (m_checked) {
            return Platform::ColorGroup::Active;
        } else if (m_hovered) {
            return Platform::ColorGroup::Hovered;
        } else {
            return Platform::ColorGroup::Default;
        }
    } else {
        return m_checked ? Platform::ColorGroup::ActiveDisabled : Platform::ColorGroup::Disabled;
    }
}

} // namespace Slic3r::App::Yoga
