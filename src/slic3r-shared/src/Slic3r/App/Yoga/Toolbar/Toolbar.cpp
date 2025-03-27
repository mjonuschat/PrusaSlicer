///|/ Copyright (c) Prusa Research 2018 - 2023 Enrico Turri @enricoturri1966, Oleksandra Iushchenko @YuSanka, Tomáš Mészáros @tamasmeszaros, Lukáš Matěna @lukasmatena, Vojtěch Bubník @bubnikv, David Kocík @kocikdav, Filip Sykala @Jony01, Lukáš Hejl @hejllukas, Vojtěch Král @vojtechkral

#include "Slic3r/App/Yoga/Toolbar/Toolbar.hpp"
#include "Slic3r/Log.hpp"
#include <chrono>
#include <Yoga.h>
#include "imgui/imgui_internal.h"

namespace Slic3r::App::Yoga::Toolbar {

#define debug 1

static void show(YGNodeRef node) {
    YGNodeStyleSetDisplay(node, YGDisplayFlex);
}

static void hide(YGNodeRef node) {
    YGNodeStyleSetDisplay(node, YGDisplayNone);
}

static bool is_shown(YGNodeRef node)
{
    return YGNodeStyleGetDisplay(node) == YGDisplayFlex;
}

static bool is_hidden(YGNodeRef node)
{
    return YGNodeStyleGetDisplay(node) == YGDisplayNone;
}

Toolbar::Toolbar(const std::string& name, float min_size /*= 25.f*/, float max_size /*= 50.f*/, Yoga::Align align/* = Yoga::Align()*/, Orientation orientation /*= Orient::Vertical*/)
{
    init(name, min_size, max_size, align, orientation);
}

Toolbar::~Toolbar()
{
    if (m_root)
        // Clean up Yoga
        YGNodeFreeRecursive(m_root);
}

void Toolbar::init(const std::string& name, float min_size /*= 25.f*/, float max_size /*= 50.f*/, Yoga::Align align/* = Yoga::Align()*/, Orientation orientation /*= Orient::Vertical*/)
{
    if (m_root) {
        // Clean up Yoga
        YGNodeFreeRecursive(m_root);
    }

    m_align         = align;
    m_name          = name;
    m_is_horizontal = orientation == Orientation::Horizontal;
    m_min_side      = min_size;
    m_max_side      = max_size > min_size ? max_size : min_size;

    m_root = YGNodeNew();
    YGNodeStyleSetFlexDirection(m_root, m_is_horizontal ? YGFlexDirectionRow : YGFlexDirectionColumn); // Set the direction to row
    YGNodeStyleSetAlignItems(m_root, YGAlignStretch);
    YGNodeStyleSetJustifyContent(m_root, YGJustify((m_is_horizontal ? m_align.get_yoga_h_align() : m_align.get_yoga_v_align()) - 1));
}

// create square flex node
YGNodeRef Toolbar::create_node()
{
    YGNodeRef node = YGNodeNew();

    YGNodeStyleSetWidth (node, m_min_side);
    YGNodeStyleSetHeight(node, m_min_side);

    YGNodeStyleSetMinWidth (node, m_min_side);
    YGNodeStyleSetMinHeight(node, m_min_side);

    YGNodeStyleSetMaxWidth (node, m_max_side);
    YGNodeStyleSetMaxHeight(node, m_max_side);

    YGNodeStyleSetAspectRatio(node, 1.f);

    YGNodeStyleSetFlex(node, 1.f);
    YGNodeStyleSetAlignSelf(node, m_is_horizontal ? m_align.get_yoga_v_align() : m_align.get_yoga_h_align());

    YGNodeStyleSetMargin(node, YGEdgeLeft,   m_h_margin);
    YGNodeStyleSetMargin(node, YGEdgeRight,  m_h_margin);
    YGNodeStyleSetMargin(node, YGEdgeTop,    m_v_margin);
    YGNodeStyleSetMargin(node, YGEdgeBottom, m_v_margin);

    return node;
}

// create square flex node
YGNodeRef Toolbar::create_separator_node(float size)
{
    YGNodeRef node = YGNodeNew();

    YGNodeStyleSetWidth (node, m_is_horizontal ? size : m_min_side);
    YGNodeStyleSetHeight(node, m_is_horizontal ? m_min_side : size);

    YGNodeStyleSetMaxWidth (node, m_is_horizontal ? size : m_max_side);
    YGNodeStyleSetMaxHeight(node, m_is_horizontal ? m_max_side : size);

    YGNodeStyleSetFlex(node, 1.f);
    YGNodeStyleSetAlignSelf(node, m_is_horizontal ? m_align.get_yoga_v_align() : m_align.get_yoga_h_align());

    return node;
}

size_t Toolbar::insert_pos()
{
    size_t pos = YGNodeGetChildCount(m_root);
    if (m_collapsed_node)
        pos--;
    return pos;
}

YGNodeRef Toolbar::add_node()
{
    YGNodeRef node = create_node();
    YGNodeInsertChild(m_root, node, insert_pos());

    update_min_size();
    return node;
}

YGNodeRef Toolbar::add_separator_node(float size)
{
    YGNodeRef node = create_separator_node(size);
    YGNodeInsertChild(m_root, node, insert_pos());

    m_separators_count++;

    update_min_size();
    return node;
}

YGNodeRef Toolbar::insert_node(int id)
{
    YGNodeRef node = create_node();
    YGNodeInsertChild(m_root, node, id);

    update_min_size();
    return node;
}

YGNodeRef Toolbar::insert_separator_node(int id, float size)
{
    YGNodeRef node = create_separator_node(size);
    YGNodeInsertChild(m_root, node, id);

    update_min_size();
    return node;
}

static void add_(std::map<YGNodeRef, Item>&  m_nodes, YGNodeRef new_node, const Item& new_item)
{
#if debug
    for (const auto& [node, item] : m_nodes)
        if (item.name() == new_item.name() && !new_item.is_separator()) {
            SPDLOG_INFO("!!! Yo try to add same node with name: {}",item.name());
            YGNodeRemoveChild(YGNodeGetParent(node), node);
            YGNodeFree(node);
            return;
        }
#endif
    m_nodes[new_node] = new_item;
}

Item& Toolbar::add(wchar_t icon, const std::string& tooltip, const std::string& shortcut, Callbacks callbacks)
{
    YGNodeRef node = add_node();
    int id = YGNodeGetChildCount(m_root) - m_separators_count;// -m_collapsed_count;
    if (m_collapsed_node)
        id--;

    add_(m_nodes, node, Item(icon, tooltip, shortcut, callbacks));

    if (m_collapsed_node && m_collapsed_count > 0) {
        hide(node);
        m_collapsed_count++;
        collapsed_item().add_sub_toolbar_item(m_nodes[node]);
    }
    layout();

    return m_nodes[node];
}

Item& Toolbar::insert(int id, wchar_t icon, const std::string& tooltip, const std::string& shortcut, Callbacks callbacks)
{
    YGNodeRef node = insert_node(id);
    add_(m_nodes, node, Item(icon, tooltip, shortcut, callbacks));
    layout();

    return m_nodes[node];
}

Item& Toolbar::add(wchar_t icon, Toolbar* sub_toolbar)
{
    YGNodeRef node = add_node();
    add_(m_nodes, node, Item(icon, sub_toolbar));
    layout();

    return m_nodes[node];
}

Item& Toolbar::add(const Item& item)
{
    YGNodeRef node;
    if (item.is_separator())
        node = add_separator_node(item.separator_size());
    else 
        node = add_node();

    add_(m_nodes, node, item);
    layout();

    return m_nodes[node];
}

Item& Toolbar::insert(int id, const Item& item)
{
#if debug
    if (m_nodes.size() != YGNodeGetChildCount(m_root))
        SPDLOG_INFO("!!! insert: Something is wrong!");
#endif
    YGNodeRef node;
    if (item.is_separator())
        node = insert_separator_node(id, item.separator_size());
    else
        node = insert_node(id);

    add_(m_nodes, node, item);
    layout();

    return m_nodes[node];
}

void Toolbar::erase(int id /*= -1*/)
{
    const int nodes_cnt = YGNodeGetChildCount(m_root);
    if (id < 0)
        id = nodes_cnt - (m_collapsed_node ? 2 : 1);
    if (id < 0)
        return;

    YGNodeRef node = YGNodeGetChild(m_root, id);

#if debug
    if (m_nodes.size() != nodes_cnt)
        SPDLOG_INFO("!!! erase: Something is wrong!");
    if (!node || node == m_collapsed_node) {
        SPDLOG_INFO("!!! Node is null or You try to erase collapsed item!");
        return;
    }
#endif

    if (m_nodes[node].is_separator())
        m_separators_count--;
    m_nodes.erase(node);

    // Remove the first child
    YGNodeRemoveChild(m_root, node);
    // Free the removed child
    YGNodeFree(node);

    if (m_collapsed_node && m_collapsed_count > 0) {
        // same node have to be deleted from sub_toolbar too
        int last_non_collapsed_id = nodes_cnt - 2 - m_collapsed_count;
        if (id > last_non_collapsed_id) {
            int sub_tb_id = id - last_non_collapsed_id - 1;
            assert(id >= 0);
            collapsed_item().erase_sub_toolbar_item(sub_tb_id);
        }
        if ((--m_collapsed_count) == 0)
            hide(m_collapsed_node);
    }

    update_min_size();
    layout();
}

void Toolbar::add_separator(float size)
{
    // suppress to add separator as a first node
    const int cnt = YGNodeGetChildCount(m_root);
    if (cnt == 0)
        return;
    // suppress to add separator next to other one
    YGNodeRef prev_node = YGNodeGetChild(m_root, cnt - (m_collapsed_node ? 2 : 1));
    if (m_nodes[prev_node].is_separator())
        return;

    YGNodeRef node = add_separator_node(size);
    add_(m_nodes, node, Item(size));
    layout();
}

void Toolbar::set_margins(float h_margin, float v_margin)
{
    m_h_margin = h_margin;
    m_v_margin = v_margin;

    for (size_t id = 0; id < YGNodeGetChildCount(m_root); id++) {
        YGNodeRef node = YGNodeGetChild(m_root, id);
        if (m_nodes[node].is_separator())
            continue;

        YGNodeStyleSetMargin(node, YGEdgeLeft, m_h_margin);
        YGNodeStyleSetMargin(node, YGEdgeRight, m_h_margin);
        YGNodeStyleSetMargin(node, YGEdgeTop, m_v_margin);
        YGNodeStyleSetMargin(node, YGEdgeBottom, m_v_margin);
    }
    update_min_size();
}

void Toolbar::finalize()
{
    layout();
    m_finalized = true;
}

void Toolbar::layout()
{
    YGNodeCalculateLayout(m_root, YGUndefined, YGUndefined, YGDirectionLTR);
    ensure_min_size();
}

void Toolbar::resize(ImVec2 win_size)
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

