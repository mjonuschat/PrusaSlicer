///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/Yoga/RootItem.hpp"

namespace Slic3r::App::Yoga {

void RootItem::render(Vec2f pos, Vec2f size)
{
    ASSERT(!m_parent);

    if (!size.isZero()) {
        style_node();
        resize(size);
        m_style_dirty = false;
        process_events(pos, size);
        if (m_style_dirty) {
            style_node();
            resize(size);
        }
    }

    render_item_begin(pos, size);

    render_item_end(pos, size);

    m_loop_events.process_events();
}

void RootItem::set_style_dirty() { m_style_dirty = true; }

void RootItem::push_event(EventPtr event) { m_loop_events.insert_event(std::move(event)); }

void RootItem::resize(Vec2f size)
{
    YGNodeCalculateLayout(m_node, size.x(), size.y(), YGDirectionLTR);
}

} // namespace Slic3r::App::Yoga
