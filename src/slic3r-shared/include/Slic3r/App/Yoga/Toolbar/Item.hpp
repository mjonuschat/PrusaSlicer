///|/ Copyright (c) Prusa Research 2018 - 2023 Enrico Turri @enricoturri1966, Tomáš Mészáros @tamasmeszaros, Lukáš Matěna @lukasmatena, Oleksandra Iushchenko @YuSanka, Filip Sykala @Jony01, Vojtěch Bubník @bubnikv, Lukáš Hejl @hejllukas, David Kocík @kocikdav, Vojtěch Král @vojtechkral
///|/ Copyright (c) BambuStudio 2023 manch1n @manch1n
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "imgui/imgui.h"
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "imgui/imgui_internal.h"

#include<string>
#include<functional>

namespace Slic3r::App::Yoga {

struct Align;

namespace Toolbar {

class Toolbar;
enum class Orientation;

// parameters for action functions is a bounding box of item
struct Callbacks {
    std::function<void(ImRect)>     action                      { nullptr };
    std::function<bool()>           visibility                  { []() {return true; } };
    std::function<bool()>           enabling                    { []() {return true; } };
    std::function<bool()>           toggled                     { []() {return false; } };
    std::function<void(ImRect)>     action_on_arrow             { nullptr };
    std::function<void(ImRect)>     action_on_arrow_hovering    { nullptr };

    bool is_empty() const { return !action && !action_on_arrow && !action_on_arrow_hovering; }
};

class Item
{
public :

    Item() {}
    // create as an item
    Item(const std::string& name, const std::string& tooltip, Callbacks callbacks) :
      m_icon_name(name)
    , m_tooltip(tooltip)
    , m_callbacks(callbacks)
    {}
    // create as an item with submenu separator
    Item(const std::string& name, Toolbar* sub_toolbar) :
      m_icon_name(name)
    , m_sub_toolbar(sub_toolbar) 
    {}
    // create as a separator
    Item(float separator_size) :
      m_size_as_separator(separator_size)
    {}
    ~Item();

    void    set_action_on_arrow(std::function<void(ImRect)> cb)             { m_callbacks.action_on_arrow = cb; }
    void    set_action_on_arrow_hovering(std::function<void(ImRect)> cb)    { m_callbacks.action_on_arrow_hovering = cb; }

    bool    is_separator() const;
    bool    is_visible() const;
    float   separator_size() const { return m_size_as_separator; }

    // render node and return true, if item was hovered
    bool    render(ImRect item_bb, ImRect parent_bb, ImDrawCornerFlags corners_flag, ImVec2 tooltip_pivot);
    void    render_sub_toolbar(ImRect item_bb, ImRect parent_bb, bool force);
    void    render_tooltip(ImVec2 win_pos, ImVec2 tt_shift = ImVec2(), ImVec2 pivot = ImVec2(), bool for_arrow = false);

    void    set_sub_toolbar(Toolbar* sub_toolbar);
    void    init_sub_toolbar(float min_item_size, float max_item_size, Yoga::Align align, Orientation orientation);
    void    add_sub_toolbar_item(const Item& item);
    void    insert_sub_toolbar_item(int insert_pos, const Item& item);
    void    erase_sub_toolbar_item(int erase_pos);

    //tmp func
    const std::string& name() const { return m_icon_name; }

private:
    std::string m_icon_name; // or icon or texture
    std::string m_tooltip;
    float       m_size_as_separator { 5.f };
    bool        m_has_arrow         { false };
    bool        m_is_toggled        { false };
    Callbacks   m_callbacks;

    Toolbar*    m_sub_toolbar       { nullptr };
    bool        m_has_internal_tollbar{ false };
};

} // Toolbar
} // Slic3r::App::Yoga
