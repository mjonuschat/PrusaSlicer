///|/ Copyright (c) Prusa Research 2018 - 2023 Enrico Turri @enricoturri1966, Oleksandra Iushchenko @YuSanka, Tomáš Mészáros @tamasmeszaros, Lukáš Matěna @lukasmatena, Vojtěch Bubník @bubnikv, David Kocík @kocikdav, Filip Sykala @Jony01, Lukáš Hejl @hejllukas, Vojtěch Král @vojtechkral

#include "Slic3r/App/Yoga/FlexSizer.hpp"

#include <Yoga.h>
#include <string.h>

#include <imgui/imgui_internal.h>

namespace Slic3r::App::Yoga {

static YGNodeRef create_node(float width = -1.f, float height = -1.f)
{
    YGNodeRef node = YGNodeNew();

    if (width < 0.f)
        YGNodeStyleSetWidthAuto(node);
    else
        YGNodeStyleSetWidth(node, width);

    if (height < 0.f)
        YGNodeStyleSetHeightAuto(node);
    else
        YGNodeStyleSetHeight(node, height);

    return node;
}

static YGNodeRef add_node(YGNodeRef parent_node, float width = -1.f, float height = -1.f)
{
    // Create node
    YGNodeRef node = create_node(width, height);

    // Add child to parent node
    YGNodeInsertChild(parent_node, node, YGNodeGetChildCount(parent_node));

    return node;
};

static YGSize get_item_size(std::function<void(ImVec2, ImVec2)> render_node_fn, bool single_item = true)
{
    if (!render_node_fn)
        return YGSize({ 0.f, 0.f });

    ImVec2 old_pos = ImGui::GetCursorScreenPos();

    // render widget with 0 alpha and store thems size
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0);
        render_node_fn(ImVec2(), ImVec2());
    ImGui::PopStyleVar();

    if (single_item) {
        // just one control is rendering
        ImVec2 size = ImGui::GetItemRectSize();
        // move cursore to the prevoius line to get correct position of the last rendered item
        ImGui::SameLine();
        // get end of the last rendered item
        ImVec2 new_pos = ImGui::GetCursorScreenPos() - GImGui->Style.ItemSpacing;

        ImVec2 real_size = new_pos - old_pos;

        // reset cursor pos
        ImGui::SetCursorScreenPos(old_pos);

        return YGSize({ real_size.x > size.x ? real_size.x : size.x, size.y });
    }

    // for non-single items (panels) we are interesting in height of content
    ImVec2 size = ImGui::GetCursorScreenPos() - old_pos - ImVec2(0.f, GImGui->Style.ItemSpacing.y);

    // reset cursor pos
    ImGui::SetCursorScreenPos(old_pos);

    return YGSize({ ImMax(10.f, size.x), ImMax(10.f, size.y) });
}

static ImVec2 get_size(YGNodeRef node)
{
    return ImVec2 (YGNodeLayoutGetWidth(node), YGNodeLayoutGetHeight(node));
}

static ImVec2 get_render_pos(YGNodeRef node)
{
    YGNodeRef col_node = YGNodeGetParent(node);
    YGNodeRef child = YGNodeGetChild(node, 0);

    return ImVec2  (YGNodeLayoutGetLeft(col_node) + YGNodeLayoutGetLeft(child) + YGNodeLayoutGetLeft(node),
                    YGNodeLayoutGetTop (col_node) + YGNodeLayoutGetTop (child) + YGNodeLayoutGetTop (node));
}

FlexSizer::FlexSizer(int col_cnt, int row_cnt, ImVec2 min_size/* = ImVec2(0.f, 0.f)*/, ImVec2 margins/* = ImVec2(0.f, 0.f)*/)
{
    init(col_cnt, row_cnt, min_size, margins);
}

bool FlexSizer::is_inited()
{
    return m_root && YGNodeGetChildCount(m_root) > 0;
}