    // check max value in respect to the size

    float min_side = std::min(win_size.x, win_size.y);
    if (m_max_side > min_side && min_side > 0.f) {
        for (auto&[node, item] : m_nodes) {
            assert(node);
            if (item.is_separator()) {
                if (m_is_horizontal)
                    YGNodeStyleSetMaxHeight(node, min_side);
                else
                    YGNodeStyleSetMaxWidth(node, min_side);
            } else {
                YGNodeStyleSetMaxWidth (node, min_side);
                YGNodeStyleSetMaxHeight(node, min_side);
            }
        }
        force_recalc = true;
    }

    if (force_recalc)
        YGNodeCalculateLayout(m_root, YGUndefined, YGUndefined, YGDirectionLTR);

    ensure_min_size();
}

void Toolbar::ensure_min_size()
{
    bool force_recalc = false;
    if (YGNodeLayoutGetWidth(m_root) < m_min_size.width) {
        YGNodeStyleSetWidth(m_root, m_min_size.width);
        // add squash 
        force_recalc = true;
    }
    if (YGNodeLayoutGetHeight(m_root) < m_min_size.height) {
        YGNodeStyleSetHeight(m_root, m_min_size.height);
        // add squash 
        force_recalc = true;
    }
    if (force_recalc)
        YGNodeCalculateLayout(m_root, YGUndefined, YGUndefined, YGDirectionLTR);
} 

