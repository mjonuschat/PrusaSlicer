///|/ Copyright (c) Prusa Research 2018 - 2023 Enrico Turri @enricoturri1966, Tomáš Mészáros @tamasmeszaros, Lukáš Matěna @lukasmatena, Oleksandra Iushchenko @YuSanka, Filip Sykala @Jony01, Vojtěch Bubník @bubnikv, Lukáš Hejl @hejllukas, David Kocík @kocikdav, Vojtěch Král @vojtechkral
///|/ Copyright (c) BambuStudio 2023 manch1n @manch1n
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "yoga/Yoga.h"
#include "imgui/imgui.h"
#include "Slic3r/App/Yoga/Align.hpp"

#include<map>
#include<string>
#include<functional>

namespace Slic3r::App::Yoga {

struct NodeRendering {
    std::function<void(ImVec2, ImVec2)> render_fn   { nullptr };
    std::string                         win_name    {};
};

class FlexSizer
{
public:
    FlexSizer() {}
    FlexSizer(int col, int row, ImVec2 min_size = ImVec2(), ImVec2 margins = ImVec2());
    virtual ~FlexSizer();

    bool    is_init();
    void    init(int col, int row, ImVec2 min_size = ImVec2(), ImVec2 margins = ImVec2());

    // Note for add(): win_name_prefix is a prefix name for windows, which will be created for this sizer item.
    // Have to be empty, if this item is rendered inside the currect imgui window.
    // add some item as a control or line of controls.
    void    add(std::function<void(ImVec2, ImVec2)> render_fn = nullptr, Align align = {}, const std::string& win_name_prefix = std::string());
    // add inner sizer
    void    add(FlexSizer& inner_sizer, const std::string& win_name_prefix = std::string());

    virtual void    render(ImVec2 win_size = ImVec2(), ImVec2 win_pos = ImVec2(-1.f, -1.f));
    virtual void    layout();

    void    set_grow_col(int col, float grow = 1.f);
    void    set_grow_row(int row, float grow = 1.f);
    void    align_col(int col, Align align = {});
    void    align_row(int row, Align align = {});
    void    align_cell(int col, int row, Align align = {});

    void    set_bg_alpha(float alpha)       { m_bg_alpha = alpha; }

#if DEBUG
    void    show_node_shapes(bool show) { m_show_node_shapes = show; }
#endif

    int     get_cols() const; // get count of columns
    int     get_rows() const; // get count of rows

    void    show_col(int col, bool show = true);
    void    show_row(int row, bool show = true);
protected:
    void    render_node(YGNodeRef node, ImVec2 win_pos);
    void    finalize();
    void    ensure_min_size();
    void    resize(ImVec2 win_size);
    void    render_nodes_bg(ImVec2 win_pos);
    bool    has_parent_window();

    YGNodeRef   get_node(int col, int row) const;
    YGNodeRef   get_next_node();              // get next node to intialize
    YGSize      get_best_size();
    YGSize      get_min_size();

protected:
    YGNodeRef   m_root              { nullptr };

    bool        m_finalized         { false };
    bool        m_show_node_shapes  { false };

private:
    float       m_v_margin      { 0.f };
    float       m_h_margin      { 0.f };

    int         m_next_col      { 0 };
    int         m_next_row      { 0 };
    float       m_bg_alpha      { 0.f }; // used for sizers without parent window

    // max{calculated min size; adjusted min size}
    YGSize      m_min_size      { YGSize({ 0.f, 0.f }) };

    std::map<YGNodeRef, NodeRendering>  m_node_rendering;
};

}