void FlexSizer::init(int col_cnt, int row_cnt, ImVec2 min_size/* = ImVec2(0.f, 0.f)*/, ImVec2 margins/* = ImVec2(0.f, 0.f)*/)
{
    if (m_root) {
        // Clean up Yoga
        YGNodeFreeRecursive(m_root);
        m_next_col = m_next_row = 0;
        m_node_rendering.clear();
    }

    m_h_margin = margins.x;
    m_v_margin = margins.y;
    m_min_size = YGSize({ min_size.x, min_size.y });

    m_root = create_node(min_size.x, min_size.y);
    YGNodeStyleSetFlexDirection(m_root, YGFlexDirectionRow);

    for (int i =0; i < col_cnt; i++) {
        // Create the column node
        YGNodeRef col_node = add_node(m_root);
        YGNodeStyleSetFlexDirection(col_node, YGFlexDirectionColumn);

        if (m_h_margin > 0.f) {
            YGNodeStyleSetMargin(col_node, YGEdgeLeft,  m_h_margin);
            YGNodeStyleSetMargin(col_node, YGEdgeRight, m_h_margin);
        }

        // Create the row nodes
        for (size_t j = 0; j < row_cnt; j++) {
            YGNodeRef row_node = add_node(col_node);
            YGNodeStyleSetFlexDirection(row_node, YGFlexDirectionRow);

            // To promise a same height for all cells in one row we have to set flex
            YGNodeStyleSetFlex(row_node, 1.f);

            if (m_v_margin > 0.f) {
                YGNodeStyleSetMargin(row_node, YGEdgeTop,    m_v_margin);
                YGNodeStyleSetMargin(row_node, YGEdgeBottom, m_v_margin);
            }
        }
    }
}

FlexSizer::~FlexSizer()
{
    if (m_root)
        // Clean up Yoga
        YGNodeFreeRecursive(m_root);
}

// get min size in respect to children
YGSize FlexSizer::get_min_size()
{
    YGSize result = YGSize({ 0.f, 0.f });

    int col_cnt = get_cols();
    int row_cnt = get_rows();

    for (int col = 0; col < col_cnt; col++) {
        YGNodeRef col_node = YGNodeGetChild(m_root, col);

        float width{ 0.f };
        for (size_t row = 0; row < row_cnt; row++) {
            YGNodeRef row_node = get_node(col, row);

            if (col == 0) {
                float height{ 0.f };
                for (int col_in = 0; col_in < col_cnt; col_in++) {
                    YGNodeRef in_node = YGNodeGetChild(get_node(col_in, row), 0);
                    float h = YGNodeStyleGetHeight(in_node).value;

                    if (height < h)
                        height = h;
                }
                result.height += height + 2 * m_v_margin;
            }

            YGNodeRef in_node = YGNodeGetChild(get_node(col, row), 0);
            float w = YGNodeStyleGetWidth(in_node).value;

            if (width < w)
                width = w;
        }
        result.width += width + 2 * m_h_margin;
    }

    return result;
}

void FlexSizer::add(std::function<void(ImVec2, ImVec2)> render_fn /*= nullptr*/, Align align/* = Align({})*/, const std::string& win_name_prefix/* = std::string()*/)
{
    int row = m_next_row;// current row, !!! get before get next node
    int col = m_next_col;// current col, !!! get before get next node

    YGNodeRef node = get_next_node();
    if (node == nullptr)
        return; // all nodes are already initialized

    YGSize sz = get_item_size(render_fn, win_name_prefix.empty());

    // add inside child to make the flex cell in both diraction
    YGNodeRef child = add_node(node, sz.width, sz.height);

    // node is aligned horizontaly
    YGNodeStyleSetAlignSelf(node, align.get_yoga_h_align());
    // child is aligned verticaly
    YGNodeStyleSetAlignSelf(child, align.get_yoga_v_align());

    // save render function
    m_node_rendering[node].render_fn = render_fn;
    if (!win_name_prefix.empty())
        m_node_rendering[node].win_name = win_name_prefix + "_col" + std::to_string(col) + "_row" + std::to_string(row);

    // If flexibility for this row is discard, then
    if (YGNodeStyleGetFlex(node) == 0.f) {
        // set min height for whole row of the parent sizer to correct layout
        for (col = 0; col < get_cols(); col++) {
            YGNodeRef row_node = get_node(col, row);
            const float min_h = YGNodeStyleGetMinHeight(row_node).value;
            if (std::isnan(min_h) || min_h < sz.height)
                YGNodeStyleSetMinHeight(row_node, sz.height);
        }
    }
}

