///|/ Copyright (c) Prusa Research 2018 - 2025 Oleksandra Iushchenko @YuSanka, Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/Yoga/Toolbar.hpp"

#include "Slic3r/App/Yoga/IconButton.hpp"
#include "Slic3r/App/Yoga/Item.hpp"

#include <utility>

namespace Slic3r::App::Yoga {

Toolbar::Toolbar(const std::string& name, Item* parent) : Window(name, parent) {
    set_padding(0);
}

void Toolbar::append(IconButton* button)
{
    if (!contains(button)) {
        button->set_min_size(m_button_min_size);
        button->set_max_size(m_button_max_size);
        button->set_aspect_ratio(m_button_aspect_ratio);
        m_buttons.emplace_back(button);
        Item::append(button);
    }
}

void Toolbar::remove(IconButton* button)
{
    if (contains(button)) {
        m_buttons.erase(m_buttons.cbegin() + index_of(button));
        Item::remove(button);
    }
}

IconButton* Toolbar::button_at(int index) const { return m_buttons.at(index); }

int Toolbar::button_count() const { return m_buttons.size(); }

bool Toolbar::contains(IconButton* button) const { return index_of(button) != -1; }

const Vec2f& Toolbar::button_min_size() const { return m_button_min_size; }

void Toolbar::set_button_min_size(const Vec2f& button_min_size)
{
    if (m_button_min_size != button_min_size) {
        m_button_min_size = button_min_size;
        for (IconButton* button : std::as_const(m_buttons)) {
            button->set_min_size(button_min_size);
        }
    }
}

const Vec2f& Toolbar::button_max_size() const { return m_button_max_size; }

void Toolbar::set_button_max_size(const Vec2f& button_max_size)
{
    if (m_button_max_size != button_max_size) {
        m_button_max_size = button_max_size;
        for (IconButton* button : std::as_const(m_buttons)) {
            button->set_max_size(button_max_size);
        }
    }
}

float Toolbar::button_aspect_ratio() const { return m_button_aspect_ratio; }

void Toolbar::set_button_aspect_ratio(float button_aspect_ratio)
{
    if (m_button_aspect_ratio != button_aspect_ratio) {
        m_button_aspect_ratio = button_aspect_ratio;
        for (IconButton* button : std::as_const(m_buttons)) {
            button->set_aspect_ratio(button_aspect_ratio);
        }
    }
}

void Toolbar::append(Item* child) { Item::append(child); }

void Toolbar::insert(Item* child, size_t index) { Item::insert(child, index); }

void Toolbar::remove(Item* child) { Item::remove(child); }

int Toolbar::index_of(IconButton* button) const
{
    std::vector<IconButton*>::const_iterator it =
        std::find_if(m_buttons.cbegin(), m_buttons.cend(), [button](IconButton* child_button) {
            return button == child_button;
        });

    return it != m_buttons.cend() ? m_buttons.cbegin() - it : -1;
}

} // namespace Slic3r::App::Yoga
