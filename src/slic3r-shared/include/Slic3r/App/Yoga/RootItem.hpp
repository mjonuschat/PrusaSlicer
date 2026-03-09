///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/Yoga/Item.hpp"
#include "Slic3r/App/Yoga/ItemEvents.hpp"

#include <list>

namespace Slic3r::App::Yoga {

class Popup;

class RootItem : public Item
{
public:
    RootItem();
    ~RootItem();

    void render(Vec2f pos, Vec2f size) override;

    void set_style_dirty() override;

    void style_node() override;

    void open_popup(Popup* popup);
    void close_popup(Popup* popup);

    /**
     * @brief Yoga recalculate whole tree
     * @note should be called only top-level item
     */
    void resize(Vec2f size);

    void push_event(EventPtr event) override;

    template <class F>
    void for_each_popup_reconcile(F&& fn);


protected:
    Vec2f get_available_size() const override;

    void render_debug_overlay();

protected:
    LoopEvents m_loop_events;

    bool m_style_dirty = true;

    Vec2f m_size;

    using Popups = std::list<Popup*>;
    Popups m_popups;
    Popups m_popups_to_be_added;
    Popups m_popups_to_be_deleted;
};

} // namespace Slic3r::App::Yoga
