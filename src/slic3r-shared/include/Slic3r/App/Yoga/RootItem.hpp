///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/Yoga/Item.hpp"
#include "Slic3r/App/Yoga/ItemEvents.hpp"

namespace Slic3r::App::Yoga {

class Popup;

class RootItem : public Item
{
public:
    RootItem();

    void render(Vec2f pos, Vec2f size) override;

    void set_style_dirty() override;

    void style_node() override;

    void process_events(Vec2f pos, Vec2f size) override;

    void open_popup(Popup* popup);
    void close_popup(Popup* popup);

    /**
     * @brief Yoga recalculate whole tree
     * @note should be called only top-level item
     */
    void resize(Vec2f size);

    void push_event(EventPtr event) override;

protected:
    Vec2f get_available_size() const override;

protected:
    LoopEvents m_loop_events;

    bool m_style_dirty = true;

    Vec2f m_size;

    using Popups = std::vector<Popup*>;
    Popups m_popups;
};

} // namespace Slic3r::App::Yoga
