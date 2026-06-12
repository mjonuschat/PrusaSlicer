///|/ Copyright (c) Prusa Research 2018 - 2025 Oleksandra Iushchenko @YuSanka, Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/Yoga/Item.hpp"

#include <Slic3r/Assert.hpp>
#include <Slic3r/Log.hpp>

#include "Slic3r/App/Render/ImguiRender.hpp"
#include "Slic3r/App/Yoga/ItemEvents.hpp"
#include "Slic3r/App/Yoga/ScrollArea.hpp"

#include <imgui_internal.h>
#include <fmt/format.h>
#include <cmath>
#include <string>
#include <stack>

namespace Slic3r::App::Yoga {

/**
 * @brief utility function to traverse tree from object ana run function on every visited object
 */
static void traverse(Object* object, std::function<void(Object* object)> function)
{
    std::stack<Object*> children;
    for (Object* child : object->objects()) {
        children.push(child);
    }
    while (!children.empty()) {
        Object* child = children.top();
        children.pop();

        function(child);

        for (Object* child_object : child->objects()) {
            children.push(child_object);
        }
    }
}

Render::ImguiRender* Item::m_imgui_render = nullptr;

YGConfigRef Object::m_config = YGConfigNew();

Theme* Object::m_theme = nullptr;

std::unordered_map<std::string, int> Object::m_object_names = {};

Object::Object() : m_heartbeat(std::make_shared<int>(1)) {}

Object::~Object() = default;

const std::string& Object::object_name() const
{
    return m_object_name;
}

void Object::set_object_name(const std::string& object_name)
{
    ASSERT(
        object_name.find('_') == std::string::npos,
        "Yoga::Object name cannot contain underscore '_'"
    );

    // Get item name without increment
    std::string old_name = m_object_name.substr(0, m_object_name.find('_'));
    if (object_name == old_name) {
        // No item change -> early exit
        return;
    }

    if (m_object_names.contains(object_name)) {
        m_object_name = object_name + "_" + std::to_string(++m_object_names[object_name]);
    } else {
        m_object_name = object_name + "_1";
        m_object_names.insert({object_name, 1});
    }
}

void Object::resize(const SizeInfo& size_info) {}

void Object::render(const Vec2f& pos, const Vec2f& size) {}

void Object::style_node()
{
    for (const ObjectPtr& child : std::as_const(m_children)) {
        child->style_node();
    }
}

Object* Object::parent() const
{
    return m_parent;
}

std::vector<Object*> Object::objects() const
{
    std::vector<Object*> objects;
    objects.reserve(m_children.size());
    std::transform(
        m_children.cbegin(),
        m_children.cend(),
        std::back_inserter(objects),
        [](const ObjectPtr& object) { return object.get(); }
    );

    return objects;
}

void Object::prepend(ObjectPtr child)
{
    insert(std::move(child), 0);
}

void Object::append(ObjectPtr child)
{
    insert(std::move(child), m_children.size());
}

void Object::insert(ObjectPtr child, size_t index)
{
    add_child(std::move(child), index);
}

ObjectPtr Object::remove(Object* child)
{
    return remove_child(child);
}

ObjectHeartBeat Object::heartbeat() const
{
    return m_heartbeat.get();
}

void Object::push_event(std::unique_ptr<Event> event)
{
    Object* item_to_push = nullptr;

    if (m_root && m_root != this) {
        item_to_push = m_root;
    }

    if (item_to_push) {
        // Event will be pushed either to parent or directly to RootItem for processing
        item_to_push->push_event(std::move(event));
    } else {
        // What just happened is that some Object (this or our children) pushed event
        // But we are not able to reach RootItem therefore there is nobody to process this event.
        // This usually happen in two cases:
        // 1) Event is pushed during RootItem destruction and the vtable cannot be derived
        // Solution: Do not push any event during RootItem destruction
        // 2) Event is pushed through isolated tree which is not parented to RootItem (therefore tree is not valid)
        // Solution: Either do not push events in these trees or insert that tree to RootItem tree.

        // We would like this to catch this in Debug always, majority of these issues
        // are present only during destruction and in that case it would be overkill to crash
        // in Release
        DEBUG_ASSERT(
            item_to_push,
            "ItemEvent {} cannot be pushed from {}, you are trying to push event either during destruction of the tree or to isolated island"
        );
        SPDLOG_WARN(
            "ItemEvent {} cannot be pushed from {}, please report",
            std::to_string(reinterpret_cast<unsigned long long>(event.get())),
            object_name()
        );
    }
}

void Object::add_child(ObjectPtr child, size_t index)
{
    ASSERT(child);
    ASSERT(!index_of(child.get()).has_value(), "Child is already parented to this item");
    ASSERT(index <= m_children.size(), "Invalid child index");

    child->m_parent = this;

    if (m_root != child->m_root) {
        child->m_root = m_root;
        traverse(
            child.get(),
            [this](Object* object)
            {
                object->root_item_about_to_update();
                object->m_root = m_root;
                object->root_item_updated();
            }
        );
    }

    m_children.insert(m_children.cbegin() + index, std::move(child));
}

ObjectPtr Object::remove_child(Object* child)
{
    ASSERT(child);

    std::vector<ObjectPtr>::iterator it = std::find_if(
        m_children.begin(),
        m_children.end(),
        [child](const ObjectPtr& child_item) { return child_item.get() == child; }
    );

    ASSERT(it != m_children.end(), "Trying to remove unmaintained child");
    if (it == m_children.end()) {
        return nullptr;
    }

    child->m_parent = nullptr;
    child->m_root   = child;

    traverse(
        child,
        [child](Object* object)
        {
            if (object->m_root) {
                object->root_item_about_to_update();
                object->m_root = child;
                object->root_item_updated();
            }
        }
    );

    ObjectPtr result(std::move(*it));
    m_children.erase(it);

    return result;
}

Object* Object::root_item() const
{
    return m_root;
}

void Object::root_item_about_to_update() {}

void Object::root_item_updated() {}

size_t Object::object_count() const
{
    return m_children.size();
}

Object* Object::get_object(size_t index) const
{
    return m_children.at(index).get();
}

void Object::set_style_dirty()
{
    if (m_root != this) {
        m_root->set_style_dirty();
    }
}

std::optional<size_t> Object::index_of(Object* item) const
{
    std::vector<ObjectPtr>::const_iterator it = std::find_if(
        m_children.cbegin(),
        m_children.cend(),
        [item](const ObjectPtr& object) { return item == object.get(); }
    );
    return it == m_children.cend() ? std::nullopt :
                                     std::optional<size_t>(std::distance(m_children.cbegin(), it));
}

Item::Item()
{
    m_node = YGNodeNew();

    YGNodeStyleSetFlexDirection(m_node, m_flex_direction);
    YGNodeStyleSetDirection(m_node, m_direction);
    YGNodeStyleSetFlexShrink(m_node, m_flex_shrink.result);
}

Item::~Item()
{
    if (m_node) {
        YGNodeFree(m_node);
    }
}

Item::Callbacks& Item::item_callbacks()
{
    return m_callbacks;
}

void Item::resize(const SizeInfo& size_info)
{
    if (size_info != m_size_info) {
        m_size_info           = size_info;
        m_min_size_calculated = false;

        if (m_width.has_value() && std::holds_alternative<EvaluatedUnit>(*m_width)) {
            auto& eu = std::get<EvaluatedUnit>(*m_width);
            eu.evaluate(size_info);
            YGNodeStyleSetWidth(m_node, eu.result);
        }
        if (m_height.has_value() && std::holds_alternative<EvaluatedUnit>(*m_height)) {
            auto& eu = std::get<EvaluatedUnit>(*m_height);
            eu.evaluate(size_info);
            YGNodeStyleSetHeight(m_node, eu.result);
        }
        if (m_left.has_value()) {
            m_left->evaluate(size_info);
            YGNodeStyleSetPosition(m_node, YGEdgeLeft, m_left->result);
        }
        if (m_right.has_value()) {
            m_right->evaluate(size_info);
            YGNodeStyleSetPosition(m_node, YGEdgeRight, m_right->result);
        }
        if (m_top.has_value()) {
            m_top->evaluate(size_info);
            YGNodeStyleSetPosition(m_node, YGEdgeTop, m_top->result);
        }
        if (m_bottom.has_value()) {
            m_bottom->evaluate(size_info);
            YGNodeStyleSetPosition(m_node, YGEdgeBottom, m_bottom->result);
        }

        m_min_width.evaluate(size_info);
        YGNodeStyleSetMinWidth(m_node, m_min_width.result);
        m_max_width.evaluate(size_info);
        YGNodeStyleSetMaxWidth(m_node, m_max_width.result);
        m_min_height.evaluate(size_info);
        YGNodeStyleSetMinHeight(m_node, m_min_height.result);
        m_max_height.evaluate(size_info);
        YGNodeStyleSetMaxHeight(m_node, m_max_height.result);

        m_flex_grow.evaluate(size_info);
        YGNodeStyleSetFlexGrow(m_node, m_flex_grow.result);
        m_flex_shrink.evaluate(size_info);
        YGNodeStyleSetFlexShrink(m_node, m_flex_shrink.result);

        m_gap.evaluate(size_info);
        YGNodeStyleSetGap(m_node, YGGutterAll, m_gap.result);

        m_margins.evaluate(size_info);
        YGNodeStyleSetMargin(m_node, YGEdgeLeft, m_margins.left);
        YGNodeStyleSetMargin(m_node, YGEdgeRight, m_margins.right);
        YGNodeStyleSetMargin(m_node, YGEdgeTop, m_margins.top);
        YGNodeStyleSetMargin(m_node, YGEdgeBottom, m_margins.bottom);

        m_paddings.evaluate(size_info);
        YGNodeStyleSetPadding(m_node, YGEdgeLeft, m_paddings.left);
        YGNodeStyleSetPadding(m_node, YGEdgeRight, m_paddings.right);
        YGNodeStyleSetPadding(m_node, YGEdgeTop, m_paddings.top);
        YGNodeStyleSetPadding(m_node, YGEdgeBottom, m_paddings.bottom);

        size_info_changed(size_info);

        set_style_dirty();
    }

    std::ranges::for_each(m_children, [&](ObjectPtr& child) { child->resize(size_info); });
}

void Item::render(const Vec2f& pos, const Vec2f& size)
{
    render_item_begin(pos, size);

    render_item_end(pos, size);
}

YGNodeRef Item::node() const
{
    return m_node;
}

float Item::z() const
{
    return m_z;
}

float Item::width() const
{
    return YGNodeLayoutGetWidth(m_node);
}

float Item::height() const
{
    return YGNodeLayoutGetHeight(m_node);
}

float Item::left() const
{
    return YGNodeLayoutGetLeft(m_node);
}

float Item::right() const
{
    return YGNodeLayoutGetRight(m_node);
}

float Item::top() const
{
    return YGNodeLayoutGetTop(m_node);
}

float Item::bottom() const
{
    return YGNodeLayoutGetBottom(m_node);
}

const Unit& Item::min_width() const
{
    return m_min_width.source;
}

const Unit& Item::min_heigth() const
{
    return m_min_height.source;
}

const Unit& Item::max_width() const
{
    return m_max_width.source;
}

const Unit& Item::max_height() const
{
    return m_max_height.source;
}

bool Item::is_visible() const
{
    return is_node_visible(m_node);
}

bool Item::is_self_visible() const
{
    return m_visible;
}

bool Item::debug_border() const
{
    return m_debug_border;
}

const Unit& Item::flex_grow() const
{
    return m_flex_grow.source;
}

const Unit& Item::flex_shrink() const
{
    return m_flex_shrink.source;
}

YGDirection Item::direction() const
{
    return m_direction;
}

float Item::aspect_ratio() const
{
    return m_aspect_ratio;
}

YGJustify Item::justify_content() const
{
    return m_justify_content;
}

YGPositionType Item::position_type() const
{
    return m_position_type;
}

YGAlign Item::align_items() const
{
    return m_align_items;
}

YGAlign Item::align_content() const
{
    return m_align_content;
}

const EvaluatedMargins& Item::margin() const
{
    return m_margins;
}

const EvaluatedPaddings& Item::padding() const
{
    return m_paddings;
}

const Unit& Item::gap() const
{
    return m_gap.source;
}

Orientation Item::orientation() const
{
    return m_orientation;
}

YGWrap Item::flex_wrap() const
{
    return YGNodeStyleGetFlexWrap(m_node);
}

bool Item::is_node_dirty() const
{
    return YGNodeIsDirty(m_node);
}

bool Item::is_in_window() const
{
    return m_parent_item ? m_parent_item->is_in_window() : false;
}

bool Item::enabled() const
{
    const Item* item = this;
    while (item) {
        if (!item->m_enabled) {
            return false;
        }

        item = item->m_parent_item;
    }

    return true;
}

void Item::set_enabled(bool enabled)
{
    if (m_enabled != enabled) {
        m_enabled = enabled;
        update_enabled();
    }
}

void Item::set_visible(bool visible)
{
    if (m_visible != visible) {
        m_visible = visible;
        YGNodeStyleSetDisplay(m_node, visible ? YGDisplayFlex : YGDisplayNone);
        set_style_dirty();
        update_visible();
    }
}

void Item::set_width(const Unit& width)
{
    if (!m_width.has_value()
        || !std::holds_alternative<EvaluatedUnit>(m_width.value())
        || std::get<EvaluatedUnit>(m_width.value()).source != width)
    {
        EvaluatedUnit new_width{width};
        new_width.evaluate(m_size_info);
        YGNodeStyleSetWidth(m_node, new_width.result);
        m_width = std::move(new_width);
        set_style_dirty();
    }
}

void Item::set_height(const Unit& height)
{
    if (!m_height.has_value()
        || !std::holds_alternative<EvaluatedUnit>(m_height.value())
        || std::get<EvaluatedUnit>(m_height.value()).source != height)
    {
        EvaluatedUnit new_height{height};
        new_height.evaluate(m_size_info);
        YGNodeStyleSetHeight(m_node, new_height.result);
        m_height = std::move(new_height);
        set_style_dirty();
    }
}

void Item::set_width_percent(float width_percent)
{
    if (!m_width.has_value()
        || !std::holds_alternative<float>(m_width.value())
        || !Domain::fuzzy_compare(std::get<float>(m_width.value()), width_percent))
    {
        m_width = width_percent;
        YGNodeStyleSetWidthPercent(m_node, width_percent);
        set_style_dirty();
    }
}

void Item::set_height_percent(float height_percent)
{
    if (!m_height.has_value()
        || !std::holds_alternative<float>(m_height.value())
        || !Domain::fuzzy_compare(std::get<float>(m_height.value()), height_percent))
    {
        m_height = height_percent;
        YGNodeStyleSetHeightPercent(m_node, height_percent);
        set_style_dirty();
    }
}

void Item::set_left(const Unit& left)
{
    if (!m_left.has_value() || m_left.value().source != left) {
        EvaluatedUnit new_left{left};
        new_left.evaluate(m_size_info);
        YGNodeStyleSetPosition(m_node, YGEdgeLeft, new_left.result);
        m_left = new_left;
        set_style_dirty();
    }
}

void Item::set_right(const Unit& right)
{
    if (!m_right.has_value() || m_right.value().source != right) {
        EvaluatedUnit new_right{right};
        new_right.evaluate(m_size_info);
        YGNodeStyleSetPosition(m_node, YGEdgeRight, new_right.result);
        m_right = new_right;
        set_style_dirty();
    }
}

void Item::set_top(const Unit& top)
{
    if (!m_top.has_value() || m_top.value().source != top) {
        EvaluatedUnit new_top{top};
        new_top.evaluate(m_size_info);
        YGNodeStyleSetPosition(m_node, YGEdgeTop, new_top.result);
        m_top = new_top;
        set_style_dirty();
    }
}

void Item::set_bottom(const Unit& bottom)
{
    if (!m_bottom.has_value() || m_bottom.value().source != bottom) {
        EvaluatedUnit new_bottom{bottom};
        new_bottom.evaluate(m_size_info);
        YGNodeStyleSetPosition(m_node, YGEdgeBottom, new_bottom.result);
        m_bottom = new_bottom;
        set_style_dirty();
    }
}

void Item::set_flex_wrap(YGWrap wrap)
{
    YGNodeStyleSetFlexWrap(m_node, wrap);
    set_style_dirty();
}

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

const std::vector<Item*>& Item::items() const
{
    return m_children_items;
}

Item* Item::get_item(size_t index) const
{
    return m_children_items.at(index);
}

void Item::set_debug_border(bool show_debug_border)
{
    m_debug_border = show_debug_border;
}

void Item::invalidate_min_size_calculation()
{
    m_min_size_calculated = false;
    set_style_dirty();
}

void Item::update_enabled()
{
    enabled_updated_internal();
    for (Item* child : m_children_items) {
        if (child) {
            child->update_enabled();
        }
    }
}

void Item::update_visible()
{
    visible_updated_internal();
    for (Item* child : m_children_items) {
        if (child) {
            child->update_visible();
        }
    }
}

void Item::size_info_changed(const SizeInfo& info_size) {}

ImVec2 Item::to_im(const Vec2f& val)
{
    return ImVec2(val.x(), val.y());
}

Vec2f Item::from_im(const ImVec2& val)
{
    return Vec2f(val.x, val.y);
}

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
    return {};
}

