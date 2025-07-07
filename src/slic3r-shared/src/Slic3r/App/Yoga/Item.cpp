///|/ Copyright (c) Prusa Research 2018 - 2025 Oleksandra Iushchenko @YuSanka, Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/Yoga/Item.hpp"

#include "Slic3r/Assert.hpp"
#include "Slic3r/App/Render/ImguiRender.hpp"
#include "Slic3r/App/Yoga/ItemEvents.hpp"

#include <imgui_internal.h>
#include <string>

namespace Slic3r::App::Yoga {

static std::string yoga_position_to_string(YGPositionType position_type)
{
    std::string result = "'";

    switch (position_type) {
    case YGPositionTypeStatic:
        result += "static";
        break;
    case YGPositionTypeAbsolute:
        result += "absolute";
        break;
    case YGPositionTypeRelative:
        result += "relative";
        break;
    }

    result += "'";

    return result;
}

static std::string yoga_flex_direction(YGFlexDirection flex_direction)
{
    std::string result = "'";

    switch (flex_direction) {
    case YGFlexDirectionColumn:
        result += "column";
        break;
    case YGFlexDirectionColumnReverse:
        result += "column-reverse";
        break;
    case YGFlexDirectionRow:
        result += "row";
        break;
    case YGFlexDirectionRowReverse:
        result += "row-reverse";
        break;
    }

    result += "'";

    return result;
}

static std::string yoga_align_to_string(YGAlign align)
{
    std::string result = "'";

    switch (align) {
    case YGAlignAuto:
        result += "auto";
        break;
    case YGAlignCenter:
        result += "center";
        break;
    case YGAlignFlexEnd:
        result += "flex-end";
        break;
    case YGAlignFlexStart:
        result += "flex-start";
        break;
    case YGAlignSpaceAround:
        result += "space-around";
        break;
    case YGAlignSpaceBetween:
        result += "space-between";
        break;
    case YGAlignSpaceEvenly:
        result += "space-evenly";
        break;
    case YGAlignBaseline:
        result += "baseline";
        break;
    case YGAlignStretch:
        result += "stretch";
        break;
    }

    result += "'";

    return result;
}

static std::string yoga_justify_to_string(YGJustify justify)
{
    std::string result = "'";

    switch (justify) {
    case YGJustifyCenter:
        result += "center";
        break;
    case YGJustifyFlexEnd:
        result += "flex-end";
        break;
    case YGJustifyFlexStart:
        result += "flex-start";
        break;
    case YGJustifySpaceAround:
        result += "space-around";
        break;
    case YGJustifySpaceBetween:
        result += "space-between";
        break;
    case YGJustifySpaceEvenly:
        result += "space-evenly";
        break;
    }

    result += "'";

    return result;
}

Render::ImguiRender* Slic3r::App::Yoga::Item::m_imgui_render = nullptr;

std::unordered_map<std::string, int> Item::m_item_names = {};

Item::Item()
{
    m_node = YGNodeNew();

    YGNodeStyleSetFlexDirection(m_node, m_flex_direction);
    YGNodeStyleSetDirection(m_node, m_direction);
    YGNodeStyleSetFlexShrink(m_node, m_flex_shrink);
}

Item::~Item()
{
    if (!m_parent && m_node) {
        YGNodeFreeRecursive(m_node);
    }
}

void Item::render(Vec2f pos, Vec2f size)
{
    render_item_begin(pos, size);

    render_item_end(pos, size);
}

void Item::process_events(Vec2f pos, Vec2f size)
{
    for (const ItemPtr& child : m_children) {
        process_events_node(pos, child.get());
    }
}

YGNodeRef Item::node() const { return m_node; }

Item* Item::parent() const { return m_parent; }

float Item::x() const { return YGNodeLayoutGetLeft(m_node); }

float Item::y() const { return YGNodeLayoutGetTop(m_node); }

float Item::z() const { return m_z; }

float Item::width() const { return YGNodeLayoutGetWidth(m_node); }

float Item::height() const { return YGNodeLayoutGetHeight(m_node); }

float Item::left() const { return YGNodeLayoutGetLeft(m_node); }

float Item::right() const { return YGNodeLayoutGetRight(m_node); }

float Item::top() const { return YGNodeLayoutGetTop(m_node); }

float Item::bottom() const { return YGNodeLayoutGetBottom(m_node); }

const Vec2f& Item::min_size() const { return m_min_size; }

const Vec2f& Item::max_size() const { return m_max_size; }

bool Item::is_visible() const { return YGNodeStyleGetDisplay(m_node) == YGDisplayFlex; }

bool Item::debug_border() const { return m_debug_border; }

float Item::flex_grow() const { return m_flex_grow; }

float Item::flex_shrink() const { return m_flex_shrink; }

YGDirection Item::direction() const { return m_direction; }

float Item::aspect_ratio() const { return m_aspect_ratio; }

YGJustify Item::justify_content() const { return m_justify_content; }

YGPositionType Item::position_type() const { return m_position_type; }

YGAlign Item::align_items() const { return m_align_items; }

YGAlign Item::align_content() const { return m_align_content; }

const Margins& Item::margin() const { return m_margin; }

const Paddings& Item::padding() const { return m_padding; }

float Item::gap() const { return m_gap; }

Orientation Item::orientation() const { return m_orientation; }

YGWrap Item::flex_wrap() const { return YGNodeStyleGetFlexWrap(m_node); }

bool Item::enabled()
{
    Item* item = this;
    while (item) {
        if (!item->m_enabled) {
            return false;
        }

        item = item->m_parent;
    }

    return true;
}

void Item::set_enabled(bool enabled)
{
    if (m_enabled != enabled) {
        m_enabled = enabled;
        enabled_updated_internal();
    }
}

void Item::set_max_size(const Vec2f max_size)
{
    if (m_max_size != max_size) {
        m_max_size = max_size;
        YGNodeStyleSetMaxWidth(m_node, m_max_size.x());
        YGNodeStyleSetMaxHeight(m_node, m_max_size.y());
        set_style_dirty();
    }
}

void Item::set_visible(bool visible)
{
    if (m_visible != visible) {
        m_visible = visible;
        YGNodeStyleSetDisplay(m_node, visible ? YGDisplayFlex : YGDisplayNone);
        set_style_dirty();
    }
}

void Item::set_width(float width) { YGNodeStyleSetWidth(m_node, width); }

void Item::set_height(float height) { YGNodeStyleSetHeight(m_node, height); }

void Item::set_width_percent(float width_percent)
{
    YGNodeStyleSetWidthPercent(m_node, width_percent);
}

void Item::set_height_percent(float height_percent)
{
    YGNodeStyleSetHeightPercent(m_node, height_percent);
}

void Item::set_left(float left) { YGNodeStyleSetPosition(m_node, YGEdgeLeft, left); }

void Item::set_right(float right) { YGNodeStyleSetPosition(m_node, YGEdgeRight, right); }

void Item::set_top(float top) { YGNodeStyleSetPosition(m_node, YGEdgeTop, top); }

void Item::set_bottom(float bottom) { YGNodeStyleSetPosition(m_node, YGEdgeBottom, bottom); }

void Item::set_flex(float flex) { YGNodeStyleSetFlex(m_node, flex); }

void Item::set_flex_wrap(YGWrap wrap) { YGNodeStyleSetFlexWrap(m_node, wrap); }

void Item::remove_later(Item* child)
{
    ASSERT(child);
    ASSERT(index_of(child).has_value());
    push_event(std::make_unique<RemoveEvent>(child));
}

void Item::move_later(Item* target, size_t index)
{
    push_event(std::make_unique<MoveEvent>(this, target, index));
}

std::vector<Item*> Item::items() const
{
    std::vector<Item*> result;
    result.reserve(m_children.size());
    std::transform(
        m_children.cbegin(), m_children.cend(), std::back_inserter(result),
        [](const ItemPtr& child) { return child.get(); }
    );
    return result;
}

std::optional<size_t> Item::index_of(Item* item) const
{
    std::vector<ItemPtr>::const_iterator it =
        std::find_if(m_children.cbegin(), m_children.cend(), [item](const ItemPtr& child) {
            return item == child.get();
        });
    std::optional<size_t> result;
    if (it != m_children.cend()) {
        result = std::distance(m_children.cbegin(), it);
    }
    return result;
}

void Item::set_debug_border(bool show_debug_border) { m_debug_border = show_debug_border; }

std::string Item::debug_dump_tree() const
{
    std::string dump;

    dump += "<Node style={{";

    dump += "minWidth: " + std::to_string(YGNodeStyleGetMinWidth(m_node).value) + ", ";
    dump += "minHeight: " + std::to_string(YGNodeStyleGetMinHeight(m_node).value) + ", ";
    // if (m_max_size.x() != YGUndefined) {
    //     dump += "maxWidth: " + std::to_string(YGNodeStyleGetMaxWidth(m_node).value) + ", "
    // }
    // if (m_max_size.y() != YGUndefined) {
    //     dump += "maxHeight: " + std::to_string(YGNodeStyleGetMaxHeight(m_node).value) + ", ";
    // }
    if (m_padding.left > 0)
        dump += "paddingLeft: " + std::to_string(YGNodeStyleGetPadding(m_node, YGEdgeLeft).value) +
            ", ";
    if (m_padding.right > 0)
        dump += "paddingRight: " +
            std::to_string(YGNodeStyleGetPadding(m_node, YGEdgeRight).value) + ", ";
    if (m_padding.left > 0)
        dump += "paddingTop: " + std::to_string(YGNodeStyleGetPadding(m_node, YGEdgeTop).value) +
            ", ";
    if (m_padding.bottom > 0)
        dump += "paddingBottom: " +
            std::to_string(YGNodeStyleGetPadding(m_node, YGEdgeBottom).value) + ", ";
    if (m_margin.left > 0)
        dump += "marginLeft: " + std::to_string(YGNodeStyleGetMargin(m_node, YGEdgeLeft).value) +
            ", ";
    if (m_margin.right > 0)
        dump += "marginRight: " + std::to_string(YGNodeStyleGetMargin(m_node, YGEdgeRight).value) +
            ", ";
    if (m_margin.top > 0)
        dump += "marginTop: " + std::to_string(YGNodeStyleGetMargin(m_node, YGEdgeTop).value) + ", ";
    if (m_margin.bottom > 0)
        dump += "marginBottom: " +
            std::to_string(YGNodeStyleGetMargin(m_node, YGEdgeBottom).value) + ", ";

    dump += "flexGrow: " + std::to_string(YGNodeStyleGetFlexGrow(m_node)) + ", ";
    // if (m_aspect_ratio != YGUndefined) {
    //     dump += "aspectRatio: " + std::to_string(YGNodeStyleGetAspectRatio(m_node)) + ", ";
    // }
    dump += "alignSelf: " + yoga_align_to_string(m_self_align) + ", ";
    dump += "flexDirection: " + yoga_flex_direction(m_flex_direction) + ", ";
    dump += "gap: " + std::to_string(YGNodeStyleGetGap(m_node, YGGutterAll)) + ", ";
    dump += "positionType: " + yoga_position_to_string(m_position_type) + ", ";
    dump += "justifyContent: " + yoga_justify_to_string(m_justify_content) + ", ";
    dump += "alignItems: " + yoga_align_to_string(m_align_items) + ", ";
    dump += "alignContent: " + yoga_align_to_string(m_align_content) + ", ";

    dump += "}}>\n";

    for (const ItemPtr& child : m_children) {
        dump += child->debug_dump_tree();
    }

    dump += "</Node>\n";

    return dump;
}

void Item::invalidate_min_size_calculation()
{
    m_min_size_calculated = false;
    m_min_size = Vec2f();
}

ImVec2 Item::to_im(const Vec2f& val) { return ImVec2(val.x(), val.y()); }

Vec2f Item::from_im(const ImVec2& val) { return Vec2f(val.x, val.y); }

bool Item::is_node_visible(YGNodeRef node)
{
    while (node) {
        if (YGNodeStyleGetDisplay(node) == YGDisplayNone) {
            return false;
        }
        node = YGNodeGetParent(node);
    }
    return true;
}

Vec2f Item::get_item_size()
{
    ImVec2 old_pos = ImGui::GetCursorPos();
    ImGui::SetCursorPos({});

    // render widget with 0 alpha and store thems size
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0);
    render({}, {});
    ImGui::PopStyleVar();

