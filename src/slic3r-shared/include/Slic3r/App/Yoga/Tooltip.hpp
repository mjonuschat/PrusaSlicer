///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/Yoga/Window.hpp"

namespace Slic3r::App::Yoga {

class Text;

class Tooltip : public Window
{
public:

    enum class Position {
        Top,
        Bottom,
        Left,
        Right
    };

    Tooltip(
        const std::string& window_name,
        const std::string& text,
        const std::string& shortcut,
        Item* parent
    );

    void style_node() override;

    const std::string& text() const;
    void set_text(const std::string& text);

    const std::string& shortcut() const;
    void set_shortcut(const std::string& shortcut);

    float offset() const;
    void set_offset(float offset);

    Position position() const;
    void set_position(Position position);

private:
    Text* m_text = nullptr;
    Text* m_shortcut = nullptr;

    float m_offset = 10;
    Position m_position = Position::Right;
};

} // namespace Slic3r::App::Yoga