YGSize Toolbar::get_size(float side)
{
    size_t nodes_count = YGNodeGetChildCount(m_root);

    YGSize size = m_is_horizontal ? YGSize({ 0.f, (side + 2.f * m_v_margin) }) : YGSize({ (side + 2.f * m_h_margin), 0.f });

    for (size_t id = 0; id < nodes_count - m_collapsed_count; id++) {
        YGNodeRef node = YGNodeGetChild(m_root, id);
        if (is_hidden(node))
            continue;

        Item& item = m_nodes[node];
        if (m_is_horizontal)
            size.width += (item.is_separator() ? item.separator_size() : side) + 2.f * m_h_margin;
        else
            size.height += (item.is_separator() ? item.separator_size() : side) + 2.f * m_v_margin;
    }
    return size;
}

void Toolbar::update_min_size()
{
    m_min_size = get_size(m_min_side);
}

void Toolbar::collapse_if_needed()
{
    m_collapsed_node = add_node();
    int id = YGNodeGetChildCount(m_root) - m_separators_count;

    m_nodes[m_collapsed_node] = Item(ImGui::ToolbarEllipsis, "", "", {});
    m_nodes[m_collapsed_node].init_sub_toolbar(m_min_side, m_max_side, 
                                               m_is_horizontal ? Align({ AlignH::Right, m_align.vertical }) : Align({ m_align.horizontal, AlignV::Bottom }), 
                                               m_is_horizontal ? Orientation::Horizontal : Orientation::Vertical);

    // hide collapsed item bzy default
    hide(m_collapsed_node);
}

