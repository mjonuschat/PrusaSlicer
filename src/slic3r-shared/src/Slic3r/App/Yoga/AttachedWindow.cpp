///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/

#include "Slic3r/App/Yoga/AttachedWindow.hpp"

namespace Slic3r::App::Yoga {

AttachedWindow::AttachedWindow(const std::string& window_name, Position position)
    : Window(window_name), m_position(position)
{
    set_position_type(YGPositionType::YGPositionTypeAbsolute);
}

void AttachedWindow::style_node()
{
    if (is_visible()) {
        switch (m_position) {
        case Position::Right:
            set_right(-(m_offset + width()));
            set_top(m_parent->height() * 0.5 - height() * 0.5);
            break;
        case Position::Left:
            set_left(-(m_offset + width()));
            set_top(m_parent->height() * 0.5 - height() * 0.5);
            break;
        case Position::Top:
            set_top(-(m_offset + width()));
            set_left(m_parent->width() * 0.5 - width() * 0.5);
            break;
        case Position::Bottom:
            set_bottom(-(m_offset + height()));
            set_left(m_parent->width() * 0.5 - width() * 0.5);
            break;
        }
    }

    Window::style_node();
}

float AttachedWindow::offset() const { return m_offset; }

void AttachedWindow::set_offset(float offset) { m_offset = offset; }

Position AttachedWindow::position() const { return m_position; }

void AttachedWindow::set_position(Position position)
{
    if (m_position != position) {
        m_position = position;
        set_top(YGUndefined);
        set_bottom(YGUndefined);
        set_left(YGUndefined);
        set_right(YGUndefined);
    }
}

} // namespace Slic3r::App::Yoga
