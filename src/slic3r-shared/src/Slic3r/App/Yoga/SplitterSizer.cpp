///|/ Copyright (c) Prusa Research 2018 - 2023 Enrico Turri @enricoturri1966, Oleksandra Iushchenko @YuSanka, Tomáš Mészáros @tamasmeszaros, Lukáš Matěna @lukasmatena, Vojtěch Bubník @bubnikv, David Kocík @kocikdav, Filip Sykala @Jony01, Lukáš Hejl @hejllukas, Vojtěch Král @vojtechkral

#include "Slic3r/App/Yoga/SplitterSizer.hpp"

#include <Yoga.h>

#include <imgui/imgui_internal.h>

namespace Slic3r::App::Yoga {

SplitterSizer::SplitterSizer(int items_cnt, ImVec2 min_size/* = ImVec2()*/, bool is_horizontal /*= true*/)
{
    init(items_cnt, min_size, is_horizontal);
}

void SplitterSizer::init(int items_cnt, ImVec2 min_size/* = ImVec2()*/, bool is_horizontal/* = true*/)
{
    FlexSizer::init(is_horizontal ? items_cnt : 1, is_horizontal ? 1 : items_cnt, min_size);

    m_is_horizontal = is_horizontal;
    apply_splitter_spacing();

    if (!is_horizontal) {
        set_grow_col(0);

        for (size_t row = 0; row < items_cnt; row++) {
            YGNodeRef row_node = get_node(0, row);
            // Discard vertical flexibility
            YGNodeStyleSetFlex(row_node, 0.f);
            YGNodeStyleSetFlexGrow(row_node, 0.f);
        }
    }
}

void SplitterSizer::show_splitter(bool show)
{ 
    m_invisible_btn = !show;
}

void SplitterSizer::set_splitter_sz(float val) 
{
    m_splitter_sz = val; 
    apply_splitter_spacing();
}
void SplitterSizer::set_splitter_padding(float val) 
{
    m_splitter_padding = val; 
    apply_splitter_spacing();
}

void SplitterSizer::apply_splitter_spacing()
{
    float splitter_space = m_splitter_sz + 2.f * m_splitter_padding;
    m_splitter_spacing = m_is_horizontal ? ImVec2(splitter_space, 0.f) : ImVec2(0.f, splitter_space);
}

void SplitterSizer::apply_width(YGNodeRef node, float delta, float width)
{
    if (delta == 0.f)
        return;

    float old_w = YGNodeLayoutGetWidth(node);
    float new_w = old_w + delta;

    if ((std::isnan(YGNodeStyleGetMaxWidth(node).value) || new_w <= YGNodeStyleGetMaxWidth(node).value) && 
        new_w > YGNodeStyleGetWidth(YGNodeGetChild(node, 0)).value) {
        // perform layout with new size
        YGNodeStyleSetWidth(node, new_w);
        layout();

        // check if new sum of widths doesn't overdraw the required width of root...
        float new_root_w{ 0.f };
        for (int col = 0; col < get_cols(); col++)
            new_root_w += YGNodeLayoutGetWidth(YGNodeGetChild(m_root, col));

        if (new_root_w > width) {
            // ...revert previous width, if yes
            YGNodeStyleSetWidth(node, old_w);
            layout();
        }
    }
}

void SplitterSizer::apply_height(YGNodeRef node, float delta, float height)
{
    if (delta == 0.f)
        return;

    float old_h = YGNodeLayoutGetHeight(node);
    float new_h = old_h + delta;

    if ((std::isnan(YGNodeStyleGetMaxHeight(node).value) || new_h <= YGNodeStyleGetMaxHeight(node).value) && 
        new_h > YGNodeStyleGetHeight(YGNodeGetChild(node, 0)).value) {
        // perform layout with new size
        YGNodeStyleSetHeight(node, new_h);
        layout();

        // check if new sum of heights doesn't overdraw the required height of root...
        float new_root_h{ 0.f };
        for (int row = 0; row < get_rows(); row++)
            new_root_h += YGNodeLayoutGetHeight(get_node(0, row));

        if (new_root_h > height) {
            // ...revert previous height, if yes
            YGNodeStyleSetHeight(node, old_h);
            layout();
        }
    }
}

void SplitterSizer::apply_size(YGNodeRef node, float delta, ImVec2 size)
{
    if (m_is_horizontal)
        apply_width(node, delta, size.x);
    else 
        apply_height(node, delta, size.y);
}

float SplitterSizer::render_splitter(YGNodeRef node, const std::string& suffix, ImVec2 pos, bool is_after_item/* = true*/)
{
    float ret = 0.f;

    pos += m_is_horizontal ? ImVec2(m_splitter_padding, 0.f) : ImVec2(0.f, m_splitter_padding);
    ImGui::SetCursorScreenPos(pos);

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.f);
    ImGui::PushStyleColor(ImGuiCol_Button,          ImVec4({ 0.67f, 0.67f, 0.67f, 1.0f }));

