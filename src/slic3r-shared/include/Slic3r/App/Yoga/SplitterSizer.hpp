///|/ Copyright (c) Prusa Research 2018 - 2023 Enrico Turri @enricoturri1966, Tomáš Mészáros @tamasmeszaros, Lukáš Matěna @lukasmatena, Oleksandra Iushchenko @YuSanka, Filip Sykala @Jony01, Vojtěch Bubník @bubnikv, Lukáš Hejl @hejllukas, David Kocík @kocikdav, Vojtěch Král @vojtechkral
///|/ Copyright (c) BambuStudio 2023 manch1n @manch1n
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "yoga/Yoga.h"
#include <string>

#include "Slic3r/App/Yoga/FlexSizer.hpp"

namespace Slic3r::App::Yoga {


class SplitterSizer : public FlexSizer
{
public:
    SplitterSizer() {}
    SplitterSizer(int items_cnt, Vec2f min_size = Vec2f(0.f, 0.f), bool is_horizontal = true);
    ~SplitterSizer() override = default;

    void    init(int items_cnt, Vec2f min_size = Vec2f(0.f, 0.f), bool is_horizontal = true);
    void    render(Vec2f win_size = Vec2f(0.f, 0.f), Vec2f win_pos = Vec2f(-1.f, -1.f)) override;
    void    show_splitter(bool show);
    void    set_splitter_sz(float val);
    void    set_splitter_padding(float val);

private:

    float   render_splitter(YGNodeRef node, const std::string& suffix, Vec2f pos, bool is_after_item = true);
    float   splitter(YGNodeRef node, const std::string& suffix, Vec2f pos, bool is_after_item = true);

    void    apply_width(YGNodeRef node, float delta,  float width);
    void    apply_height(YGNodeRef node, float delta, float height);
    void    apply_size(YGNodeRef node, float delta, Vec2f size);
    void    apply_splitter_spacing();

private:
    bool        m_is_horizontal     { true };
    bool        m_invisible_btn     { false };
    float       m_splitter_sz       { 6.f };
    float       m_splitter_padding  { 10.f };

    Vec2f      m_splitter_spacing;

    Vec2f      WindowPadding       {Vec2f(0.f, 0.f)};
};

}
