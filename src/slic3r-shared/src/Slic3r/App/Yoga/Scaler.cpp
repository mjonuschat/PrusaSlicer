///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/Yoga/Scaler.hpp"

namespace Slic3r::App::Yoga {

void Scaler::style_node()
{
    set_size(get_size());

    Item::style_node();
}

float Scaler::get_size() const
{
    if (m_orientation == Orientation::Horizontal) {
        return m_parent->width() - m_parent->padding().horizontal() - m_margin.horizontal();
    } else {
        return m_parent->height() - m_parent->padding().vertical() - m_margin.vertical();
    }
}

void Scaler::set_size(float size)
{
    if (!Domain::fuzzy_compare(m_size, size)) {
        m_size = size;
        m_orientation == Orientation::Horizontal ? set_width(m_size) : set_height(m_size);
        set_style_dirty();
    }
}

} // namespace Slic3r::App::Yoga
