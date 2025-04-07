///|/ Copyright (c) Prusa Research 2018 - 2023 Enrico Turri @enricoturri1966, Oleksandra Iushchenko @YuSanka, Tomáš Mészáros @tamasmeszaros, Lukáš Matěna @lukasmatena, Vojtěch Bubník @bubnikv, David Kocík @kocikdav, Filip Sykala @Jony01, Lukáš Hejl @hejllukas, Vojtěch Král @vojtechkral

#include "Slic3r/App/Yoga/Toolbar/Toolbar.hpp"
#include "Slic3r/Log.hpp"
#include <chrono>
#include <Yoga.h>
#include "imgui/imgui_internal.h"
#include <libassert/assert.hpp>

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
    clear();
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
    if (m_subtoolbar_expander_node)
        pos--;
    return pos;
}

YGNodeRef Toolbar::add_node()
{
    YGNodeRef node = create_node();
    YGNodeInsertChild(m_root, node, insert_pos());

    return node;
}

YGNodeRef Toolbar::add_separator_node(float size)
{
    YGNodeRef node = create_separator_node(size);
    YGNodeInsertChild(m_root, node, insert_pos());

    return node;
}

YGNodeRef Toolbar::insert_node(int id)
{
    YGNodeRef node = create_node();
    YGNodeInsertChild(m_root, node, id);

    return node;
}

YGNodeRef Toolbar::insert_separator_node(int id, float size)
{
    YGNodeRef node = create_separator_node(size);
    YGNodeInsertChild(m_root, node, id);

    return node;
}

void Toolbar::add_item(std::map<YGNodeRef, Item>&  nodes, YGNodeRef new_node, const Item& new_item)
{
#if debug
    for (const auto& [node, item] : nodes)
        if (item.icon_name() == new_item.icon_name() && !new_item.is_separator()) {
            SPDLOG_ERROR("!!! Yo try to add same node with name: {}", std::to_string(item.icon_name()));
            YGNodeRemoveChild(YGNodeGetParent(node), node);
            YGNodeFree(node);
            return;
        }
#endif
    if (!new_item.is_visible())
        hide(new_node);

    nodes[new_node] = new_item;
    update_min_size();
}

Item& Toolbar::add(wchar_t icon, const std::string& tooltip, const std::string& shortcut, Callbacks callbacks)
{
    YGNodeRef node = add_node();
    add_item(m_nodes, node, Item(icon, tooltip, shortcut, callbacks)); 
    layout();

    return m_nodes[node];
}

Item& Toolbar::insert(int id, wchar_t icon, const std::string& tooltip, const std::string& shortcut, Callbacks callbacks)
{
    YGNodeRef node = insert_node(id);
    add_item(m_nodes, node, Item(icon, tooltip, shortcut, callbacks));
    layout();

    return m_nodes[node];
}

Item& Toolbar::add(wchar_t icon, Toolbar* sub_toolbar)
{
    YGNodeRef node = add_node();
    add_item(m_nodes, node, Item(icon, sub_toolbar));
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

    add_item(m_nodes, node, item);
    layout();

    return m_nodes[node];
}

Item& Toolbar::insert(int id, const Item& item)
{
#if debug
    if (m_nodes.size() != YGNodeGetChildCount(m_root))
        SPDLOG_ERROR("!!! insert: Something is wrong!");
#endif
    YGNodeRef node;
    if (item.is_separator())
        node = insert_separator_node(id, item.separator_size());
    else
        node = insert_node(id);

    add_item(m_nodes, node, item);
    layout();

    return m_nodes[node];
}

void Toolbar::erase(int id /*= -1*/)
{
    const int nodes_cnt = YGNodeGetChildCount(m_root);
    if (id < 0)
        id = nodes_cnt - (m_subtoolbar_expander_node ? 2 : 1);
    if (id < 0)
        return;

    YGNodeRef node = YGNodeGetChild(m_root, id);

#if debug
    if (m_nodes.size() != nodes_cnt)
        SPDLOG_ERROR("!!! erase: Something is wrong!");
    if (!node) {
        SPDLOG_ERROR("!!! Node you try to erase doesn't exist!");
        return;
    }
    if (node == m_subtoolbar_expander_node) {
        SPDLOG_ERROR("!!! You try to erase subtoolbar expander!");
        return;
    }
#endif

    m_nodes.erase(node);

    // Remove the first child
    YGNodeRemoveChild(m_root, node);
    // Free the removed child
    YGNodeFree(node);

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
    YGNodeRef prev_node = YGNodeGetChild(m_root, cnt - (m_subtoolbar_expander_node ? 2 : 1));
    if (m_nodes[prev_node].is_separator())
        return;

    YGNodeRef node = add_separator_node(size);
    add_item(m_nodes, node, Item(size));
    layout();
}