void Item::enabled_updated_internal() {}

void Item::visible_updated_internal() {}

void Item::on_resized()
{
    if (m_callbacks.size_changed) {
        m_callbacks.size_changed();
    }
}

Vec2f Item::get_global_pos() const
{
    Vec2f pos{left(), top()};

    if (m_parent_item) {
        ScrollArea* scroll_area = dynamic_cast<ScrollArea*>(m_parent_item);
        if (scroll_area) {
            pos -= scroll_area->scroll_pos();
        }
        pos += m_parent_item->get_global_pos();
    }

    return pos;
};

void Item::set_imgui_render(Render::ImguiRender* imgui_render)
{
    m_imgui_render = imgui_render;
}

void Object::set_theme(Theme* theme)
{
    m_theme = theme;
}

void Object::set_scale_factor(float scale_factor)
{
    YGConfigSetPointScaleFactor(m_config, scale_factor);
}

float Object::scale_factor()
{
    return YGConfigGetPointScaleFactor(m_config);
}

float Object::pixel_round(float value)
{
    const auto scale = YGConfigGetPointScaleFactor(m_config);
    return YGRoundValueToPixelGrid(value, scale, false, false);
}

ImVec2 Object::pixel_round(const ImVec2& value)
{
    return {pixel_round(value.x), pixel_round(value.y)};
}