void FlexSizer::add(FlexSizer& inner_sizer, const std::string& win_name_prefix /*= std::string()*/, Align align/* = {}*/)
{
    int row = m_next_row;// current row, !!! get before get next node
    int col = m_next_col;// current col, !!! get before get next node

    YGNodeRef node = get_next_node();
    if (node == nullptr)
        return; // all nodes are already initialized

    YGSize sz = inner_sizer.get_best_size();
    // add inside child to make the flex cell in both diraction
    YGNodeRef child = add_node(node, sz.width, sz.height);

    // node is aligned horizontaly
    YGNodeStyleSetAlignSelf(node, align.get_yoga_h_align());
    // child is aligned verticaly
    YGNodeStyleSetAlignSelf(child, align.get_yoga_v_align());

    // save render function
    m_node_rendering[node].render_fn = [&inner_sizer](ImVec2 size, ImVec2 pos) {
        inner_sizer.render(size, pos);
    };
    if (!win_name_prefix.empty())
        m_node_rendering[node].win_name = win_name_prefix + "_col" + std::to_string(col) + "_row" + std::to_string(row);

    // Set min height for whole row of the parent sizer to correct layout
    {
        auto min_sz = inner_sizer.get_min_size();

        // Node, that in this case MinHeight have to be decreased for minimal possible value of height
        float min_height = sz.height - min_sz.height;

        for (col = 0; col < get_cols(); col++) {
            YGNodeRef row_node = get_node(col, row);
            const float min_h = YGNodeStyleGetMinHeight(row_node).value; 
            if (std::isnan(min_h) || min_h < min_height)
                YGNodeStyleSetMinHeight(row_node, min_height);
        }
    }
}

void FlexSizer::finalize()
{
    YGNodeCalculateLayout(m_root, YGUndefined, YGUndefined, YGDirectionLTR);

    // Set best min size -> exactly minimal size of the sizer
    // it can be bigger, the users input in constructor
    auto best_min_size = get_min_size();

    if (m_min_size.width < best_min_size.width)
        m_min_size.width = best_min_size.width;
    if (m_min_size.height < best_min_size.height)
        m_min_size.height = best_min_size.height;

    ensure_min_size();

    m_finalized = true;
}

void FlexSizer::layout()
{
    YGNodeCalculateLayout(m_root, YGUndefined, YGUndefined, YGDirectionLTR);
    ensure_min_size();
}

void FlexSizer::resize(ImVec2 win_size)
{
    bool force_recalc = false;
    if (win_size.x >= m_min_size.width && YGNodeLayoutGetWidth(m_root) != win_size.x) {
        YGNodeStyleSetWidth(m_root, win_size.x);
        force_recalc = true;
    }
    if (win_size.y >= m_min_size.height && YGNodeLayoutGetHeight(m_root) != win_size.y) {
        YGNodeStyleSetHeight(m_root, win_size.y);
        force_recalc = true;
    }

    if (force_recalc)
        YGNodeCalculateLayout(m_root, YGUndefined, YGUndefined, YGDirectionLTR);

    ensure_min_size();
}

void FlexSizer::ensure_min_size()
{
    bool force_recalc = false;
    if (YGNodeLayoutGetWidth(m_root) < m_min_size.width) {
        YGNodeStyleSetWidth(m_root, m_min_size.width);
        force_recalc = true;
    }
    if (YGNodeLayoutGetHeight(m_root) < m_min_size.height) {
        YGNodeStyleSetHeight(m_root, m_min_size.height);
        force_recalc = true;
    }
    if (force_recalc)
        YGNodeCalculateLayout(m_root, YGUndefined, YGUndefined, YGDirectionLTR);
}