    // for non-single items (panels) we are interesting in height of content
    ImVec2 cur_pos_prev = ImGui::GetCursorPos();
    ImGui::SameLine();
    ImVec2 cur_pos = ImGui::GetCursorPos();

    // if we render literally nothing, set our size to nothing, otherwise use whatever ImGui gave us
    ImVec2 size = cur_pos_prev.x == 0 && cur_pos_prev.y == 0
        ? cur_pos_prev
        : ImVec2{cur_pos.x, cur_pos_prev.y + GImGui->Style.ItemSpacing.y};

    Vec2f result = Vec2f{ImMax(10.f, size.x), ImMax(10.f, size.y)};

    // reset cursor pos
    ImGui::SetCursorPos(old_pos);

    return result;
}

void Item::push_event(std::unique_ptr<Event> event)
{
    ASSERT(m_parent);
    m_parent->push_event(std::move(event));
}

void Item::enabled_updated_internal() {}

Vec2f Item::get_available_size() const { return m_parent->get_available_size(); }

void Item::on_resized() {}

Vec2f Item::get_global_pos() const
{
    const Vec2f pos{left(), top()};

    return m_parent ? pos + m_parent->get_global_pos() : pos;
};

void Item::set_imgui_render(Render::ImguiRender* imgui_render) { m_imgui_render = imgui_render; }

