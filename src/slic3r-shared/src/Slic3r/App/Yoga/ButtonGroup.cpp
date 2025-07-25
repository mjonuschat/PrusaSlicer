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
    m_checked_button = nullptr;
    for (AbstractButton* button : std::as_const(m_buttons)) {
        unset_button_callbacks(button);
    }

    m_buttons = initializer_list;

    for (AbstractButton* button : std::as_const(m_buttons)) {
        ASSERT(button);
        ASSERT(button->checkable());

        set_button_callbacks(button);
        if (button->checked()) {
            ASSERT(m_checked_button == nullptr, "Multiple checked buttons provided!");
            button->set_checked(true);
        }
    }

    if (!m_buttons.empty() && m_checked_button == nullptr) {
        (*initializer_list.begin())->set_checked(true);
    }
}

AbstractButton* ButtonGroup::checked_button() const { return m_checked_button; }

void ButtonGroup::insert_button(AbstractButton* button)
{
    ASSERT(button);
    ASSERT(button->checkable());
    if (m_buttons.contains(button)) {
        return;
    }

    m_buttons.insert(button);

    set_button_callbacks(button);


    if (m_checked_button == nullptr || button->checked()) {
        check_one_button(button);
    }
}

bool ButtonGroup::remove_button(AbstractButton* button)
{
    ASSERT(!button->checked(), "New checked button wont be implicitly picked!");

    if (!m_buttons.contains(button)) {
        return false;
    }

    button->callbacks().action = nullptr;
    button->callbacks().checked_changed = nullptr;

    m_buttons.erase(button);

    return true;
}

size_t ButtonGroup::button_count() const { return m_buttons.size(); }

const ButtonGroup::Buttons& ButtonGroup::buttons() const { return m_buttons; }

void ButtonGroup::on_button_action(AbstractButton* button)
{
    if (m_callbacks.action) {
        m_callbacks.action(button);
    }
}

void ButtonGroup::check_one_button(AbstractButton* button) {
    m_checked_blocker = true;
    m_checked_button = button;
    for (AbstractButton* owned_button : std::as_const(m_buttons)) {
        if (owned_button != button) {
            owned_button->set_checked(false);
        } else {
            owned_button->set_checked(true);
        }
    }
    m_checked_blocker = false;
}

void ButtonGroup::on_button_checked(AbstractButton* button)
{
    if (m_checked_blocker) {
        return;
    }

    AbstractButton* last = m_checked_button;
    if (m_callbacks.checked_changed) {
        m_callbacks.checked_changed(button, last);
    }
    check_one_button(button);
}

void ButtonGroup::set_button_callbacks(AbstractButton* button)
{
    button->callbacks().action = [this, button]() { on_button_action(button); };
    button->callbacks().checked_changed = [this, button](bool) {
        // Intentionally ignored checked state.
        // The button cannot be unchecked if it is already checked.
        on_button_checked(button);
    };
}

void ButtonGroup::unset_button_callbacks(AbstractButton* button)
{
    button->callbacks().action = nullptr;
    button->callbacks().checked_changed = nullptr;
}

} // namespace Slic3r::App::Yoga