// used just for debugging
void FlexSizer::render_nodes_bg(ImVec2 win_pos)
{
    int cols = get_cols();
    int rows = get_rows();
    
    auto start = win_pos;
    auto stop = start + get_size(m_root);

    ImGui::RenderFrame(start, stop, IM_COL32(20, 220, 200, 25), false);

    for (int col = 0; col < cols; col++) {

        YGNodeRef node = YGNodeGetChild(m_root, col);
        if (node) {
            auto pos = ImVec2(YGNodeLayoutGetLeft(m_root) + YGNodeLayoutGetLeft(node),
                YGNodeLayoutGetTop(m_root) + YGNodeLayoutGetTop(node));
            auto start = win_pos + pos;
            auto stop = start + get_size(node);

            ImGui::RenderFrame(start, stop, IM_COL32(220, 20, 200, 25), false);

            for (int row = 0; row < rows; row++) {
                YGNodeRef node_row = get_node(col, row);
                if (node_row) {
                    auto start_row = start + ImVec2(YGNodeLayoutGetLeft(node_row), YGNodeLayoutGetTop(node_row));
                    stop = start_row + get_size(node_row);

                    ImGui::RenderFrame(start_row, stop, IM_COL32(200, 220, 20, 35), false);
                }
            }
        }
    }
}

bool FlexSizer::has_parent_window()
{
    ImGuiWindow* parent_window = GImGui->CurrentWindow;
    std::string parent_win_name = parent_window ? parent_window->Name : "";
    return parent_win_name != "Debug##Default";
}

void FlexSizer::render_node(YGNodeRef node, ImVec2 win_pos)
{
    if (node && YGNodeStyleGetDisplay(node) != YGDisplayNone) {
        // Send to the render function position and size of whole cell to use it, if needed
        YGNodeRef col_node = YGNodeGetParent(node);

        if (YGNodeStyleGetDisplay(col_node) == YGDisplayNone)
            return;

        const ImVec2 cell_pos = win_pos + ImVec2(YGNodeLayoutGetLeft(col_node) + YGNodeLayoutGetLeft(node),
                                                 YGNodeLayoutGetTop (col_node) + YGNodeLayoutGetTop (node));
        const ImVec2 cell_size = ImVec2(YGNodeLayoutGetWidth(col_node), YGNodeLayoutGetHeight(node));

        std::function<void(ImVec2, ImVec2)> render_fn = m_node_rendering[node].render_fn;

        if (has_parent_window())
            ImGui::SetCursorScreenPos(win_pos + get_render_pos(node));

        bool begin_separate_window = !m_node_rendering[node].win_name.empty() && render_fn;
        if (begin_separate_window) {
            ImGui::SetNextWindowPos(cell_pos);
            ImGui::SetNextWindowSize(cell_size);
            ImGui::SetNextWindowBgAlpha(m_bg_alpha);

            // Discard current paddings and spacing of the window to corect apply of sizer's margins
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));

            ImGui::Begin(m_node_rendering[node].win_name.c_str(), nullptr,
                ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoFocusOnAppearing);
        }

        if (render_fn)
            render_fn(cell_size, cell_pos);

        if (begin_separate_window) {
            ImGui::End();
            // Revert current paddings and spacing
            ImGui::PopStyleVar(2);
        }
    }
}

void FlexSizer::render(ImVec2 win_size/* = ImVec2()*/, ImVec2 win_pos /*= ImVec2(-1.f, -1.f)*/)
{
    if (!m_finalized)
        finalize();

    resize(win_size);

    if (win_pos.x < 0 && win_pos.y < 0)
        win_pos = ImGui::GetCursorScreenPos();

    if (m_show_node_shapes)
        render_nodes_bg(win_pos);

    int cols = get_cols();
    int rows = get_rows();

    ImVec2 old_pos = ImGui::GetCursorScreenPos();

    for (int row = 0; row < rows; row++) {
        for (int col = 0; col < cols; col++) {
            render_node(get_node(col, row), win_pos);
        }
    }

    if (has_parent_window()) {
        old_pos.y += YGNodeLayoutGetHeight(m_root);
        ImGui::SetCursorScreenPos(old_pos);
    }
}

YGSize  FlexSizer::get_best_size()
{
    finalize();

    return YGSize({ YGNodeLayoutGetWidth(m_root), YGNodeLayoutGetHeight(m_root) });
}

int FlexSizer::get_cols() const 
{
    return YGNodeGetChildCount(m_root);
}

int FlexSizer::get_rows() const
{
    return YGNodeGetChildCount(YGNodeGetChild(m_root, 0));
}

YGNodeRef FlexSizer::get_node(int col, int row) const
{
    return YGNodeGetChild(YGNodeGetChild(m_root, col), row);
}