Item& Toolbar::collapsed_item()
{
    assert(m_collapsed_node);
    return m_nodes[m_collapsed_node];
}

void Toolbar::collapse_node(YGNodeRef node)
{
    hide(node);

    collapsed_item().insert_sub_toolbar_item(int(0), m_nodes[node]);
    m_collapsed_count++;

    if (is_hidden(m_collapsed_node))
        show(m_collapsed_node);
}

bool Toolbar::collapse(ImVec2 win_size, YGSize control_size)
{
    if (m_is_horizontal ? control_size.width <= win_size.x : control_size.height <= win_size.y)
        return false;    

    const float win_end_pos = (m_is_horizontal ? win_size.x : win_size.y);

    for (size_t id = 0; id < YGNodeGetChildCount(m_root); id++) {
        YGNodeRef node = YGNodeGetChild(m_root, id);

        if (is_hidden(node))
            continue;

        float item_end_pos = m_is_horizontal ? YGNodeLayoutGetLeft(node) + YGNodeLayoutGetWidth(node) :
                                               YGNodeLayoutGetTop(node)  + YGNodeLayoutGetHeight(node);
        if (item_end_pos <= win_end_pos)
            continue;
        if (id == 0)
            return false;

        if (node == m_collapsed_node) {
            for (--id; id > 1; --id) {
                if (is_shown(YGNodeGetChild(m_root, id))) {
                    collapse_node(YGNodeGetChild(m_root, id));
                    return true;
                }
            }
            return false;
        }

        for (size_t id_sub = YGNodeGetChildCount(m_root) - 2; id_sub >= id - 1; id_sub--) {
            YGNodeRef node = YGNodeGetChild(m_root, id_sub);
            if (is_shown(node))
                collapse_node(node);
        }

        if (m_collapsed_count == 1) {
            expand_node(YGNodeGetChild(m_root, YGNodeGetChildCount(m_root) - 2));
            return false;
        }

        return true;
    }

    return false;
}

void Toolbar::expand_node(YGNodeRef node)
{
    assert(is_hidden(node));
    show(node);

    collapsed_item().erase_sub_toolbar_item(int(0));
    m_collapsed_count--;

    if (m_collapsed_count == 0)
        hide(m_collapsed_node);
}