Vec2f Object::pixel_round(const Vec2f& value)
{
    return {pixel_round(value.x()), pixel_round(value.y())};
}

void Item::update_children_render_order()
{
    m_children_render_order.clear();
    std::copy_if(
        m_children_items.cbegin(),
        m_children_items.cend(),
        std::back_inserter(m_children_render_order),
        [](Item* child) { return child; }
    );
    // sort children by Z layer, higher Z shall be rendered first
    std::sort(
        m_children_render_order.begin(),
        m_children_render_order.end(),
        [](Item* left, Item* right) { return left->z() > right->z(); }
    );

    set_style_dirty();
}

Item* Item::parent_item() const
{
    return m_parent_item;
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
    if (m_margins.source != margin) {
        m_margins.source = margin;
        m_margins.evaluate(m_size_info);
        YGNodeStyleSetMargin(m_node, YGEdgeLeft, m_margins.left);
        YGNodeStyleSetMargin(m_node, YGEdgeRight, m_margins.right);
        YGNodeStyleSetMargin(m_node, YGEdgeTop, m_margins.top);
        YGNodeStyleSetMargin(m_node, YGEdgeBottom, m_margins.bottom);
        set_style_dirty();
    }
}

void Item::set_padding(const Paddings& padding)
{
    if (m_paddings.source != padding) {
        m_paddings.source = padding;
        m_paddings.evaluate(m_size_info);
        YGNodeStyleSetPadding(m_node, YGEdgeLeft, m_paddings.left);
        YGNodeStyleSetPadding(m_node, YGEdgeRight, m_paddings.right);
        YGNodeStyleSetPadding(m_node, YGEdgeTop, m_paddings.top);
        YGNodeStyleSetPadding(m_node, YGEdgeBottom, m_paddings.bottom);
        set_style_dirty();
    }
}