void Item::add_child(ItemPtr child, size_t index)
{
    ASSERT(child);
    ASSERT(child->node());
    ASSERT(!index_of(child.get()).has_value(), "Child is already parented to this item");
    ASSERT(index <= m_children.size(), "Invalid child index");

    child->m_parent = this;

    YGNodeInsertChild(m_node, child->m_node, index);
    m_children.insert(m_children.cbegin() + index, std::move(child));
    update_children_render_order();
    set_style_dirty();
}

ItemPtr Item::remove_child(Item* child)
{
    ASSERT(child);
    ASSERT(child->node());

    std::vector<ItemPtr>::iterator it =
        std::find_if(m_children.begin(), m_children.end(), [child](const ItemPtr& child_item) {
            return child_item.get() == child;
        });

    ASSERT(it != m_children.end(), "Trying to remove unmaintained child");
    if (it == m_children.end()) {
        return nullptr;
    }

    child->m_parent = nullptr;

    ItemPtr result(std::move(*it));

    YGNodeRemoveChild(m_node, child->node());
    m_children.erase(it);
    update_children_render_order();
    set_style_dirty();

    return result;
}

void Item::update_children_render_order()
{
    m_children_render_order.clear();
    std::transform(
        m_children.cbegin(), m_children.cend(), std::back_inserter(m_children_render_order),
        [](const ItemPtr& child) { return child.get(); }
    );
    // sort children by Z layer, higher Z shall be rendered first
    std::sort(
        m_children_render_order.begin(), m_children_render_order.end(),
        [](Item* left, Item* right) { return left->z() > right->z(); }
    );
}

