///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/Yoga/Item.hpp"

namespace Slic3r::App::Yoga {

class ScrollArea : public Item
{
public:
    explicit ScrollArea(const std::string& name = "ScrollArea");

    void render(Vec2f pos, Vec2f size) override;

    void process_events(Vec2f pos, Vec2f size) override;

    ImGuiChildFlags child_flags() const;
    void set_child_flags(ImGuiChildFlags child_flags);

    ImGuiWindowFlags window_flags() const;
    void set_window_flags(ImGuiWindowFlags window_flags);

    const Vec2f& content_pos() const;

private:
    ImGuiChildFlags m_child_flags = 0;
    ImGuiWindowFlags m_window_flags = 0;
    Vec2f m_last_scroll;
};

} // namespace Slic3r::App::Yoga