bool Toolbar::expand(ImVec2 win_size, YGSize size)
{
    if (m_collapsed_count == 0)
        return false;    

    bool force_recalc = false;
    
    const float margins     = (m_is_horizontal ? m_h_margin : m_v_margin) * 2.f;
    const float win_end_pos = m_is_horizontal ? win_size.x : win_size.y;
    const float side        = m_is_horizontal ? size.height : size.width;
    const size_t end_id     = YGNodeGetChildCount(m_root) - 2;

    float       end_pos     = m_is_horizontal ? size.width : size.height;

    for (size_t id = end_id - m_collapsed_count; id < end_id; id++) {
        YGNodeRef node = YGNodeGetChild(m_root, id);
        if (is_shown(node))
            continue;

        Item& item = m_nodes[node];
        end_pos += (item.is_separator() ? item.separator_size() : side) + margins;
        if (end_pos > win_end_pos)
            break;

        expand_node(node);
        force_recalc = true;
    }

    if (m_collapsed_count == 1) {
        expand_node(YGNodeGetChild(m_root, end_id));
        force_recalc = true;
    }

    return force_recalc;
}

void Toolbar::process_collapse(ImVec2 win_size)
{
    if (!m_collapsed_node || win_size.x <= 0.f || win_size.y <= 0.f)
        return;

    for (size_t id = 0; id < YGNodeGetChildCount(m_root); id++) {
        YGNodeRef node = YGNodeGetChild(m_root, id);
        if (is_shown(node)) {
            // fill side from first visible node
            YGSize size = get_size(m_is_horizontal ? YGNodeLayoutGetWidth(node) : YGNodeLayoutGetHeight(node));
            
            // check if toolbar needs to be collased/expanded
            if (collapse(win_size, size) || expand(win_size, size)) {
                YGNodeCalculateLayout(m_root, YGUndefined, YGUndefined, YGDirectionLTR);
                update_min_size();
                return;
            }
            break;
        }
    }
}

void Toolbar::render(Domain::Vec2f size, Domain::Vec2f pos)
{
    if (!m_finalized)
        finalize();

    static auto start_time = std::chrono::steady_clock::now();
    if (m_running)
        m_elapsed_time = std::chrono::duration<float>( std::chrono::steady_clock::now() - start_time).count();

    ImVec2 win_size = ImVec2(size.x(), size.y());
    ImVec2 win_pos  = ImVec2(pos.x(), pos.y());
    resize(win_size);

    if (win_pos.x < 0 && win_pos.y < 0)
        win_pos = ImGui::GetCursorScreenPos();

    process_collapse(win_size);

    ImRect bb = get_bb(win_pos);

    m_show_tooltips = false;
    size_t items_cnt = YGNodeGetChildCount(m_root);
    for (size_t i = 0; i < items_cnt; i++)
        m_show_tooltips |= render_node(i, win_pos, bb);
    
    if (m_show_tooltips) {
        if (!m_running) {
            start_time = std::chrono::steady_clock::now(); // Reset start time
            m_running = true; // Start the timer
        }

        if (m_elapsed_time > 0.5f || items_cnt == 1)
            for (size_t i = 0; i < items_cnt; i++)
                render_tooltip(i, win_pos);
    }
    else
        m_running = false;

}

ImRect Toolbar::get_bb(ImVec2 win_pos)
{
    size_t items_cnt = YGNodeGetChildCount(m_root);
    for (size_t id = 0; id < items_cnt; id++) {
        YGNodeRef   node = YGNodeGetChild(m_root, id);
        Item& item = m_nodes[node];

        if (item.is_separator() || is_hidden(node))
            continue;

        const ImVec2 item_pos = win_pos + ImVec2(YGNodeLayoutGetLeft(node), YGNodeLayoutGetTop(node));
        const float item_side = m_is_horizontal ? YGNodeLayoutGetWidth(node) : YGNodeLayoutGetHeight(node);

        YGSize size = get_size(item_side);
        if (m_collapsed_count > 1) {
            // expand size for size of collapsible item
            (m_is_horizontal ? size.width : size.height) += item_side;
        }

        return ImRect(item_pos, item_pos + ImVec2(size.width, size.height));
    }
    return ImRect();
}

int Toolbar::shown_items_cnt()
{
    int visible_cnt = 0;
    for (size_t i = 0; m_root && i < YGNodeGetChildCount(m_root); ++i) {
        YGNodeRef node = YGNodeGetChild(m_root, i);
        if (!m_nodes[node].is_separator() && m_nodes[node].is_visible() && is_shown(node))
            visible_cnt++;
    }
    return visible_cnt;
}