void Item::set_self_align(YGAlign align)
{
    if (m_self_align != align) {
        m_self_align = align;
        YGNodeStyleSetAlignSelf(m_node, m_self_align);
        set_style_dirty();
    }
}

void Item::set_margin(const Margins& margin)
{
    if (m_margin != margin) {
        m_margin = margin;
        YGNodeStyleSetMargin(m_node, YGEdgeLeft, m_margin.left);
        YGNodeStyleSetMargin(m_node, YGEdgeRight, m_margin.right);
        YGNodeStyleSetMargin(m_node, YGEdgeTop, m_margin.top);
        YGNodeStyleSetMargin(m_node, YGEdgeBottom, m_margin.bottom);
        set_style_dirty();
    }
}

void Item::set_padding(const Paddings& padding)
{
    if (m_padding != padding) {
        m_padding = padding;
        YGNodeStyleSetPadding(m_node, YGEdgeLeft, m_padding.left);
        YGNodeStyleSetPadding(m_node, YGEdgeRight, m_padding.right);
        YGNodeStyleSetPadding(m_node, YGEdgeTop, m_padding.top);
        YGNodeStyleSetPadding(m_node, YGEdgeBottom, m_padding.bottom);
        set_style_dirty();
    }
}

void Item::set_min_size(const Vec2f min_size)
{
    if (m_min_size != min_size) {
        m_min_size = min_size;
        YGNodeStyleSetMinWidth(m_node, m_min_size.x());
        YGNodeStyleSetMinHeight(m_node, m_min_size.y());
        set_style_dirty();
    }
}

