///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/Yoga/Item.hpp"

namespace Slic3r::App::Yoga {

class ContextPopup : public Item
{
public:
    explicit ContextPopup(const std::string& name = {});

    void style_node() override;

    void render(Vec2f pos, Vec2f size) override;

    float offset() const;
    void set_offset(float offset);

    Position position() const;
    void set_position(Position position);

    float rounding() const;
    void set_rounding(float rounding);

    void open();
    void close();
    bool opened() const;

private:
    float m_offset           = 10;
    Position m_position      = Position::Right;
    float m_rounding         = 5;
    ImGuiWindowFlags m_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove;
    bool m_request_close     = false;
};

} // namespace Slic3r::App::Yoga