void Toolbar::clear()
{
    m_nodes.clear();

    while (m_root && YGNodeGetChildCount(m_root) > 0) {
        const auto child = YGNodeGetChild(m_root, 0);
        YGNodeRemoveChild(m_root, child);
        YGNodeFreeRecursive(child);
    }
}

bool Toolbar::is_empty() const
{
    return m_nodes.empty();
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

void Toolbar::resize(Vec2f win_size)
{
    bool force_recalc = false;
    if (win_size.x() >= m_min_size.x() && YGNodeLayoutGetWidth(m_root) != win_size.x()) {
        YGNodeStyleSetWidth(m_root, win_size.x());
        force_recalc = true;
    }
    if (win_size.y() >= m_min_size.y() && YGNodeLayoutGetHeight(m_root) != win_size.y()) {
        YGNodeStyleSetHeight(m_root, win_size.y());
        force_recalc = true;
    }

    // check max value in respect to the size

    float min_side = std::min(win_size.x(), win_size.y());
    if (m_max_side > min_side && min_side > 0.f) {
        for (auto&[node, item] : m_nodes) {
            ASSERT(node);
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
    if (YGNodeLayoutGetWidth(m_root) < m_min_size.x()) {
        YGNodeStyleSetWidth(m_root, m_min_size.x());
        force_recalc = true;
    }
    if (YGNodeLayoutGetHeight(m_root) < m_min_size.y()) {
        YGNodeStyleSetHeight(m_root, m_min_size.y());
        force_recalc = true;
    }
    if (force_recalc)
        YGNodeCalculateLayout(m_root, YGUndefined, YGUndefined, YGDirectionLTR);
} 

Vec2f Toolbar::get_size(float side)
{
    Vec2f size = m_is_horizontal ? Vec2f({ 0.f, (side + 2.f * m_v_margin) }) : Vec2f({ (side + 2.f * m_h_margin), 0.f });

    size_t nodes_count = YGNodeGetChildCount(m_root);
    for (size_t id = 0; id < nodes_count; id++) {
        YGNodeRef node = YGNodeGetChild(m_root, id);
        if (is_hidden(node))
            continue;

        Item& item = m_nodes[node];
        if (m_is_horizontal)
            size.x() += (item.is_separator() ? item.separator_size() : side) + 2.f * m_h_margin;
        else
            size.y() += (item.is_separator() ? item.separator_size() : side) + 2.f * m_v_margin;
    }

    return size;
}

void Toolbar::update_min_size()
{
    m_min_size = get_size(m_min_side);
}

void Toolbar::set_collapsible()
{
    m_subtoolbar_expander_node = add_node();

    m_nodes[m_subtoolbar_expander_node] = Item(ImGui::ToolbarEllipsis, "", "", { nullptr, [this]() {
        return m_show_subtoolbar_expander; } });
    m_nodes[m_subtoolbar_expander_node].init_sub_toolbar(m_name + "_sub", m_min_side, m_max_side,
                                               m_is_horizontal ? Align({ AlignH::Right, m_align.vertical }) : Align({ m_align.horizontal, AlignV::Bottom }), 
                                               m_is_horizontal ? Orientation::Horizontal : Orientation::Vertical);

    // hide collapsed item by default
    hide(m_subtoolbar_expander_node);
}

bool Toolbar::is_collapsible()
{
    return m_subtoolbar_expander_node != nullptr;
}

Item& Toolbar::subtoolbar_expander()
{
    ASSERT(is_collapsible());
    return m_nodes[m_subtoolbar_expander_node];
}

void Toolbar::process_items_visibility()
{
    bool force_layout_update{ false };

    const size_t items_cnt = YGNodeGetChildCount(m_root);
    for (size_t id = 0; id < items_cnt; id++) {
        YGNodeRef   node = YGNodeGetChild(m_root, id);
        bool visible = m_nodes[node].is_visible();

        if (visible && is_hidden(node)) {
            show(node);
            force_layout_update = true;
        }
        else if (!visible && is_shown(node)) {
            hide(node);
            force_layout_update = true;
        }
    }

    if (force_layout_update) {
        //update_min_size();
        //layout();

        bool has_subtoolbar = is_collapsible() && subtoolbar_expander().sub_toolbar_items_cnt() > 0;
        // Postpone the parent's layout update until the collapse is processed.
        refresh_full_layout(!has_subtoolbar);
    }
}

void Toolbar::refresh_full_layout(bool force_parent_layout/* = true*/)
{
    update_min_size();
    layout();
    if (force_parent_layout && m_cb_on_visible_items_changed) {
        // Propagate chages to the parent sizer
        m_cb_on_visible_items_changed();
    }
}

void Toolbar::collapse_from(size_t start_collapse_id)
{
    m_show_subtoolbar_expander = true;

    const size_t child_cnt = YGNodeGetChildCount(m_root) - 1;// Decrease the size for "subtoolbar_expander" node
    bool force_layout_update{ false };

    if (child_cnt - start_collapse_id != subtoolbar_expander().sub_toolbar_items_cnt()) {
        /* The count of collapsed items differs from the sub-toolbar item count,
         * so we need to update the sub-toolbar.
         * */
        Item& expander = subtoolbar_expander();
        expander.clear_sub_toolbar();
        for (size_t collapse_id = start_collapse_id; collapse_id < child_cnt; collapse_id++) {
            YGNodeRef node = YGNodeGetChild(m_root, collapse_id);
            expander.add_sub_toolbar_item(m_nodes[node]);
        }
    }

    // Hide collapsed items
    for (size_t collapse_id = start_collapse_id; collapse_id < child_cnt; collapse_id++) {
        YGNodeRef   node = YGNodeGetChild(m_root, collapse_id);
        if (is_shown(node)) {
            hide(node);
            force_layout_update = true;
        }
    }

    // Show subtoolbar_expander
    if (is_hidden(m_subtoolbar_expander_node)) {
        show(m_subtoolbar_expander_node);
        force_layout_update = true;
    }

    if (force_layout_update)
        refresh_full_layout();
}

void Toolbar::process_collapse(Vec2f win_size)
{
    auto force_subtoolbar_expander_hide = [this]() ->void {
        m_show_subtoolbar_expander = false;
        if (is_shown(m_subtoolbar_expander_node)) {
            hide(m_subtoolbar_expander_node);
            refresh_full_layout();
        }
    };

    const size_t child_cnt = YGNodeGetChildCount(m_root) - 1;// Decrease the size for "subtoolbar_expander" node

    float side = 0.f;
    for (size_t id = 0; id < child_cnt; id++) {
        YGNodeRef node = YGNodeGetChild(m_root, id); 
        // Get the side size from the first visible item
        if (is_shown(node)) {
            side = m_is_horizontal ? YGNodeLayoutGetWidth(node) : YGNodeLayoutGetHeight(node);
            break;
        }
    }

    if (side == 0.f) {
        // All items are invisible
        force_subtoolbar_expander_hide();
        return;
    }

    Vec2f size = get_size(side);

    const float win_end_pos = m_is_horizontal ? win_size.x() : win_size.y();
    const float tb_end_pos  = m_is_horizontal ? size.x()     : size.y();

    if (tb_end_pos <= win_end_pos) {
        // All items are expanded, and the entire toolbar is inside the window.
        force_subtoolbar_expander_hide();
        return;
    }

    size_t id = 0;
    for (id; id < child_cnt; id++) {
        YGNodeRef node = YGNodeGetChild(m_root, id);
        if (!m_nodes[node].is_visible()) {
            // Invisible items are ignored when collapsing.
            continue;
        }

        float item_end_pos = m_is_horizontal ? YGNodeLayoutGetLeft(node) + YGNodeLayoutGetWidth(node) :
                                               YGNodeLayoutGetTop(node) + YGNodeLayoutGetHeight(node);

        if (item_end_pos <= win_end_pos) {
            // The item is inside the window.
            continue;
        }
        break;
    }

    if (id == child_cnt) {
        // Since we are here, all items must be expanded
        force_subtoolbar_expander_hide();
    }
    else {
        /* Items from this one onward must be collapsed. However, the subtoolbar_expander will remain visible, 
         * so we need to collapse one previously visible item as well.
         * */
        for (size_t step_back = 1; step_back < id; step_back++) {
            const Item& item = m_nodes[YGNodeGetChild(m_root, id - step_back)];
            if (item.is_visible() && !item.is_separator()) {
                collapse_from(id - step_back);
                return;
            }
        }
    }
}

//#define RENDER_BG

void Toolbar::render(Vec2f win_size, Vec2f win_pos)
{
    if (!m_finalized)
        finalize();

    // Show or hide nodes based on item.is_visible().
    process_items_visibility();

    if (is_collapsible()) {
        // Check the toolbar size relative to the window size and collapse it if needed.
        process_collapse(win_size);
    }

    static auto start_time = std::chrono::steady_clock::now();
    if (m_running)
        m_elapsed_time = std::chrono::duration<float>( std::chrono::steady_clock::now() - start_time).count();

    resize(win_size);

    if (win_pos.x() < 0 && win_pos.y() < 0)
        win_pos = Vec2f(ImGui::GetCursorScreenPos().x, ImGui::GetCursorScreenPos().y);

    ImRect bb = get_bb(win_pos);

#ifdef RENDER_BG
    ImGui::SetNextWindowPos(ImVec2(win_pos.x()+YGNodeLayoutGetLeft(m_root), win_pos.y()+YGNodeLayoutGetTop(m_root)));
    ImGui::SetNextWindowSize(ImVec2(YGNodeLayoutGetWidth(m_root), YGNodeLayoutGetHeight(m_root)));
    //ImGui::SetNextWindowPos(bb.Min);
    //ImGui::SetNextWindowSize(bb.GetSize());
    ImGui::SetNextWindowBgAlpha(0.25f);

    // Discard current paddings and spacing of the window to corect apply of sizer's margins
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));

    ImGui::Begin(("sub_tt_win_" + m_name).c_str(), nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus);
#endif

    m_show_tooltips = false;
    size_t items_cnt = YGNodeGetChildCount(m_root);
    for (size_t i = 0; i < items_cnt; i++)
        m_show_tooltips |= render_node(i, win_pos, bb);

#ifdef RENDER_BG
    ImGui::End();
    // Revert current paddings and spacing
    ImGui::PopStyleVar(2);
#endif
    
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

ImRect Toolbar::get_bb(Vec2f win_pos)
{
    const size_t items_cnt = YGNodeGetChildCount(m_root);
    for (size_t id = 0; id < items_cnt; id++) {
        YGNodeRef   node = YGNodeGetChild(m_root, id);
        Item& item = m_nodes[node];

        if (is_hidden(node))
            continue;

        const Vec2f item_pos = win_pos + Vec2f(YGNodeLayoutGetLeft(node), YGNodeLayoutGetTop(node));
        const float item_side = m_is_horizontal ? YGNodeLayoutGetWidth(node) : YGNodeLayoutGetHeight(node);

        Vec2f size = get_size(item_side);

        return ImRect(item_pos.x(), item_pos.y(), item_pos.x() + size.x(), item_pos.y() + size.y());
    }
    return ImRect();
}

size_t Toolbar::items_cnt() const
{
    ASSERT(m_root);
    return YGNodeGetChildCount(m_root);
}

float Toolbar::get_flex_ration() const
{
    // Ratio will be calculated based on the visible items.

    float ratio { 0 };

    float visible_separators_total_size { 0.f };
    float visible_item_size{ 0.f };

    bool is_hidden = (is_horizontal() ? YGNodeLayoutGetWidth(m_root) : YGNodeLayoutGetHeight(m_root)) == 0.f;

    for (size_t i = 0; m_root && i < YGNodeGetChildCount(m_root); ++i) {
        const YGNodeRef node = YGNodeGetChild(m_root, i);
        if (visible_item_size == 0.f)
            visible_item_size = m_is_horizontal ? YGNodeLayoutGetHeight(node) : YGNodeLayoutGetWidth(node);

        const Item& item = m_nodes.at(node);
        /* An item can be hidden due to toolbar collapsing, in which case we only need to process the nodes 
         * that are actually shown. However, when the entire toolbar is hidden, 
         * we need to check visibility using the is_visible() callback.
        * */
        if (is_hidden ? item.is_visible() : is_shown(node)) {
            if (item.is_separator())
                visible_separators_total_size += item.separator_size();
            else
                ratio++;
        }
    }

    if (visible_item_size > 0.f && visible_separators_total_size) {
        /* The space needed for a separator is less than the space for an item.
         * So let's convert visible_separators_total_size into "visible items count" units 
         * and increase the ratio accordingly.
         * */ 
        ratio += visible_separators_total_size / visible_item_size;
    }

    return ratio;
}

Vec2f Toolbar::tooltip_pivot()
{
    if (m_is_horizontal)
        return Vec2f(m_align.horizontal == AlignH::Right ? 1.f : 0.f, m_align.vertical == AlignV::Bottom ? 1.f : 0.f);

    return Vec2f(m_align.horizontal == AlignH::Right ? 1.f : 0.f, 0.f);
}

ImDrawFlags Toolbar::corners_flag(int cur_id)
{
    bool start_rounding { true };
    for (int id = cur_id-1; id >= 0; id--) {
        YGNodeRef   node = YGNodeGetChild(m_root, size_t(id));
        if (is_shown(node)) {
            if (!m_nodes[node].is_separator())
                start_rounding = false;
            break;
        }
    }

    bool stop_rounding  { true };
    const size_t items_cnt = YGNodeGetChildCount(m_root);
    for (size_t id = cur_id + 1; id < items_cnt; id++) {
        YGNodeRef   node = YGNodeGetChild(m_root, id);
        if (is_shown(node)) {
            if (!m_nodes[node].is_separator())
                stop_rounding = false;
            break;
        }
    }

    return (start_rounding && stop_rounding) ? ImDrawFlags_RoundCornersAll :
            start_rounding ? (m_is_horizontal ? ImDrawFlags_RoundCornersLeft  : ImDrawFlags_RoundCornersTop) :
            stop_rounding  ? (m_is_horizontal ? ImDrawFlags_RoundCornersRight : ImDrawFlags_RoundCornersBottom) : ImDrawFlags_RoundCornersNone;
}

bool Toolbar::render_node(int id, Vec2f win_pos, ImRect bb)
{
    YGNodeRef   node = YGNodeGetChild(m_root, id);
    Item&       item = m_nodes[node];

    if (item.is_separator() || is_hidden(node))
        return false;

    const Vec2f item_pos  = win_pos + Vec2f(YGNodeLayoutGetLeft(node), YGNodeLayoutGetTop(node));
    const Vec2f item_size = Vec2f(YGNodeLayoutGetWidth(node), YGNodeLayoutGetHeight(node));

    ImRect item_bb(item_pos.x(), item_pos.y(), item_pos.x() + item_size.x(), item_pos.y() + item_size.y());
    Vec2f tt_pivot = tooltip_pivot();
    return item.render(item_bb, bb, corners_flag(id), { tt_pivot.x(), tt_pivot.y() });
}

void Toolbar::render_tooltip(int id, Vec2f win_pos, bool for_arrow /*= false*/)
{
    YGNodeRef node = YGNodeGetChild(m_root, id);
    Item& item = m_nodes[node];

    if (item.is_separator() || is_hidden(node))
        return;

    const Vec2f item_size = Vec2f(YGNodeLayoutGetWidth(node), YGNodeLayoutGetHeight(node));
    Vec2f tt_pos = win_pos + Vec2f(YGNodeLayoutGetLeft(node), YGNodeLayoutGetTop(node));

    if (m_is_horizontal) {
        // need to improve, if we will use it
        tt_pos.x() += 5.f;
        if (m_align.vertical == AlignV::Bottom)
            tt_pos.y() -= 5.f;
        else
            tt_pos.y() += item_size.y() + 5.f;
        if (for_arrow)
            tt_pos += Vec2f(item_size.x() * 0.75f, 0.f);
        else {
            size_t visible_id = -1;
            for (size_t i = 0; i <= id; ++i) {
                YGNodeRef i_node = YGNodeGetChild(m_root, i);
                if (!m_nodes[i_node].is_separator() && is_shown(i_node))
                    visible_id++;
            }
        
            tt_pos += Vec2f(0.f, 1.5f * (visible_id - visible_id / 3 * 3) * ImGui::GetTextLineHeightWithSpacing()) * (m_align.vertical == AlignV::Bottom ? -1.f : 1.f);
        }
    }
    else {
        if (m_align.horizontal == AlignH::Right)
            tt_pos.x() -= 5.f;
        else
            tt_pos.x() += item_size.x() + 5.f;
        tt_pos.y() += (for_arrow ? 0.5f : 0.25f) * item_size.y();
    }

    Vec2f tt_pivot = tooltip_pivot();
    item.render_tooltip({ tt_pos.x(), tt_pos.y() }, ImVec2(), { tt_pivot.x(), tt_pivot.y() });
    return;
}

} // namespace Slic3r::Yoga::Toolbar
