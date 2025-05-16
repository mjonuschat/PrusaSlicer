///|/ Copyright (c) Prusa Research 2018 - 2025 Oleksandra Iushchenko @YuSanka, Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "yoga/Yoga.h"
#include "imgui.h"

#include "Slic3r/App/Yoga/Namespace.hpp"
#include "Slic3r/Assert.hpp"

#include <memory>
#include <optional>

namespace Slic3r::App::Render {
class ImguiRender;
}

namespace Slic3r::App::Yoga {

class Item;
using ItemPtr = std::unique_ptr<Item>;

class Item
{
public:
    explicit Item();
    virtual ~Item();
    Item(const Item& rhs) = delete;
    Item& operator=(Item& rhs) = delete;

    /**
     * @note Layout style cannot be changed inside render
     * @note You have to call render() of your children as well
     */
    virtual void render(Vec2f pos, Vec2f size);

    /**
     * @brief Layout aware & dynamic code should be put here
     * @note You have to call style_node() of your children as well
     */
    virtual void style_node();

    /**
     * @brief Yoga recalculate whole tree
     * @note should be called only top-level item
     */
    virtual void resize(Vec2f size);

    /**
     * @brief process_events processes input events and calls callbacks
     * @note You have to call process_events() of your children as well
     */
    virtual void process_events(Vec2f pos, Vec2f size);

    YGNodeRef node() const;

    Item* parent() const;
    /**
     * @return position that is relative to Item parent
     * @note resize has to be called for parent Item
     */
    float x() const;
    /**
     * @return position that is relative to Item parent
     * @note resize has to be called for parent Item
     */
    float y() const;
    /**
     * @note resize has to be called for parent Item
     */
    float width() const;
    /**
     * @note resize has to be called for parent Item
     */
    float height() const;
    /**
     * @note z layer only works between siblings
     */
    float z() const;
    float left() const;
    float right() const;
    float top() const;
    float bottom() const;
    const Vec2f& min_size() const;
    const Vec2f& max_size() const;
    bool is_visible() const;
    float flex_grow() const;
    YGDirection direction() const;
    float aspect_ratio() const;
    YGPositionType position_type() const;
    bool debug_border() const;
    YGJustify justify_content() const;
    YGAlign align_items() const;
    YGAlign align_content() const;
    const std::string& item_name() const;
    const Margins& margin() const;
    const Paddings& padding() const;
    float gap() const;
    Orientation orientation() const;
    bool is_dirty() const;

    bool enabled();
    void set_enabled(bool enabled);

    void set_self_align(YGAlign align);
    void set_margin(const Margins& margin);
    void set_padding(const Paddings& padding);
    void set_min_size(const Vec2f min_size);
    void set_max_size(const Vec2f max_size);
    void set_visible(bool visible);
    void set_flex_grow(float flex_grow);
    void set_direction(YGDirection direction);
    void set_aspect_ratio(float aspect_ratio);
    void set_position_type(YGPositionType position_type);
    void set_align_items(YGAlign align_items);
    void set_orientation(Orientation orientation);
    void set_gap(float gap);
    void set_justify_content(YGJustify justify_content);
    void set_align_content(YGAlign align_content);
    void set_item_name(const std::string& item_name);
    void set_width(float width);
    void set_height(float height);
    void set_width_percent(float width_percent);
    void set_height_percent(float height_percent);
    void set_left(float left);
    void set_right(float right);
    void set_top(float top);
    void set_bottom(float bottom);
    /**
     * @note z layer only works between siblings
     */
    void set_z(float z);

    virtual void prepend(ItemPtr child);
    virtual void append(ItemPtr child);
    virtual void insert(ItemPtr child, size_t index);

    template<class T, class... Args>
    T* emplace_back(Args&&... __args)
    {
        std::unique_ptr<T> item = std::make_unique<T>(std::forward<Args>(__args)...);
        T* item_raw = item.get();
        append(std::move(item));
        return item_raw;
    }

    virtual ItemPtr remove(Item* child);
    std::vector<Item*> items() const;
    size_t item_count() const;
    Item* get_item(size_t index) const;
    std::optional<size_t> index_of(Item* item) const;

    static void set_imgui_render(Render::ImguiRender* imgui_render);

    void set_debug_border(bool show_debug_border);
    virtual std::string debug_dump_tree() const;

protected:
    static ImVec2 to_im(const Vec2f& val);
    static Vec2f from_im(const ImVec2& val);
    static bool is_node_visible(YGNodeRef node);

    virtual Vec2f get_item_size();

    void add_child(ItemPtr child, size_t index);
    ItemPtr remove_child(Item* child);
    void update_children_render_order();

    void set_style_dirty();

    YGNodeRef get_node(size_t index);
    size_t get_node_count() const;
    ImVec2 get_node_pos() const;

    void render_item_begin(Vec2f pos, Vec2f size);
    void render_item_end(Vec2f pos, Vec2f size);
    void render_node(Vec2f pos, Item* child);
    void render_debug(Vec2f pos, Vec2f size);
    void process_events_node(Vec2f pos, Item* child);

protected:
    // I will burn in hell for this
    static Render::ImguiRender* m_imgui_render;

    Vec2f m_min_size = {};
    Vec2f m_max_size = {YGUndefined, YGUndefined};
    Item* m_parent = nullptr;
    YGNodeRef m_node = nullptr;
    std::string m_item_name;

    bool m_style_dirty = true;
    bool m_min_size_calculated = false;

    YGAlign m_self_align = YGAlign::YGAlignAuto;
    YGAlign m_align_items = YGAlign::YGAlignStretch;
    YGAlign m_align_content = YGAlign::YGAlignFlexStart;
    YGJustify m_justify_content = YGJustify::YGJustifyFlexStart;
    Margins m_margin;
    Paddings m_padding;
    float m_flex_grow = 0;
    float m_aspect_ratio = YGUndefined;
    float m_gap = 0;
    YGPositionType m_position_type = YGPositionType::YGPositionTypeRelative;
    bool m_debug_border = false;
    float m_z = 0;
    bool m_enabled = true;
    bool m_visible = true;

    Orientation m_orientation = Orientation::Horizontal;
    YGFlexDirection m_flex_direction = YGFlexDirectionRow;
    YGDirection m_direction = YGDirectionLTR;

    std::vector<ItemPtr> m_children;
    std::vector<Item*> m_children_render_order;
};

/**
 * The Passthrough class is a handy container for "passing through"
 * changing ownership of primarily of ItemPtr. It utilizes load and unload methods,
 * unloanding still keeps set m_raw pointer so even though the ownership
 * was already transfered a Passthrough instance still provides pointer
 * to the once owned instance.
 */
template<class T>
class Passthrough
{
public:
    Passthrough() {}
    Passthrough(std::unique_ptr<T>&& unique) { reset(std::move(unique)); }
    T* operator->() const { return m_raw; }

    void reset(std::unique_ptr<T> unique)
    {
        m_raw = unique.get();
        m_unique = std::move(unique);
    }
    std::unique_ptr<T> release()
    {
        ASSERT(m_unique, "releasing empty unique_ptr");
        return std::move(m_unique);
    }
    T* get() const { return m_raw; }

private:
    std::unique_ptr<T> m_unique;
    T* m_raw = nullptr;
};

template<class T>
std::unique_ptr<T> unique_dynamic_cast(ItemPtr item)
{
    return std::unique_ptr<T>(dynamic_cast<T*>(item.release()));
}

} // namespace Slic3r::App::Yoga
