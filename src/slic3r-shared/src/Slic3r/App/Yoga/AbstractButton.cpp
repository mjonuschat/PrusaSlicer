#include "Slic3r/App/Yoga/AbstractButton.hpp"

#include "Slic3r/App/Yoga/Tooltip.hpp"

#include <imgui_internal.h>

namespace Slic3r::App::Yoga {

AbstractButton::Callbacks& AbstractButton::callbacks() { return m_callbacks; }

AbstractButton::AbstractButton(wchar_t icon, const std::string& tooltip, Item* parent)
    : Item(parent), m_icon(icon)
{
    static size_t button_tooltip_number = 0;

    m_tooltip = new Tooltip("button_tooltip_" + std::to_string(button_tooltip_number++), tooltip, "", this);
    m_tooltip->set_visible(false);
}

void AbstractButton::render(Vec2f pos, Vec2f size)
{
    ImVec2 button_size = to_im(size);
    ImRect button_bb(to_im(pos), to_im(pos) + button_size);

    // Check if the button is clicked or hovered
    bool hovered = ImGui::IsMouseHoveringRect(button_bb.Min, button_bb.Max);
    bool pressed = m_enabled && hovered && ImGui::IsMouseClicked(0);

    if (pressed) {
        if (m_callbacks.action) {
            m_callbacks.action();
        }
    }

    Item::render(pos, size);
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

void AbstractButton::set_enabled(bool new_enabled) { m_enabled = new_enabled; }

bool AbstractButton::checkable() const { return m_checkable; }

void AbstractButton::set_checkable(bool newCheckable) { m_checkable = newCheckable; }

bool AbstractButton::checked() const { return m_checked; }

void AbstractButton::set_checked(bool newChecked) { m_checked = newChecked; }

} // namespace Slic3r::App::Yoga