void Item::set_min_width(const Unit& min_width)
{
    if (min_width != m_min_width.source) {
        m_min_width.source = min_width;
        m_min_width.evaluate(m_size_info);
        YGNodeStyleSetMinWidth(m_node, m_min_width.result);
        set_style_dirty();
    }
}

void Item::set_max_width(const Unit& max_width)
{
    if (max_width != m_max_width.source) {
        m_max_width.source = max_width;
        m_max_width.evaluate(m_size_info);
        YGNodeStyleSetMaxWidth(m_node, m_max_width.result);
        set_style_dirty();
    }
}

void Item::set_min_height(const Unit& min_height)
{
    if (min_height != m_min_height.source) {
        m_min_height.source = min_height;
        m_min_height.evaluate(m_size_info);
        YGNodeStyleSetMinHeight(m_node, m_min_height.result);
        set_style_dirty();
    }
}

void Item::set_max_height(const Unit& max_height)
{
    if (max_height != m_max_height.source) {
        m_max_height.source = max_height;
        m_max_height.evaluate(m_size_info);
        YGNodeStyleSetMaxHeight(m_node, m_max_height.result);
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
        switch (orientation) {
        case Orientation::Horizontal:
            m_flex_direction = YGFlexDirectionRow;
            break;
        case Orientation::Vertical:
            m_flex_direction = YGFlexDirectionColumn;
            break;
        case Orientation::VerticalReverse:
            m_flex_direction = YGFlexDirectionColumnReverse;
            break;
        }
        YGNodeStyleSetFlexDirection(m_node, m_flex_direction);
        set_style_dirty();
    }
}

