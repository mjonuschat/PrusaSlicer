///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/Yoga/RootItem.hpp"

#include "Slic3r/App/Yoga/Popup.hpp"

namespace Slic3r::App::Yoga {

RootItem::RootItem()
{
    set_item_name("RootItem");
}

void RootItem::render(Vec2f pos, Vec2f size)
{
    ASSERT(!m_parent);

    if (size.isZero()) {
        return;
    }

    m_size = size;

    style_node();
    m_style_dirty = false;
    resize(size);
    process_events(pos, size);
    if (m_style_dirty) {
        style_node();
        resize(size);
    }

    render_item_begin(pos, size);

    render_item_end(pos, size);

    for (Popup* popup : std::as_const(m_popups)) {
        popup->render(size);
    }

    m_loop_events.process_events();
}

void RootItem::set_style_dirty() { m_style_dirty = true; }

void RootItem::push_event(EventPtr event) { m_loop_events.insert_event(std::move(event)); }

Vec2f RootItem::get_available_size() const { return m_size; }

void RootItem::resize(Vec2f size)
{
    YGNodeCalculateLayout(m_node, size.x(), size.y(), YGDirectionLTR);

    for (Popup* popup : std::as_const(m_popups)) {
        popup->resize(size);
    }

    check_resized();

    for (Popup* popup : std::as_const(m_popups)) {
        popup->check_resized();
    }
}

void RootItem::style_node()
{
    Item::style_node();

    for (Popup* popup : std::as_const(m_popups)) {
        popup->style_node();
    }
}

void RootItem::process_events(Vec2f pos, Vec2f size)
{
    Item::process_events(pos, size);

    for (Popup* popup : std::as_const(m_popups)) {
        popup->process_events(pos, size);
    }
}

void RootItem::open_popup(Popup* popup)
{
    ASSERT(popup);
    ASSERT(popup->content_item());

    Popups::const_iterator it = std::find(m_popups.cbegin(), m_popups.cend(), popup);
    if (it != m_popups.cend()) {
        return;
    }

    m_popups.push_back(popup);
}

void RootItem::close_popup(Popup* popup)
{
    Popups::const_iterator it = std::find(m_popups.cbegin(), m_popups.cend(), popup);
    if (it == m_popups.cend()) {
        return;
    }

    m_popups.erase(it);
}

} // namespace Slic3r::App::Yoga
