///|/ Copyright (c) Prusa Research 2018 - 2023 Enrico Turri @enricoturri1966, Tomáš Mészáros @tamasmeszaros, Lukáš Matěna @lukasmatena, Oleksandra Iushchenko @YuSanka, Filip Sykala @Jony01, Vojtěch Bubník @bubnikv, Lukáš Hejl @hejllukas, David Kocík @kocikdav, Vojtěch Král @vojtechkral
///|/ Copyright (c) BambuStudio 2023 manch1n @manch1n
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "yoga/Yoga.h"

#include "Slic3r/App/Yoga/Toolbar/Item.hpp"
#include "Slic3r/App/Yoga/Align.hpp"
#include "Slic3r/Domain/Types.hpp"

#include <map>

namespace Slic3r::App::Yoga::Toolbar {

using Vec2f = Domain::Vec2f;

struct Callbacks;
class Item;

enum class Orientation {
     Horizontal
    ,Vertical
};

class Toolbar
{
public:
    Toolbar() {}
    Toolbar(const std::string& name, float min_item_size = 25.f, float max_item_size = 50.f, Yoga::Align align = {}, Orientation orientation = Orientation::Vertical);
    ~Toolbar();

    void    init(const std::string& name, float min_item_size = 25.f, float max_item_size = 50.f, Yoga::Align align = {}, Orientation orientation = Orientation::Vertical);

    Item&   add(wchar_t icon, const std::string& tooltip, const std::string& shortcut, Callbacks callbacks);
    Item&   insert(int id, wchar_t icon, const std::string& tooltip, const std::string& shortcut, Callbacks callbacks);
    Item&   add(wchar_t icon, Toolbar* sub_toolbar);
    Item&   add(const Item& item);
    Item&   insert(int id, const Item& item);
    void    erase(int id = -1);
    void    add_separator(float size);
    void    clear();
    bool    is_empty() const;
    void    set_margins(float h_margin, float v_margin);

    void    render(Vec2f win_size = Vec2f(0.f, 0.f), Vec2f win_pos = Vec2f(-1.f, -1.f));
    void    layout();
    void    set_collapsible();
    void    set_cb_on_visible_items_changed(std::function<void()> cb) { m_cb_on_visible_items_changed = cb; }

    size_t  items_cnt() const;
    float   get_flex_ration() const;
    bool    is_horizontal() const { return m_is_horizontal; }
    // get real bounding box of the control in respect to the layout
    ImRect  get_bb(Vec2f win_pos);

private:

    YGNodeRef   create_node();
    YGNodeRef   create_separator_node(float size);
    YGNodeRef   add_node();
    YGNodeRef   add_separator_node(float size);
    YGNodeRef   insert_node(int id);
    YGNodeRef   insert_separator_node(int id, float size);
    size_t      insert_pos();

    void        add_item(std::map<YGNodeRef, Item>&  nodes, YGNodeRef new_node, const Item& new_item);

    void        finalize();
    void        ensure_min_size();
    void        update_min_size();
    void        resize(Vec2f win_size);

    // get real size of the control in respect to the layout
    Vec2f       get_size(float side);
    Vec2f       tooltip_pivot();
    ImDrawFlags corners_flag(int id);

    // render node and return true, if item was hovered
    bool        render_node(int id, Vec2f win_pos, ImRect bb);
    void        render_tooltip(int id, Vec2f win_pos, bool for_arrow = false);

    bool        is_collapsible();
    Item&       subtoolbar_expander();

    void        refresh_full_layout(bool force_parent_layout = true);
    void        collapse_from(size_t start_collapse_id);
    void        process_collapse(Vec2f win_size);
    void        process_items_visibility();

private:
    YGNodeRef   m_root              { nullptr };
    std::string m_name              {};
    Yoga::Align m_align             { Yoga::Align() };

    bool        m_finalized         { false };
    bool        m_is_horizontal     { false };
    bool        m_show_node_shapes  { false };
    bool        m_show_tooltips     { false };

    float       m_v_margin          { 0.f };
    float       m_h_margin          { 0.f };
    float       m_min_side          { 0.f };
    float       m_max_side          { 0.f };

    YGNodeRef   m_subtoolbar_expander_node    { nullptr };
    bool        m_show_subtoolbar_expander{ false };

    // max{calculated min size; adjusted min size}
    Vec2f       m_min_size          { 0.f, 0.f };

    std::map<YGNodeRef, Item>  m_nodes;

    // for delay tooltip showing 
    bool        m_running       { false };
    float       m_elapsed_time  { 0.f };

    std::function<void()> m_cb_on_visible_items_changed{nullptr};
};

}
