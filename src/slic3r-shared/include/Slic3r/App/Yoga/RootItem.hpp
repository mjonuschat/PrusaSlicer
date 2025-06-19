///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/Yoga/Item.hpp"
#include "Slic3r/App/Yoga/ItemEvents.hpp"

namespace Slic3r::App::Yoga {

class RootItem : public Item
{
public:
    void render(Vec2f pos, Vec2f size) override;

    void set_style_dirty() override;

    void resize(Vec2f size) override;

protected:
    void push_event(EventPtr event) override;

protected:
    LoopEvents m_loop_events;

    bool m_style_dirty = true;
};

} // namespace Slic3r::App::Yoga
