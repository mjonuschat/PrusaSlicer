///|/ Copyright (c) Prusa Research 2018 - 2025 Oleksandra Iushchenko @YuSanka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/Yoga/WindowParams.hpp"
#include "Slic3r/App/Yoga/Namespace.hpp"

#include "yoga/Yoga.h"

#include <map>

namespace Slic3r::App::Yoga {

class FlexSizer
{
    friend class FlexSizerFixture;
public:
    FlexSizer();
    FlexSizer(int col, int row, Vec2f min_size = Vec2f(0.f, 0.f), Margins margins = Margins());
    virtual ~FlexSizer();

    bool    is_inited() const;
    void    init(int col, int row, Vec2f min_size = Vec2f(0.f, 0.f), Margins margins = Margins());

    // Note for add(): win_name_prefix is a prefix name for windows, which will be created for this sizer item.
    // Have to be empty, if this item is rendered inside the currect imgui window.
    // add some item as a control or line of controls.
    void    add(RenderPosFn render_fn = nullptr, bool single_item = false, const WindowParams& win = {}, Align align = { Yoga::AlignH::Left, Yoga::AlignV::Top });
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

    bool    is_shown_col(int col) const;
    bool    is_shown_row(int row) const;
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
    struct NodeRendering {
        RenderPosFn   render_fn               { nullptr };
        WindowParams   win                     {};
        Vec2f          inner_sizer_min_size    {0.f, 0.f};
    };

    Margins     m_margins;

    int         m_next_col      { 0 };
    int         m_next_row      { 0 };
    float       m_bg_alpha      { 0.f }; // used for sizers without parent window

    // max{calculated min size; adjusted min size}
    Vec2f       m_min_size      { 0.f, 0.f };

    std::map<YGNodeRef, NodeRendering>  m_node_rendering;
};

}