void Item::set_flex_grow(const Unit& flex_grow)
{
    if (flex_grow != m_flex_grow.source) {
        m_flex_grow.source = flex_grow;
        m_flex_grow.evaluate(m_size_info);
        YGNodeStyleSetFlexGrow(m_node, m_flex_grow.result);
        set_style_dirty();
    }
}

void Item::set_flex_shrink(const Unit& flex_shrink)
{
    if (flex_shrink != m_flex_shrink.source) {
        m_flex_shrink.source = flex_shrink;
        m_flex_shrink.evaluate(m_size_info);
        YGNodeStyleSetFlexShrink(m_node, m_flex_shrink.result);
        set_style_dirty();
    }
}

void Item::set_gap(const Unit& gap)
{
    if (m_gap.source != gap) {
        m_gap.source = gap;
        m_gap.evaluate(m_size_info);
        YGNodeStyleSetGap(m_node, YGGutter::YGGutterAll, m_gap.result);
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
        if (m_parent_item) {
            m_parent_item->update_children_render_order();
        }
    }
}

void Item::insert(ObjectPtr child, size_t index)
{
    Item* item = dynamic_cast<Item*>(child.get());

    Object::insert(std::move(child), index);

    if (item) {
        item->m_parent_item = this;
        YGNodeInsertChild(m_node, item->node(), std::min(index, YGNodeGetChildCount(m_node)));
        m_children_items.insert(m_children_items.cbegin() + index, item);
        update_children_render_order();
        m_has_items = true;
    } else {
        m_children_items.insert(m_children_items.cbegin() + index, nullptr);
    }
}

ObjectPtr Item::remove(Object* child)
{
    std::optional<size_t> index = index_of(child);
    ASSERT(index.has_value());

    Item* item = m_children_items.at(index.value());
    m_children_items.erase(m_children_items.cbegin() + index.value());
    if (item) {
        item->m_parent_item = nullptr;
        YGNodeRemoveChild(m_node, item->node());
        update_children_render_order();

        m_has_items = std::any_of(
            m_children_items.cbegin(),
            m_children_items.cend(),
            [](Item* item) { return item; }
        );
    }

    return Object::remove(child);
}

void Item::style_node()
{
    ASSERT(m_node);
    if (!m_visible) {
        return;
    }

    if (!m_has_items) {
        // If we are a leaf node, calculate our min size
        if (!m_min_size_calculated) {
            m_min_size_calculated = true;
            Vec2f sz              = get_item_size();
            sz                    = sz.cwiseMax(Vec2f{m_min_width.result, m_min_height.result});
            YGNodeStyleSetMinWidth(m_node, sz.x());
            YGNodeStyleSetMinHeight(m_node, sz.y());
        }
    } else {
        // Otherwise style all our children
        for (const ObjectPtr& object : std::as_const(m_children)) {
            object->style_node();
        }
    }
}

void Item::render_item_begin(Vec2f pos, Vec2f size) {}

void Item::render_item_end(Vec2f pos, Vec2f size)
{
    render_debug(pos, size);

    for (size_t render_index = 0; render_index < m_children_render_order.size(); ++render_index) {
        render_node(pos, m_children_render_order.at(render_index));
    }
}

void Item::render_node(Vec2f pos, Item* child)
{
    YGNodeRef child_node = child->node();
    if (!is_node_visible(child_node)) {
        return;
    }

    Vec2f cell_pos  = pos + Vec2f(child->left(), child->top());
    Vec2f cell_size = Vec2f(child->width(), child->height());

    if (isnan(cell_size.x()) || isnan(cell_size.y())) {
        return;
    }

    child->render(cell_pos, cell_size);
}

#ifdef DEBUG
Item* Item::m_debug_item = nullptr;

static std::string_view unit_type_string(Unit::Type type)
{
    switch (type) {
    case Unit::Type::Pixel:
        return "px";
    case Unit::Type::FigmaPixel:
        return "fpx";
    case Unit::Type::Point:
        return "pt";
    case Unit::Type::Rem:
        return "rem";
    case Unit::Type::ViewportWidth:
        return "vw";
    case Unit::Type::ViewportHeight:
        return "vh";
    case Unit::Type::ViewportMin:
        return "vmin";
    case Unit::Type::ViewportMax:
        return "vmax";
    }
    return "?";
}

