///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/Yoga/Window.hpp"

namespace Slic3r::App::Yoga {

class AttachedWindow : public Window {
public:

    AttachedWindow(const std::string& window_name, Position position);

    void style_node() override;

    float offset() const;
    void set_offset(float offset);

    Position position() const;
    void set_position(Position position);

private:
    float m_offset = 10;
    Position m_position = Position::Right;
};

}
