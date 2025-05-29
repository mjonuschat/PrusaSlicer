///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/Yoga/ButtonGroup.hpp"

#include "Slic3r/App/Yoga/AbstractButton.hpp"

namespace Slic3r::App::Yoga {

ButtonGroup::ButtonGroup(std::initializer_list<AbstractButton*> initializer_list)
{
    set_buttons(initializer_list);
}

ButtonGroup::Callbacks& ButtonGroup::callbacks() { return m_callbacks; }

void ButtonGroup::set_buttons(std::initializer_list<AbstractButton*> initializer_list)
{
    for (AbstractButton* button : std::as_const(m_buttons)) {
        unset_button_callbacks(button);
    }

    m_buttons = initializer_list;

    for (AbstractButton* button : std::as_const(m_buttons)) {
        set_button_callbacks(button);
    }
}

AbstractButton* ButtonGroup::checked_button() const { return m_checked_button; }

void ButtonGroup::insert_button(AbstractButton* button)
{
    ASSERT(button);
    if (m_buttons.contains(button)) {
        return;
    }

    m_buttons.insert(button);

    set_button_callbacks(button);
}

bool ButtonGroup::remove_button(AbstractButton* button)
{
    if (!m_buttons.contains(button)) {
        return false;
    }

    button->callbacks().action = nullptr;
    button->callbacks().checked_changed = nullptr;

    m_buttons.erase(button);

    return true;
}

size_t ButtonGroup::button_count() const { return m_buttons.size(); }

void ButtonGroup::on_button_action(AbstractButton* button)
{
    if (m_callbacks.action) {
        m_callbacks.action(button);
    }
}

void ButtonGroup::on_button_checked(AbstractButton* button)
{
    if (m_checked_blocker) {
        return;
    }

    m_checked_blocker = true;

    AbstractButton* last = m_checked_button;
    m_checked_button = button;
    if (m_callbacks.checked_changed) {
        m_callbacks.checked_changed(button, last);
    }

    for (AbstractButton* owned_button : std::as_const(m_buttons)) {
        if (owned_button != button) {
            owned_button->set_checked(false);
        }
    }

    m_checked_blocker = false;
}

void ButtonGroup::set_button_callbacks(AbstractButton* button)
{
    button->callbacks().action = [this, button]() { on_button_action(button); };
    button->callbacks().checked_changed = [this, button](bool checked) {
        if (checked) {
            on_button_checked(button);
        }
    };
}

void ButtonGroup::unset_button_callbacks(AbstractButton *button)
{
    button->callbacks().action = nullptr;
    button->callbacks().checked_changed = nullptr;
}

} // namespace Slic3r::App::Yoga