static std::string fmt_side(const Unit& src, float result)
{
    if (std::isnan(src.value) || std::isnan(result)) {
        return "(unset)";
    } else if (src.type == Unit::Type::Pixel) {
        return fmt::format("{:.4g}px", src.value);
    } else {
        return fmt::format("{:.4g}{}={:.4g}", src.value, unit_type_string(src.type), result);
    }
}

static std::string fmt_eu(const EvaluatedUnit& eu)
{
    return fmt_side(eu.source, eu.result);
}

static void draw_debug_cross(ImVec2 pos)
{
    static float a = 0.0f;
    a += 0.13f; // ~7.5°

    const ImVec2 c{pos.x + 8.0f, pos.y + 8.0f};
    const auto l = [&](float x) {
        const ImVec2 d{std::cos(x) * 8.0f, std::sin(x) * 8.0f};
        ImGui::GetForegroundDrawList()->AddLine(
            {c.x - d.x, c.y - d.y},
            {c.x + d.x, c.y + d.y},
            IM_COL32(255, 0, 0, 255),
            2.0f
            );
    };

    l(a);
    l(a + 1.5708f);
}
#endif

void Item::render_debug_overlay(ImDrawList* draw_list) const
{
#ifdef DEBUG
    ImGui::PushFont(ImGui::GetFont(), 16);

    constexpr ImU32 c_header  = IM_COL32(255, 180, 60, 255);
    constexpr ImU32 c_text    = IM_COL32(220, 220, 220, 255);
    constexpr ImU32 c_margin  = IM_COL32(246, 178, 107, 230);
    constexpr ImU32 c_padding = IM_COL32(147, 196, 125, 230);
    constexpr ImU32 c_content = IM_COL32(180, 220, 255, 255);
    constexpr ImU32 c_border  = IM_COL32(255, 60, 60, 220);
    constexpr ImU32 f_margin  = IM_COL32(246, 178, 107, 90);
    constexpr ImU32 f_padding = IM_COL32(147, 196, 125, 90);
    constexpr ImU32 f_content = IM_COL32(97, 150, 218, 70);

    const Vec2f gpos = get_global_pos();
    const float w    = width();
    const float h    = height();

    draw_list->AddRect(to_im(gpos), to_im(gpos + Vec2f(w, h)), c_border, 0.f, 0, 1.5f);

    const float ml = m_margins.left;
    const float mt = m_margins.top;
    const float mr = m_margins.right;
    const float mb = m_margins.bottom;

    const float pl = m_paddings.left;
    const float pt = m_paddings.top;
    const float pr = m_paddings.right;
    const float pb = m_paddings.bottom;

    const Sides& ms = m_margins.source;
    const Sides& ps = m_paddings.source;

    using Line = std::pair<std::string, ImU32>;

    std::vector<Line> lines;
    lines.reserve(7);

    {
        std::string header = object_name().empty() ? "" : object_name() + "  ";
        header += fmt::format("{}x{}px  at [{}, {}]", w, h, gpos.x(), gpos.y());
        lines.emplace_back(std::move(header), c_header);
    }

    lines.emplace_back(
        fmt::format(
            "dpi:{}  scale:{:.4g}x  vp:{}x{}  rem:{:.4g}px",
            m_size_info.dpi,
            m_size_info.dpi_scale_factor,
            m_size_info.viewport_size_x,
            m_size_info.viewport_size_y,
            m_size_info.root_font_size
        ),
        c_text
    );

    if (m_min_width.result > 0 || m_min_height.result > 0) {
        lines.emplace_back(
            "min_w: " + fmt_eu(m_min_width) + " min_h: " + fmt_eu(m_min_height),
            c_text
        );
    }
    if (!YGFloatIsUndefined(m_max_width.result) || !YGFloatIsUndefined(m_max_height.result)) {
        lines.emplace_back(
            "max_w: " + fmt_eu(m_max_width) + " max_h: " + fmt_eu(m_max_height),
            c_text
        );
    }

    auto fmt_yoga_size = [](const Item::YogaSize& yoga_size) -> std::string
    {
        if (std::holds_alternative<EvaluatedUnit>(yoga_size)) {
            return fmt_eu(std::get<EvaluatedUnit>(yoga_size));
        } else {
            return fmt::format("{:.4g}%", std::get<float>(yoga_size));
        }
    };

    if (m_width.has_value()) {
        lines.emplace_back("explicit width: " + fmt_yoga_size(m_width.value()), c_text);
    }
    if (m_height.has_value()) {
        lines.emplace_back("explicit height: " + fmt_yoga_size(m_height.value()), c_text);
    }

    lines.emplace_back(
        fmt::format(
            "grow:{:.4g}  shrink:{:.4g}  gap:{}",
            m_flex_grow.result,
            m_flex_shrink.result,
            fmt_eu(m_gap)
        ),
        c_text
    );

    const ImVec2 pad = {6.f, 4.f};
    const float lh   = ImGui::GetFontSize() + 2.f;

    auto for_each_text_line = [](const std::string& text, auto&& fn)
    {
        for (size_t start = 0;;) {
            const size_t end  = text.find('\n', start);
            const char* begin = text.c_str() + start;
            const char* stop =
                end == std::string::npos ? text.c_str() + text.size() : text.c_str() + end;

            fn(begin, stop);

            if (end == std::string::npos)
                break;

            start = end + 1;
        }
    };

    int total_text_lines = 0;
    float text_w         = 0.f;

    for (const auto& [text, col] : lines) {
        for_each_text_line(
            text,
            [&](const char* begin, const char* end)
            {
                text_w = std::max(text_w, ImGui::CalcTextSize(begin, end).x);
                ++total_text_lines;
            }
        );
    }

    const float ratio = h > 0.f ? w / h : 1.f;

    float dw = 220.f;
    float dh = 220.f / ratio;

    if (dh > 130.f) {
        dh = 130.f;
        dw = 130.f * ratio;
    }

    dw = std::max(dw, 80.f);
    dh = std::max(dh, 55.f);

    const float sx = w > 0.f ? dw / w : 1.f;
    const float sy = h > 0.f ? dh / h : 1.f;

    auto scaled_x = [sx](float value) { return value > 0.f ? std::max(3.f, value * sx) : 0.f; };

    auto scaled_y = [sy](float value) { return value > 0.f ? std::max(3.f, value * sy) : 0.f; };

    const float dml = scaled_x(ml);
    const float dmt = scaled_y(mt);
    const float dmr = scaled_x(mr);
    const float dmb = scaled_y(mb);

    const float dpl = scaled_x(pl);
    const float dpt = scaled_y(pt);
    const float dpr = scaled_x(pr);
    const float dpb = scaled_y(pb);

    auto label_width = [](const Unit& src, float result)
    {
        if (!(result > 0.f))
            return 0.f;

        const std::string text = fmt_side(src, result);
        return ImGui::CalcTextSize(text.c_str()).x;
    };

    const float max_label_w = std::max(
        {label_width(ms.left, ml),
         label_width(ms.top, mt),
         label_width(ms.right, mr),
         label_width(ms.bottom, mb),
         label_width(ps.left, pl),
         label_width(ps.top, pt),
         label_width(ps.right, pr),
         label_width(ps.bottom, pb)}
    );

    const float fh        = ImGui::GetFontSize();
    const float label_gap = 2.f;

    const float diagram_extra_x      = max_label_w + label_gap;
    const float diagram_extra_top    = mt > 0.f ? fh + label_gap : 0.f;
    const float diagram_extra_bottom = mb > 0.f ? fh + label_gap : 0.f;

    const float diagram_w = dw + diagram_extra_x * 2.f;
    const float diagram_h = dh + diagram_extra_top + diagram_extra_bottom;

    const float pw = std::max(text_w, diagram_w) + pad.x * 2.f;
    const float ph = static_cast<float>(total_text_lines) * lh + diagram_h + pad.y * 2.f;

    const ImGuiViewport* vp = ImGui::GetMainViewport();
    const ImVec2 pmin       = {vp->Pos.x, vp->Pos.y + vp->Size.y - ph};
    const ImVec2 pmax       = {vp->Pos.x + pw, vp->Pos.y + vp->Size.y};

    draw_list->AddRectFilled(pmin, pmax, IM_COL32(0, 0, 0, 160));
    draw_list->AddRect(pmin, pmax, IM_COL32(255, 100, 0, 180));

    float y = pmin.y + pad.y;

    for (const auto& [text, col] : lines) {
        for_each_text_line(
            text,
            [&](const char* begin, const char* end)
            {
                draw_list->AddText({pmin.x + pad.x, y}, col, begin, end);
                y += lh;
            }
        );
    }

    const float ox = pmin.x + (pw - dw) * .5f;

    const ImVec2 mar_min = {ox, y + diagram_extra_top};
    const ImVec2 mar_max = {ox + dw, mar_min.y + dh};

    const ImVec2 bdr_min = {mar_min.x + dml, mar_min.y + dmt};
    const ImVec2 bdr_max = {mar_max.x - dmr, mar_max.y - dmb};

    const ImVec2 cnt_min = {bdr_min.x + dpl, bdr_min.y + dpt};
    const ImVec2 cnt_max = {bdr_max.x - dpr, bdr_max.y - dpb};

    if (dml > 0.f || dmt > 0.f || dmr > 0.f || dmb > 0.f) {
        draw_list->AddRectFilled({mar_min.x, mar_min.y}, {mar_max.x, bdr_min.y}, f_margin);
        draw_list->AddRectFilled({mar_min.x, bdr_max.y}, {mar_max.x, mar_max.y}, f_margin);
        draw_list->AddRectFilled({mar_min.x, bdr_min.y}, {bdr_min.x, bdr_max.y}, f_margin);
        draw_list->AddRectFilled({bdr_max.x, bdr_min.y}, {mar_max.x, bdr_max.y}, f_margin);
        draw_list->AddRect(mar_min, mar_max, c_margin);
    }

    if (dpl > 0.f || dpt > 0.f || dpr > 0.f || dpb > 0.f) {
        draw_list->AddRectFilled({bdr_min.x, bdr_min.y}, {bdr_max.x, cnt_min.y}, f_padding);
        draw_list->AddRectFilled({bdr_min.x, cnt_max.y}, {bdr_max.x, bdr_max.y}, f_padding);
        draw_list->AddRectFilled({bdr_min.x, cnt_min.y}, {cnt_min.x, cnt_max.y}, f_padding);
        draw_list->AddRectFilled({cnt_max.x, cnt_min.y}, {bdr_max.x, cnt_max.y}, f_padding);
    }

    if (cnt_min.x < cnt_max.x && cnt_min.y < cnt_max.y)
        draw_list->AddRectFilled(cnt_min, cnt_max, f_content);

    draw_list->AddRect(bdr_min, bdr_max, c_border, 0.f, 0, 1.5f);

    const float bdr_cx = (bdr_min.x + bdr_max.x) * .5f;
    const float bdr_cy = (bdr_min.y + bdr_max.y) * .5f;

    auto add_label = [&](const Unit& src, float result, ImU32 col, ImVec2 pos, ImVec2 align)
    {
        if (!(result > 0.f))
            return;

        const std::string text = fmt_side(src, result);
        const ImVec2 size      = ImGui::CalcTextSize(text.c_str());

        draw_list->AddText({pos.x - size.x * align.x, pos.y - size.y * align.y}, col, text.c_str());
    };

    ImGui::PushFont(ImGui::GetFont(), 12);
    // Margin: outside border box, adjacent to border edge.
    add_label(ms.top, mt, c_margin, {bdr_cx, bdr_min.y - fh - label_gap}, {.5f, 0.f});
    add_label(ms.bottom, mb, c_margin, {bdr_cx, bdr_max.y + label_gap}, {.5f, 0.f});
    add_label(ms.left, ml, c_margin, {bdr_min.x - label_gap, bdr_cy}, {1.f, .5f});
    add_label(ms.right, mr, c_margin, {bdr_max.x + label_gap, bdr_cy}, {0.f, .5f});

    // Padding: inside border box, adjacent to border edge.
    add_label(ps.top, pt, c_padding, {bdr_cx, bdr_min.y + label_gap}, {.5f, 0.f});
    add_label(ps.bottom, pb, c_padding, {bdr_cx, bdr_max.y - fh - label_gap}, {.5f, 0.f});
    add_label(ps.left, pl, c_padding, {bdr_min.x + label_gap, bdr_cy}, {0.f, .5f});
    add_label(ps.right, pr, c_padding, {bdr_max.x - label_gap, bdr_cy}, {1.f, .5f});
    ImGui::PopFont();

    if (cnt_min.x < cnt_max.x && cnt_min.y < cnt_max.y) {
        const std::string content_size =
            fmt::format("{:.4g}x{:.4g}", std::max(0.f, w - pl - pr), std::max(0.f, h - pt - pb));
        const ImVec2 size = ImGui::CalcTextSize(content_size.c_str());

        if (cnt_max.x - cnt_min.x >= size.x + 4.f && cnt_max.y - cnt_min.y >= size.y) {
            draw_list->AddText(
                {(cnt_min.x + cnt_max.x - size.x) * .5f, (cnt_min.y + cnt_max.y - size.y) * .5f},
                c_content,
                content_size.c_str()
            );
        }
    }

    ImGui::PopFont();

    draw_debug_cross({0, pmin.y - 16});
#endif
}