void Item::set_flex_grow(float flex_grow)
{
    if (!Domain::fuzzy_compare(m_flex_grow, flex_grow)) {
        m_flex_grow = flex_grow;
        YGNodeStyleSetFlexGrow(m_node, m_flex_grow);
        set_style_dirty();
    }
}

void Item::set_flex_shrink(float flex_shrink)
{
    if (!Domain::fuzzy_compare(m_flex_shrink, flex_shrink)) {
        m_flex_shrink = flex_shrink;
        YGNodeStyleSetFlexShrink(m_node, m_flex_shrink);
        set_style_dirty();
    }
}

void Item::set_direction(YGDirection direction)
{
    if (m_direction != direction) {
        m_direction = direction;
        YGNodeStyleSetDirection(m_node, m_direction);
        set_style_dirty();
    }
}

void Item::set_aspect_ratio(float aspect_ratio)
{
    if (!Domain::fuzzy_compare(m_aspect_ratio, aspect_ratio)) {
        m_aspect_ratio = aspect_ratio;
        YGNodeStyleSetAspectRatio(m_node, m_aspect_ratio);
        set_style_dirty();
    }
}

void Item::set_justify_content(YGJustify justify_content)
{
    if (m_justify_content != justify_content) {
        m_justify_content = justify_content;
        YGNodeStyleSetJustifyContent(m_node, m_justify_content);
        set_style_dirty();
    }
}

void Item::set_position_type(YGPositionType position_type)
{
    if (m_position_type != position_type) {
        m_position_type = position_type;
        YGNodeStyleSetPositionType(m_node, m_position_type);
        set_style_dirty();
    }
}

void Item::set_orientation(Orientation orientation)
{
    if (m_orientation != orientation) {
        m_orientation = orientation;
        m_flex_direction = orientation == Orientation::Horizontal
            ? YGFlexDirectionRow
            : YGFlexDirectionColumn;
        YGNodeStyleSetFlexDirection(m_node, m_flex_direction);
        set_style_dirty();
    }
}

void Item::set_gap(float gap)
{
    if (m_gap != gap) {
        m_gap = gap;
        YGNodeStyleSetGap(m_node, YGGutter::YGGutterAll, m_gap);
        set_style_dirty();
    }
}

void Item::set_align_items(YGAlign align_items)
{
    if (m_align_items != align_items) {
        m_align_items = align_items;
        YGNodeStyleSetAlignItems(m_node, m_align_items);
        set_style_dirty();
    }
}

void Item::set_align_content(YGAlign align_content)
{
    if (m_align_content != align_content) {
        m_align_content = align_content;
        YGNodeStyleSetAlignContent(m_node, m_align_content);
        set_style_dirty();
    }
}

void Item::set_z(float z)
{
    if (!Domain::fuzzy_compare(m_z, z)) {
        m_z = z;
        if (m_parent) {
            m_parent->update_children_render_order();
        }
        set_style_dirty();
    }
}

const std::string& Item::item_name() const { return m_item_name; }

void Item::set_item_name(const std::string& item_name)
{
    ASSERT(item_name.find('_'), "Yoga::Item name cannot contain underscore '_'");

    // Get item name without increment
    std::string old_name = m_item_name.substr(0, m_item_name.find('_'));
    if (item_name == old_name) {
        return;
    }

    // decrement old name map
    if (!old_name.empty()) {
        DEBUG_ASSERT(m_item_names.contains(old_name) && m_item_names.at(old_name) > 0);
        m_item_names[old_name]--;
    }

    if (m_item_names.contains(item_name)) {
        m_item_name = item_name + "_" + std::to_string(++m_item_names[item_name]);
    } else {
        m_item_name = item_name;
        m_item_names.insert({item_name, 1});
    }
}

void Item::set_style_dirty()
{
    if (m_parent) {
        m_parent->set_style_dirty();
    }
}

