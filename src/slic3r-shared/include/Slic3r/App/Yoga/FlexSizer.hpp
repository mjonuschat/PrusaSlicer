///|/ Copyright (c) Prusa Research 2018 - 2023 Enrico Turri @enricoturri1966, Tomáš Mészáros @tamasmeszaros, Lukáš Matěna @lukasmatena, Oleksandra Iushchenko @YuSanka, Filip Sykala @Jony01, Vojtěch Bubník @bubnikv, Lukáš Hejl @hejllukas, David Kocík @kocikdav, Vojtěch Král @vojtechkral
///|/ Copyright (c) BambuStudio 2023 manch1n @manch1n
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "yoga/Yoga.h"
#include "Slic3r/Domain/Types.hpp"
#include "Slic3r/App/Yoga/Align.hpp"

#include<map>
#include<string>
#include<functional>

namespace Slic3r::App::Yoga {

using Vec2f = Slic3r::Domain::Vec2f;

// parameters for ImGui::Window
struct WindowParams
{
    std::string name_prefix {};
    Vec2f       paddings    { 0.f, 0.f };
};

struct NodeRendering {
    std::function<void(Vec2f, Vec2f)>   render_fn               { nullptr };
    WindowParams                        win                     {};
    Vec2f                               inner_sizer_min_size    {0.f, 0.f};
};

struct Margins {
    Margins(float horizontal = 0.f, float vertical = 0.f) {
        left = right = horizontal;
        top = bottom = vertical;
    }

    Margins(Vec2f margins) {
        left = right = margins.x();
        top = bottom = margins.y();
    }

    float left      { 0.f };
    float right     { 0.f };
    float top       { 0.f };
    float bottom    { 0.f };
};

class FlexSizer
{
public:
    FlexSizer() {}
    FlexSizer(int col, int row, Vec2f min_size = Vec2f(0.f, 0.f), Margins margins = Margins());
    virtual ~FlexSizer();

    bool    is_inited();
    void    init(int col, int row, Vec2f min_size = Vec2f(0.f, 0.f), Margins margins = Margins());

    // Note for add(): win_name_prefix is a prefix name for windows, which will be created for this sizer item.
    // Have to be empty, if this item is rendered inside the currect imgui window.
    // add some item as a control or line of controls.
    void    add(std::function<void(Vec2f, Vec2f)> render_fn = nullptr, bool single_item = false, const WindowParams& win = {}, Align align = { Yoga::AlignH::Left, Yoga::AlignV::Top });
    // add inner sizer
    void    add(FlexSizer& inner_sizer, const WindowParams& win = {}, Align align = {});

    virtual void    render(Vec2f win_size = Vec2f(0.f, 0.f), Vec2f win_pos = Vec2f(-1.f, -1.f));
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

    bool    is_shown_col(int col);
    bool    is_shown_row(int row);
protected:
    void    render_node(YGNodeRef node, Vec2f win_pos);
    void    finalize();
    void    ensure_min_size();
    void    resize(Vec2f win_size);
    void    render_nodes_bg(Vec2f win_pos);
    bool    has_parent_window();

    YGNodeRef   get_node(int col, int row) const;
    YGNodeRef   get_next_node();              // get next node to intialize
    Vec2f       get_best_size();
    Vec2f       get_min_size();
    Vec2f       get_inner_sizer_min_size(YGNodeRef node) { return m_node_rendering[node].inner_sizer_min_size; }

protected:
    YGNodeRef   m_root              { nullptr };

    bool        m_finalized         { false };
    bool        m_show_node_shapes  { false };

private:
    Margins     m_margins;

    int         m_next_col      { 0 };
    int         m_next_row      { 0 };
    float       m_bg_alpha      { 0.f }; // used for sizers without parent window

    // max{calculated min size; adjusted min size}
    Vec2f       m_min_size      { 0.f, 0.f };

    std::map<YGNodeRef, NodeRendering>  m_node_rendering;
};

}