void Item::render_debug(Vec2f pos, Vec2f size)
{
#ifdef DEBUG
    {
        ImRect rect(to_im(pos), to_im(pos + size));

        if (ImGui::IsMouseHoveringRect(rect.Min, rect.Max, false)) {
            if (ImGui::IsKeyDown(ImGuiMod_Alt) && ImGui::IsKeyDown(ImGuiMod_Ctrl)) {
                ImDrawList* draw_list = ImGui::GetForegroundDrawList();
                draw_list->AddRect(rect.Min, rect.Max, IM_COL32(255, 0, 0, 128));
            } else if (ImGui::IsKeyDown(ImGuiMod_Alt)) {
                m_debug_item = this;
            }
        }
    }
#endif

    if (m_debug_border) {
        ImDrawList* draw_list = ImGui::GetForegroundDrawList();
        ImRect rect(to_im(pos), to_im(pos + size));
        // SPDLOG_INFO(
        // "debug {}:{} -> {}:{} pos {}:{} size {}:{}", rect.Min.x, rect.Min.y, rect.Max.x,
        // rect.Max.y, pos.x(), pos.y(), size.x(), size.y()
        // );
        draw_list->AddRectFilled(
            rect.Min,
            rect.Max,
            enabled() ? IM_COL32(255, 0, 0, 128) : IM_COL32(0, 0, 255, 128),
            false
        );
    }
}

void Item::render_image(
    Render::TexturePtr texture,
    const ImVec2& image_size,
    const ImVec2& uv0,
    const ImVec2& uv1,
    const ImVec4& background_col,
    const ImVec4& tint_col,
    float rounding
)
{
    ImGui::PushStyleVar(ImGuiStyleVar_ImageRounding, rounding);
    ImGui::ImageWithBg(
        (ImTextureID) (intptr_t) texture.get(),
        image_size,
        uv0,
        uv1,
        background_col,
        tint_col
    );
    m_imgui_render->use_texture(texture);
    ImGui::PopStyleVar();
}

void Item::check_resized()
{
    float w = width();
    float h = height();
    if (!Domain::fuzzy_compare(w, m_last_width) || !Domain::fuzzy_compare(h, m_last_height)) {
        m_last_width  = w;
        m_last_height = h;
        on_resized();
    }

    const std::vector<Item*> children_items = m_children_items;
    for (Item* child : children_items) {
        if (child) {
            child->check_resized();
        }
    }
}

} // namespace Slic3r::App::Yoga