ImVec2 Toolbar::tooltip_pivot()
{
    if (m_is_horizontal)
        return ImVec2(m_align.horizontal == AlignH::Right ? 1.f : 0.f, m_align.vertical == AlignV::Bottom ? 1.f : 0.f);

    return ImVec2(m_align.horizontal == AlignH::Right ? 1.f : 0.f, 0.f);
}

ImDrawFlags Toolbar::corners_flag(int id)
{
    bool start_rounding = id == 0;
    if (!start_rounding && id > 0) {
        YGNodeRef prev_node = YGNodeGetChild(m_root, id - 1);
        start_rounding = prev_node && m_nodes[prev_node].is_separator();
    }

    int last_visible_node_id = YGNodeGetChildCount(m_root) - 1;
    if (m_collapsed_node && m_collapsed_count == 0)
        last_visible_node_id -= 1;
    bool stop_rounding = id == last_visible_node_id;
    if (!stop_rounding) {
        YGNodeRef post_node = YGNodeGetChild(m_root, id + 1);
        stop_rounding = post_node && m_nodes[post_node].is_separator();
    }

    return (start_rounding && stop_rounding) ? ImDrawFlags_RoundCornersAll :
            start_rounding ? (m_is_horizontal ? ImDrawFlags_RoundCornersLeft  : ImDrawFlags_RoundCornersTop) :
            stop_rounding  ? (m_is_horizontal ? ImDrawFlags_RoundCornersRight : ImDrawFlags_RoundCornersBottom) : ImDrawFlags_RoundCornersNone;
}

bool Toolbar::render_node(int id, ImVec2 win_pos, ImRect bb)
{
    YGNodeRef   node = YGNodeGetChild(m_root, id);
    Item&       item = m_nodes[node];

    if (item.is_separator() || !item.is_visible() || is_hidden(node))
        return false;

    const ImVec2 item_pos  = win_pos + ImVec2(YGNodeLayoutGetLeft(node), YGNodeLayoutGetTop(node));
    const ImVec2 item_size = ImVec2(YGNodeLayoutGetWidth(node), YGNodeLayoutGetHeight(node));

    return item.render(ImRect(item_pos, item_pos+item_size), bb, corners_flag(id), tooltip_pivot());
}

void Toolbar::render_tooltip(int id, ImVec2 win_pos, bool for_arrow /*= false*/)
{
    YGNodeRef node = YGNodeGetChild(m_root, id);
    Item& item = m_nodes[node];

    if (item.is_separator() || is_hidden(node))
        return;

    const ImVec2 item_size = ImVec2(YGNodeLayoutGetWidth(node), YGNodeLayoutGetHeight(node));
    ImVec2 tt_pos = win_pos + ImVec2(YGNodeLayoutGetLeft(node), YGNodeLayoutGetTop(node));

    if (m_is_horizontal) {
        // need to improve, if we will use it
        tt_pos.x += 5.f;
        if (m_align.vertical == AlignV::Bottom)
            tt_pos.y -= 5.f;
        else
            tt_pos.y += item_size.y + 5.f;
        if (for_arrow)
            tt_pos += ImVec2(item_size.x * 0.75f, 0.f);
        else {
            size_t visible_id = -1;
            for (size_t i = 0; i <= id; ++i) {
                YGNodeRef i_node = YGNodeGetChild(m_root, i);
                if (!m_nodes[i_node].is_separator() && is_shown(i_node))
                    visible_id++;
            }
        
            tt_pos += ImVec2(0.f, 1.5f * (visible_id - visible_id / 3 * 3) * ImGui::GetTextLineHeightWithSpacing()) * (m_align.vertical == AlignV::Bottom ? -1.f : 1.f);
        }
    }
    else {
        if (m_align.horizontal == AlignH::Right)
            tt_pos.x -= 5.f;
        else
            tt_pos.x += item_size.x + 5.f;
        tt_pos.y += (for_arrow ? 0.5f : 0.25f)* item_size.y;
    }

    item.render_tooltip(tt_pos, ImVec2(), tooltip_pivot());
    return;
}

} // namespace Slic3r::Yoga::Toolbar
