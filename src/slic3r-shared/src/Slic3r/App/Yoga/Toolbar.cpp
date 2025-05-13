///|/ Copyright (c) Prusa Research 2018 - 2025 Oleksandra Iushchenko @YuSanka, Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/Yoga/Toolbar.hpp"

#include "Slic3r/App/Yoga/ToolbarButton.hpp"
#include "Slic3r/App/Yoga/Item.hpp"
#include "Slic3r/Assert.hpp"
#include "Slic3r/Log.hpp"

#include <utility>
#include <imgui_internal.h>

namespace Slic3r::App::Yoga {

Toolbar::Toolbar(const std::string& name, Item* parent) : Window(name, parent)
{
    set_padding(0);
    // Button More is used for collapsible and should never be part of m_buttons
    m_button_more = new ToolbarButton(ImGui::ToolbarEllipsis, "Show more");
    m_button_more->set_item_name(name + "_show_more_button");
}

Toolbar::~Toolbar()
{
    if (!m_button_more->parent()) {
        delete m_button_more;
    }
}

void Toolbar::process_events(Vec2f pos, Vec2f size)
{
    ImRect button_bb(to_im(pos), to_im(pos) + to_im(size));

    // Check if the button is clicked or hovered
    bool hovered = ImGui::IsMouseHoveringRect(button_bb.Min, button_bb.Max, false);
    if (m_hovered != hovered) {
        m_hovered = hovered;
        if (m_callbacks.hovered_changed) {
            m_callbacks.hovered_changed();
        }
    }

    Window::process_events(pos, size);
}

Toolbar::Callbacks& Toolbar::callbacks() { return m_callbacks; }

void Toolbar::append(ToolbarButton* button)
{
    if (!contains(button)) {
        button->set_min_size(m_button_min_size);
        button->set_max_size(m_button_max_size);
        button->set_aspect_ratio(m_button_aspect_ratio);
        m_buttons.push_back(button);
        Item::append(button);
    }
}

void Toolbar::remove(ToolbarButton* button)
{
    if (contains(button)) {
        int index = index_of(button);
        ASSERT(index != -1);
        m_buttons.erase(m_buttons.cbegin() + index);
        Item::remove(button);
    }
}

void Toolbar::clear()
{
    const std::vector<ToolbarButton*> buttons = m_buttons;
    for (ToolbarButton* button : buttons) {
        remove(button);
    }
    ASSERT(m_buttons.empty());
}

void Toolbar::set(const std::vector<ToolbarButton*>& buttons)
{
    if (m_buttons == buttons) {
        return;
    }

    clear();

    for (ToolbarButton* button : std::as_const(buttons)) {
        append(button);
    }
}

ToolbarButton* Toolbar::button_at(int index) const { return m_buttons.at(index); }

int Toolbar::button_count() const { return m_buttons.size(); }

bool Toolbar::contains(ToolbarButton* button) const { return index_of(button) != -1; }

const Vec2f& Toolbar::button_min_size() const { return m_button_min_size; }

void Toolbar::set_button_min_size(const Vec2f& button_min_size)
{
    if (m_button_min_size != button_min_size) {
        m_button_min_size = button_min_size;
        for (ToolbarButton* button : std::as_const(m_buttons)) {
            button->set_min_size(button_min_size);
        }
        m_button_more->set_min_size(button_min_size);
    }
}

const Vec2f& Toolbar::button_max_size() const { return m_button_max_size; }

void Toolbar::set_button_max_size(const Vec2f& button_max_size)
{
    if (m_button_max_size != button_max_size) {
        m_button_max_size = button_max_size;
        for (ToolbarButton* button : std::as_const(m_buttons)) {
            button->set_max_size(button_max_size);
        }
        m_button_more->set_max_size(button_max_size);
    }
}

float Toolbar::button_aspect_ratio() const { return m_button_aspect_ratio; }

void Toolbar::set_button_aspect_ratio(float button_aspect_ratio)
{
    if (m_button_aspect_ratio != button_aspect_ratio) {
        m_button_aspect_ratio = button_aspect_ratio;
        for (ToolbarButton* button : std::as_const(m_buttons)) {
            button->set_aspect_ratio(button_aspect_ratio);
        }
        m_button_more->set_aspect_ratio(button_aspect_ratio);
    }
}

void Toolbar::append(Item* child) { Item::append(child); }

void Toolbar::insert(Item* child, size_t index) { Item::insert(child, index); }

void Toolbar::remove(Item* child) { Item::remove(child); }

bool Toolbar::hovered() const { return m_hovered; }

bool Toolbar::show_tooltips() const { return m_show_tooltips; }

void Toolbar::set_show_tooltips(bool show_tooltips) { m_show_tooltips = show_tooltips; }

bool Toolbar::any_subtoolbar_opened() const
{
    return (m_button_more->subtoolbar() && m_button_more->subtoolbar()->is_visible()) ||
        std::any_of(m_buttons.cbegin(), m_buttons.cend(), [](ToolbarButton* button) {
               return button->subtoolbar() && button->subtoolbar()->is_visible();
           });
}

bool Toolbar::collapsible() const { return m_collapsible; }

void Toolbar::set_collapsible(bool collapsible) { m_collapsible = collapsible; }

float Toolbar::available_size() const { return m_available_size; }

void Toolbar::set_available_size(float available_size) { m_available_size = available_size; }

void Toolbar::style_node()
{
    if (m_collapsible && m_parent && !m_button_min_size.isZero() && width() > 0 && height() > 0) {
        // Compute available size
        float available_size = 0;
        available_size = m_orientation == Orientation::Horizontal
            ? m_parent->width()
            : m_parent->height();
        for (Item* node : m_parent->items()) {
            if (node != this) {
                available_size -= m_orientation == Orientation::Horizontal
                    ? node->width()
                    : node->height();
            }
            if (node != *m_parent->items().rbegin()) {
                available_size -= m_parent->gap();
            }
        }
        available_size -= m_orientation == Orientation::Horizontal
            ? (m_margin.left + m_margin.right)
            : (m_margin.top + m_margin.bottom);

        // Decide which buttons will be included

        // Assume button size
        const float button_size = m_button_min_size.x();
        // Take away size from collapsed button
        available_size -= button_size;

        std::vector<ToolbarButton*> included_buttons;
        std::vector<ToolbarButton*> collapsed_buttons;
        // for (std::vector<ToolbarButton*>::const_reverse_iterator it = m_buttons.crbegin(); it != m_buttons.crend(); ++it) {
        for (ToolbarButton* button : std::as_const(m_buttons)) {
            // ToolbarButton* button = *it;
            if (!button->is_visible()) {
                included_buttons.push_back(button);
                continue;
            }

            available_size -= button_size;
            if (available_size > 0) {
                included_buttons.push_back(button);
            } else {
                collapsed_buttons.push_back(button);
            }

            if (button != *m_buttons.rbegin()) {
                available_size -= m_gap;
            }
        }

        m_button_more->set_orientation(orientation());
        m_button_more->set_subtoolbar_buttons(collapsed_buttons);
        m_button_more->set_parent(collapsed_buttons.empty() ? nullptr : this);

        size_t index = item_count();
        if (m_button_more->parent()) {
            index--;
        }
        for (ToolbarButton* button : std::as_const(included_buttons)) {
            // check and reparent every included button
            if (button->parent() != this) {
                button->set_parent(this, index++);
            }
        }
    }

    Item::style_node();
}

int Toolbar::index_of(ToolbarButton* button) const
{
    std::vector<ToolbarButton*>::const_iterator it =
        std::find_if(m_buttons.cbegin(), m_buttons.cend(), [button](ToolbarButton* child_button) {
            return button == child_button;
        });

    return it != m_buttons.cend() ? m_buttons.cbegin() - it : -1;
}

} // namespace Slic3r::App::Yoga