    if (m_is_horizontal) {
        float h = YGNodeLayoutGetHeight(node);
        if (m_invisible_btn)
            ImGui::InvisibleButton(("vsplitter" + suffix).c_str(), ImVec2(m_splitter_sz, h));
        else
            ImGui::Button(("##vsplitter" + suffix).c_str(), ImVec2(m_splitter_sz, h));
        if (ImGui::IsItemActive())
            ret =  is_after_item ? ImGui::GetIO().MouseDelta.x : (-ImGui::GetIO().MouseDelta.x);
    }
    else {
        float w = YGNodeLayoutGetWidth(YGNodeGetParent(node));
        if (m_invisible_btn)
            ImGui::InvisibleButton(("hsplitter" + suffix).c_str(), ImVec2(w, m_splitter_sz));
        else
            ImGui::Button(("##hsplitter" + suffix).c_str(), ImVec2(w, m_splitter_sz));
        if (ImGui::IsItemActive())
            ret = is_after_item ? ImGui::GetIO().MouseDelta.y : (-ImGui::GetIO().MouseDelta.y);
    }

    ImGui::PopStyleColor(1);
    ImGui::PopStyleVar();

    return ret;
}

float SplitterSizer::splitter(YGNodeRef node, const std::string& suffix, ImVec2 pos, bool is_after_item/* = true*/)
{
    bool in_separate_window = !has_parent_window();

    if (in_separate_window) {
        std::string win_name = "splitter_";

        if (m_is_horizontal) {
            ImGui::SetNextWindowSize(ImVec2(m_splitter_spacing.x, YGNodeLayoutGetHeight(node)));
            win_name += "h_" + suffix;
        }
        else {
            float col_width = YGNodeLayoutGetWidth(YGNodeGetParent(node));
            ImGui::SetNextWindowSize(ImVec2(col_width, m_splitter_spacing.y));
            win_name += "v_" + suffix;
        }
        ImGui::SetNextWindowPos(pos);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
        ImGui::Begin(win_name.c_str(), nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground);
    }

    float ret = render_splitter(node, suffix, pos, is_after_item);

    if (in_separate_window) {
        ImGui::End();
        ImGui::PopStyleVar(2);
    }

    return ret;
}

void SplitterSizer::render(ImVec2 win_size/* = ImVec2()*/, ImVec2 win_pos /*= ImVec2(-1.f, -1.f)*/)
{
    if (!m_finalized)
        finalize();

    int cols = get_cols();
    int rows = get_rows();

    // Backup initial size
    auto in_win_sz = win_size;

    // calculate count of visible splitters
    int splitters_cnt = 0;
    bool    is_flex_prev_child = false;
    for (int row = 0; row < rows; row++) {
        for (int col = 0; col < cols; col++) {
            YGNodeRef node = get_node(col, row);

            const bool is_visible = YGNodeStyleGetDisplay(m_is_horizontal ? YGNodeGetParent(node) : node) != YGDisplayNone;
            const bool is_flex = YGNodeStyleGetFlexGrow(m_is_horizontal ? YGNodeGetParent(node) : node) > 0.f;

            if (is_flex_prev_child && is_visible)
                splitters_cnt++;

            if (size_t(m_is_horizontal ? col : row) + 1 == size_t(m_is_horizontal ? cols : rows))
                break;

            if (!is_flex && is_visible)
                splitters_cnt++;

            is_flex_prev_child = is_flex;
        }
    }

    // decrease size in respect to the splitters
    if (m_is_horizontal)
        win_size.x -= m_splitter_spacing.x * splitters_cnt;
    else
        win_size.y -= m_splitter_spacing.y * splitters_cnt;

    resize(win_size);

    // corect height, if it was default
    if (in_win_sz.y == 0.f)
        in_win_sz.y = YGNodeLayoutGetHeight(m_root);

    if (win_pos.x < 0 && win_pos.y < 0)
        win_pos = ImGui::GetCursorScreenPos();

    if (m_show_node_shapes)
        render_nodes_bg(win_pos);

    ImVec2 old_pos = ImGui::GetCursorScreenPos();

    ImVec2 splitter_pos = win_pos;

    is_flex_prev_child = false;
    for (int row = 0; row < rows; row++) {
        for (int col = 0; col < cols; col++) {

            YGNodeRef node = get_node(col, row);

            size_t i = size_t(m_is_horizontal ? col : row);

            std::string suffix    = std::to_string(i);

            const bool is_visible = YGNodeStyleGetDisplay(m_is_horizontal ? YGNodeGetParent(node) : node) != YGDisplayNone;
            const bool is_flex = YGNodeStyleGetFlexGrow(m_is_horizontal ? YGNodeGetParent(node) : node) > 0.f;

            if (is_flex_prev_child && is_visible) {
                apply_size(node, splitter(node, suffix, splitter_pos, false), win_size);
                win_pos += m_splitter_spacing;
                splitter_pos += m_splitter_spacing;
            }

            const ImVec2 cell_size = ImVec2(YGNodeLayoutGetWidth(YGNodeGetParent(node)), YGNodeLayoutGetHeight(node));

            render_node(node, win_pos);

            if (m_is_horizontal)
                splitter_pos.x += cell_size.x;
            else
                splitter_pos.y += cell_size.y;

            if (i + 1 == size_t(m_is_horizontal ? cols : rows))
                break;

            if (!is_flex && is_visible) {
                apply_size(node, splitter(node, suffix, splitter_pos), win_size);
                win_pos += m_splitter_spacing;
                splitter_pos += m_splitter_spacing;
            }
            is_flex_prev_child = is_flex;
        }
    }

    if (has_parent_window()) {
        old_pos.y += YGNodeLayoutGetHeight(m_root);
        ImGui::SetCursorScreenPos(old_pos);
    }
}

} 