YGNodeRef FlexSizer::get_next_node()
{
    if (m_next_col < get_cols() && m_next_row < get_rows()) {
        YGNodeRef node = get_node(m_next_col, m_next_row);

        m_next_col++;
        if (m_next_col == get_cols()) {
            m_next_row++;
            m_next_col = 0;
        }

        return node;
    }

    return nullptr;
}

void FlexSizer::set_grow_col(int col, float grow /*= 1.f*/)
{
    if (auto col_node = YGNodeGetChild(m_root, col)) {
        YGNodeStyleSetFlexBasis(col_node, 0); // discard default flex for columns
        YGNodeStyleSetFlexGrow(col_node, grow);
    }
}

void FlexSizer::set_grow_row(int row, float grow /*= 1.f*/)
{
    int cols = get_cols();

    float max_height = 0.f;
    // if flexibility have to be discard, then find max height for this row
    if (grow == 0.f) {
        for (int col = 0; col < get_cols(); col++) {
            if (YGNodeRef row_node_child = YGNodeGetChild(get_node(col, row), 0)) {
                const float h = YGNodeStyleGetHeight(row_node_child).value;
                if (max_height < h)
                    max_height = h;
            }
        }
    }

    for (int col = 0; col < cols; col++) {
        if (auto row_node = get_node(col, row)) {
            YGNodeStyleSetFlexGrow(row_node, grow);

            if (grow == 0.f) {
                // discard felxibility 
                YGNodeStyleSetFlex(row_node, 0.f);

                if (max_height > 0.f) {
                    // Set min height for whole row of the parent sizer to correct layout
                    const float min_h = YGNodeStyleGetHeight(YGNodeGetChild(row_node, 0)).value;
                    if (std::isnan(min_h) || min_h < max_height)
                        YGNodeStyleSetMinHeight(row_node, max_height);
                }
            }
        }
    }
}

void FlexSizer::show_col(int col, bool show /*= true*/)
{
    if (auto col_node = YGNodeGetChild(m_root, col)) {
        if (show)
            YGNodeStyleSetDisplay(col_node, YGDisplayFlex); // Show the node
        else
            YGNodeStyleSetDisplay(col_node, YGDisplayNone); // Hide the node
    }
    YGNodeCalculateLayout(m_root, YGUndefined, YGUndefined, YGDirectionLTR);
}

bool FlexSizer::is_shown_col(int col)
{
    if (auto col_node = YGNodeGetChild(m_root, col))
        return YGNodeStyleGetDisplay(col_node) == YGDisplayFlex;

    return false;
}

void FlexSizer::show_row(int row, bool show /*= true*/)
{
    int cols = get_cols();

    for (int col = 0; col < cols; col++) {
        if (auto row_node = get_node(col, row)) {
            if (show)
                YGNodeStyleSetDisplay(row_node, YGDisplayFlex); // Show the node
            else
                YGNodeStyleSetDisplay(row_node, YGDisplayNone); // Hide the node
        }
    }
    YGNodeCalculateLayout(m_root, YGUndefined, YGUndefined, YGDirectionLTR);
}

bool FlexSizer::is_shown_row(int row)
{
    bool is_shown{ false };

    int cols = get_cols();
    for (int col = 0; col < cols; col++) {
        if (auto row_node = get_node(col, row))
            is_shown |= YGNodeStyleGetDisplay(row_node) == YGDisplayFlex;
    }
    return is_shown;
}

void FlexSizer::align_col(int col, Align align/* = Align({})*/)
{
    if (auto col_node = YGNodeGetChild(m_root, col)) {
        for (int row = 0; row < get_rows(); row++)
            align_cell(col, row, align);
    }
}

void FlexSizer::align_row(int row, Align align/* = Align({})*/)
{
    for (int col = 0; col < get_cols(); col++) {
        align_cell(col, row, align);
    }
}

void FlexSizer::align_cell(int col, int row, Align align/* = Align({})*/)
{
    if (auto node = get_node(col, row)) {
        // node is aligned horizontaly
        YGNodeStyleSetAlignSelf(node, align.get_yoga_h_align());
        // child is aligned verticaly
        YGNodeStyleSetAlignSelf(YGNodeGetChild(node, 0), align.get_yoga_v_align());
    }
}

} // namespace Slic3r::Yoga