void Item::style_node()
{
    ASSERT(m_node);
    if (m_children.empty()) {
        // If we are a leaf node, calculate our min size
        if (!m_min_size_calculated) {
            m_min_size_calculated = true;
            Vec2f sz = get_item_size();
            m_min_size = m_min_size.cwiseMax(sz);
            YGNodeStyleSetMinWidth(m_node, m_min_size.x());
            YGNodeStyleSetMinHeight(m_node, m_min_size.y());
        }
    } else {
        // Otherwise style all our children
        for (const ItemPtr& item : std::as_const(m_children)) {
            item->style_node();
        }
    }
}

YGNodeRef Item::get_node(size_t index)
{
    ASSERT(m_node);
    return YGNodeGetChild(m_node, index);
}

size_t Item::get_node_count() const
{
    ASSERT(m_node);
    return YGNodeGetChildCount(m_node);
}

ImVec2 Item::get_node_pos() const
{
    ASSERT(m_node);
    return {YGNodeLayoutGetLeft(m_node), YGNodeLayoutGetTop(m_node)};
}

void Item::prepend(ItemPtr child) { insert(std::move(child), 0); }

void Item::append(ItemPtr child) { insert(std::move(child), get_node_count()); }

void Item::insert(ItemPtr child, size_t index) { add_child(std::move(child), index); }

ItemPtr Item::remove(Item* child) { return remove_child(child); }

void Item::render_item_begin(Vec2f pos, Vec2f size) {}

void Item::render_item_end(Vec2f pos, Vec2f size)
{
    render_debug(pos, size);

    for (Item* child : std::as_const(m_children_render_order)) {
        render_node(pos, child);
    }
}

void Item::render_node(Vec2f pos, Item* child)
{
    YGNodeRef child_node = child->node();
    if (!is_node_visible(child_node)) {
        return;
    }

    Vec2f cell_pos = pos + Vec2f(YGNodeLayoutGetLeft(child_node), YGNodeLayoutGetTop(child_node));
    Vec2f cell_size = Vec2f(YGNodeLayoutGetWidth(child_node), YGNodeLayoutGetHeight(child_node));

    child->render(cell_pos, cell_size);
}

void Item::render_debug(Vec2f pos, Vec2f size)
{
    if (m_debug_border) {
        ImDrawList* draw_list = ImGui::GetForegroundDrawList();
        ImRect rect(to_im(pos), to_im(pos + size));
        // SPDLOG_INFO(
        //     "debug {}:{} -> {}:{} pos {}:{} size {}:{}", rect.Min.x, rect.Min.y, rect.Max.x,
        //     rect.Max.y, pos.x(), pos.y(), size.x(), size.y()
        // );
        draw_list->AddRectFilled(rect.Min, rect.Max, IM_COL32(255, 0, 0, 128), false);
        // ImGui::RenderFrame(to_im(pos), to_im(pos + size), IM_COL32(20, 220, 200, 25), false);
    }
}

void Item::process_events_node(Vec2f pos, Item* child)
{
    YGNodeRef child_node = child->node();
    if (!is_node_visible(child_node) || !child->enabled()) {
        return;
    }

    Vec2f cell_pos = pos + Vec2f(YGNodeLayoutGetLeft(child_node), YGNodeLayoutGetTop(child_node));
    Vec2f cell_size = Vec2f(YGNodeLayoutGetWidth(child_node), YGNodeLayoutGetHeight(child_node));

    child->process_events(cell_pos, cell_size);
}

void Item::render_image(
    Render::TexturePtr texture,
    const ImVec2& image_size,
    const ImVec2& uv0,
    const ImVec2& uv1,
    const ImVec4& tint_col,
    const ImVec4& border_col
)
{
    ImGui::Image((ImTextureID) (intptr_t) texture.get(), image_size, uv0, uv1, tint_col, border_col);
    m_imgui_render->use_texture(texture);
}

Item* Item::get_item(size_t index) const
{
    return index >= m_children.size() ? nullptr : m_children.at(index).get();
}

size_t Item::item_count() const { return m_children.size(); }

void Item::check_resized()
{
    float w = width();
    float h = height();
    if (!Domain::fuzzy_compare(w, m_last_width) || !Domain::fuzzy_compare(h, m_last_height)) {
        m_last_width = w;
        m_last_height = h;
        on_resized();
    }

    for (const ItemPtr& child : std::as_const(m_children)) {
        child->check_resized();
    }
}

} // namespace Slic3r::App::Yoga
