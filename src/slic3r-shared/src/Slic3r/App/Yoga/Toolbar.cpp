///|/ Copyright (c) Prusa Research 2018 - 2025 Oleksandra Iushchenko @YuSanka, Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/Yoga/Toolbar.hpp"

#include "Slic3r/App/Yoga/ToolbarButton.hpp"
#include "Slic3r/App/Yoga/Item.hpp"

#include <utility>
#include <imgui_internal.h>

namespace Slic3r::App::Yoga {

Toolbar::Toolbar(const std::string& name) : Window(name)
{
    set_padding(4);
    // Button More is used for collapsible and should never be part of m_buttons
    m_button_more = emplace_back<ToolbarButton>(Render::Icon::Ellipsis, "Show more");
    m_button_more->set_object_name(name + "ShowMoreButton");
    m_button_more->set_visible(false);
}

Toolbar::Callbacks& Toolbar::callbacks()
{
    return m_callbacks;
}

void Toolbar::render_body(Vec2f pos, Vec2f size)
{
    ImRect button_bb(to_im(pos), to_im(pos) + to_im(size));

    bool hovered =
        ImGui::IsWindowHovered() && ImGui::IsMouseHoveringRect(button_bb.Min, button_bb.Max, true);
    if (m_hovered != hovered) {
        m_hovered = hovered;
        if (m_callbacks.hovered_changed) {
            m_callbacks.hovered_changed();
        }
    }
}

std::unique_ptr<ToolbarButton> Toolbar::remove(ToolbarButton* button)
{
    if (contains(button)) {
        std::optional<size_t> index = index_of(button);
        ASSERT(index.has_value());
        m_buttons.erase(m_buttons.cbegin() + index.value());
        return unique_dynamic_cast<ToolbarButton>(Item::remove(button));
    } else {
        return nullptr;
    }
}

ToolbarButton* Toolbar::button_at(int index) const
{
    return m_buttons.at(index);
}

int Toolbar::button_count() const
{
    return m_buttons.size();
}

bool Toolbar::contains(ToolbarButton* button) const
{
    return index_of(button).has_value();
}

float Toolbar::button_width() const
{
    return m_button_width;
}

void Toolbar::set_button_width(const float button_width)
{
    if (!Domain::fuzzy_compare(m_button_width, button_width)) {
        m_button_width = button_width;
        for (ToolbarButton* button : std::as_const(m_buttons)) {
            if (m_orientation == Orientation::Horizontal && !button->label().empty()) {
                continue;
            }
            button->set_width(button_width);
        }
        m_button_more->set_width(button_width);
    }
}

float Toolbar::button_height() const
{
    return m_button_height;
}

void Toolbar::set_button_height(float button_height)
{
    if (!Domain::fuzzy_compare(m_button_height, button_height)) {
        m_button_height = button_height;
        for (ToolbarButton* button : std::as_const(m_buttons)) {
            if (m_orientation == Orientation::Vertical && !button->label().empty()) {
                continue;
            }
            button->set_height(button_height);
        }
        m_button_more->set_height(button_height);
    }
}

void Toolbar::append(ObjectPtr child)
{
    Item::append(std::move(child));
}

void Toolbar::insert(ObjectPtr child, size_t index)
{
    ToolbarButton* button = dynamic_cast<ToolbarButton*>(child.get());
    ASSERT(button);

    if (m_button_width > 0 && (m_orientation != Orientation::Horizontal || button->label().empty()))
    {
        button->set_width(m_button_width);
    }
    if (m_button_height > 0 && (m_orientation != Orientation::Vertical || button->label().empty()))
    {
        button->set_height(m_button_height);
    }

    m_buttons.push_back(button);
    Item::insert(std::move(child), object_count() ? index - 1 : index);
}

ObjectPtr Toolbar::remove(Object* child)
{
    return Item::remove(child);
}

bool Toolbar::hovered() const
{
    return m_hovered;
}

bool Toolbar::collapsible() const
{
    return m_collapsible;
}

void Toolbar::set_collapsible(bool collapsible)
{
    m_collapsible = collapsible;
}

float Toolbar::available_size() const
{
    return m_available_size;
}

void Toolbar::set_available_size(float available_size)
{
    m_available_size = available_size;
}

void Toolbar::style_node()
{
    // I absolutely understand this is hidous, I already spent > 0 hours debugging this,
    // I will someday clean this up, but today is not the day
    if (m_collapsible
        && parent_item()
        && m_button_width > 0
        && m_button_height > 0
        && is_visible()
        && width() > 0
        && height() > 0)
    {
        // Compute available size
        float available_size = m_orientation == Orientation::Horizontal ? parent_item()->width() :
                                                                          parent_item()->height();
        for (Item* node : parent_item()->items()) {
            if (node != this) {
                available_size -=
                    m_orientation == Orientation::Horizontal ? node->width() : node->height();
            }
            if (node != *parent_item()->items().rbegin()) {
                available_size -= parent_item()->gap();
            }
            available_size -= m_orientation == Orientation::Horizontal ?
                node->margin().horizontal() :
                node->margin().vertical();
        }

        available_size -= m_orientation == Orientation::Horizontal ? m_padding.horizontal() :
                                                                     m_padding.vertical();

        // Decide which buttons will be included

        // Assume button size
        const float button_size =
            m_orientation == Orientation::Horizontal ? m_button_width : m_button_height;
        // Take away size from collapsed button
        available_size -= button_size;

        std::vector<ToolbarButton*> included_buttons;
        std::vector<ToolbarButton*> collapsed_buttons;
        for (ToolbarButton* button : std::as_const(m_buttons)) {
            if (button == m_button_more) {
                continue;
            }

            if (YGNodeStyleGetDisplay(button->node()) == YGDisplay::YGDisplayNone) {
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

        if (!collapsed_buttons.empty()) {
            // Put every collapsed button to ButtonMore subtoolbar
            m_button_more->set_orientation(orientation());
            Toolbar* subtoolbar = m_button_more->get_or_create_subtoolbar();
            for (ToolbarButton* collapsed_button : std::as_const(collapsed_buttons)) {
                if (collapsed_button->parent() != subtoolbar) {
                    subtoolbar->append(
                        unique_dynamic_cast<ToolbarButton>(Item::remove(collapsed_button))
                    );
                }
            }
        }

        m_button_more->set_visible(!collapsed_buttons.empty());

        Toolbar* subtoolbar = m_button_more->get_subtoolbar();
        if (subtoolbar) {
            // Go through every included button and append it if they are missing
            for (ToolbarButton* included_button : std::as_const(included_buttons)) {
                if (included_button->parent() == subtoolbar) {
                    Item::insert(subtoolbar->remove(included_button), object_count() - 1);
                }
            }
        }
    }

    Item::style_node();
}

std::optional<size_t> Toolbar::index_of(ToolbarButton* button) const
{
    std::vector<ToolbarButton*>::const_iterator it = std::find_if(
        m_buttons.cbegin(),
        m_buttons.cend(),
        [button](ToolbarButton* child_button) { return button == child_button; }
    );

    return it != m_buttons.cend() ? std::distance(m_buttons.cbegin(), it) : std::optional<size_t>();
}

} // namespace Slic3r::App::Yoga
