///|/ Copyright (c) Prusa Research 2018 - 2025 Oleksandra Iushchenko @YuSanka, Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "yoga/Yoga.h"
#include "imgui.h"

#include "Slic3r/App/Yoga/Namespace.hpp"
#include "Slic3r/Assert.hpp"
#include "Slic3r/Biz/IListObserver.hpp"
#include "Slic3r/App/Theme.hpp"

#include <memory>
#include <optional>

namespace Slic3r::App {
class Theme;
}

namespace Slic3r::App::Render {
class ImguiRender;
class Texture;
using TexturePtr = std::shared_ptr<Texture>;
} // namespace Slic3r::App::Render

namespace Slic3r::App::Yoga {

class Object;
using ObjectPtr = std::unique_ptr<Object>;
class Item;
using ItemPtr         = std::unique_ptr<Item>;
using ObjectHeartBeat = std::weak_ptr<int>;

class Event;
class Popup;
class RootItem;

/**
 * @brief The Object class is an Object in a tree
 */
class Object
{
public:
    Object();
    virtual ~Object();
    Object(const Object& rhs)      = delete;
    Object& operator=(Object& rhs) = delete;

    const std::string& object_name() const;
    void set_object_name(const std::string& object_name);

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
     * @warning will request scene redraw
     */
    virtual void set_style_dirty();

    Object* parent() const;

    template <class T, class... Args>
    T* emplace_back(Args&&... args)
    {
        std::unique_ptr<T> item = std::make_unique<T>(std::forward<Args>(args)...);
        T* item_raw             = item.get();
        append(std::move(item));
        return item_raw;
    }

    template <class T, class... Args>
    T* emplace(size_t index, Args&&... args)
    {
        std::unique_ptr<T> item = std::make_unique<T>(std::forward<Args>(args)...);
        T* item_raw             = item.get();
        insert(std::move(item), index);
        return item_raw;
    }

    std::vector<Object*> objects() const;

    virtual void prepend(ObjectPtr child);
    virtual void append(ObjectPtr child);
    virtual void insert(ObjectPtr child, size_t index);

    /**
     * @warning Immediate remove Item from tree, for deffered use remove_later
     * @param child to remove
     */
    virtual ObjectPtr remove(Object* child);
    /**
     * @brief remove_later removes item only after render traversal
     * @param child to remove
     */
    void remove_later(Object* child);
    /**
     * @brief move_later moves this item only after render traversal
     * @param target to move this item
     * @param index to move this item
     */
    void move_later(Object* target, size_t index);
    size_t object_count() const;
    Object* get_object(size_t index) const;
    std::optional<size_t> index_of(Object* item) const;

    virtual void push_event(std::unique_ptr<Event> event);

    /**
     * @warning ignore this and do not use
     */
    ObjectHeartBeat heartbeat() const;

    static void set_theme(Theme* theme);

    static void set_scale_factor(float scale_factor);
    static float scale_factor();
    static float pixel_round(float value);
    static Vec2f pixel_round(const Vec2f& value);
    static ImVec2 pixel_round(const ImVec2& value);

private:
    /**
     * @note intentionally private, please use append/prepend/insert
     */
    void add_child(ObjectPtr child, size_t index);
    /**
     * @note intentionally private, please use remove/remove_later
     */
    ObjectPtr remove_child(Object* child);

protected:
    Object* root_item() const;

    virtual void root_item_about_to_update();
    virtual void root_item_updated();

protected:
    /**
     * @note Stored here only for convenience, it is assumed that Derived classes
     * do not modify children directly
     * @todo move to method
     */
    std::vector<ObjectPtr> m_children;

    static Theme* m_theme;
    static YGConfigRef m_config;

private:
    static std::unordered_map<std::string, int> m_object_names;

    // Nikita: I'M SORRY!! Due to my poor hastened decisions I literally got into corner and this is
    // the only way right now how to actually be sure if this damn object is still alive.
    // This will be rewritten once Dialogs are properly implemented!
    //
    // There should be a registered C++ offender list and I should be on it.
    Biz::UnsharedPointer<int> m_heartbeat;

    Object* m_parent = nullptr;
    Object* m_root   = this;

    std::string m_object_name;
};

/**
 * @brief The Item class is an Object Item in a tree that can be styled using Yoga
 */
class Item : public Object
{
public:
    struct Callbacks
    {
        std::function<void()> size_changed{
            nullptr
        }; ///< Do not use, you probably want to override on_resized instead!
    };

    Item();
    ~Item();

    Callbacks& item_callbacks();

    /**
     * @warning invisible Items (and their object) are not rendered
     */
    void render(Vec2f pos, Vec2f size) override;

    /**
     * @warning invisible Items (and their object) are not styled
     */
    void style_node() override;

    /**
     * @return Yoga node
     */
    YGNodeRef node() const;

    /**
     * @return true if this Item is contained in Yoga::Window
     * @warning recursive solution
     */
    virtual bool is_in_window() const;

    bool enabled() const;
    void set_enabled(bool enabled);

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
    bool is_self_visible() const;
    float flex_grow() const;
    float flex_shrink() const;
    YGDirection direction() const;
    float aspect_ratio() const;
    YGPositionType position_type() const;
    bool debug_border() const;
    YGJustify justify_content() const;
    YGAlign align_items() const;
    YGAlign align_content() const;
    const Margins& margin() const;
    const Paddings& padding() const;
    float gap() const;
    Orientation orientation() const;
    YGWrap flex_wrap() const;
    bool is_node_dirty() const;

    void set_self_align(YGAlign align);
    void set_margin(const Margins& margin);
    void set_padding(const Paddings& padding);
    void set_min_size(const Vec2f& min_size);
    void set_min_width(float min_width);
    void set_min_height(float min_height);
    void set_max_size(const Vec2f& max_size);
    void set_visible(bool visible);
    void set_flex_grow(float flex_grow);
    void set_flex_shrink(float flex_shrink);
    void set_direction(YGDirection direction);
    void set_aspect_ratio(float aspect_ratio);
    void set_position_type(YGPositionType position_type);
    void set_align_items(YGAlign align_items);
    void set_orientation(Orientation orientation);
    void set_gap(float gap);
    void set_justify_content(YGJustify justify_content);
    void set_align_content(YGAlign align_content);
    void set_width(float width);
    void set_height(float height);
    void set_width_percent(float width_percent);
    void set_height_percent(float height_percent);
    void set_left(float left);
    void set_right(float right);
    void set_top(float top);
    void set_bottom(float bottom);
    void set_flex(float flex);
    void set_flex_wrap(YGWrap wrap);
    /**
     * @note z layer only works between siblings
     */
    void set_z(float z);

    void insert(ObjectPtr child, size_t index) override;
    ObjectPtr remove(Object* child) override;

    /**
     * @brief remove_later removes item only after render traversal
     * @param child to remove
     */
    void remove_later(Item* child);
    /**
     * @brief move_later moves this item only after render traversal
     * @param target to move this item
     * @param index to move this item
     */
    void move_later(Item* target, size_t index);

    const std::vector<Item*>& items() const;
    Item* get_item(size_t index) const;

    static void set_imgui_render(Render::ImguiRender* imgui_render);

    void set_debug_border(bool show_debug_border);
    virtual std::string debug_dump_tree() const;

    Vec2f get_global_pos() const;

    void check_resized();

    /**
     * @warning could return null even if parented! (parent is just not Item)
     */
    Item* parent_item() const;

protected:
    static ImVec2 to_im(const Vec2f& val);
    static Vec2f from_im(const ImVec2& val);
    static bool is_node_visible(YGNodeRef node);

    virtual Vec2f get_item_size();

    virtual void enabled_updated_internal();
    virtual void visible_updated_internal();

    virtual Vec2f get_available_size() const;

    virtual void on_resized();

    void update_children_render_order();

    void render_item_begin(Vec2f pos, Vec2f size);
    void render_item_end(Vec2f pos, Vec2f size);
    void render_node(Vec2f pos, Item* child);
    void render_debug(Vec2f pos, Vec2f size);

    /**
     * @brief render_image - replacement for ImGui::Image
     */
    void render_image(
        Render::TexturePtr texture,
        const ImVec2& image_size,
        const ImVec2& uv0      = ImVec2(0, 0),
        const ImVec2& uv1      = ImVec2(1, 1),
        const ImVec4& bg_col   = ImVec4(0, 0, 0, 0),
        const ImVec4& tint_col = ImVec4(1, 1, 1, 1)
    );

    /**
     * Invalidates the initial calculation of the minimal item size, forcing it to be recalculated.
     *
     * @warning Use this function carefully and only when you're sure it's necessary.
     * It should be called only when the content has changed in a way that might affect the item's size.
     */
    void invalidate_min_size_calculation();

private:
    void update_enabled();
    void update_visible();

protected:
    // I will burn in hell for this
    static Render::ImguiRender* m_imgui_render;

    Callbacks m_callbacks;

    Vec2f m_min_size = {};
    Vec2f m_max_size = {YGUndefined, YGUndefined};
    YGNodeRef m_node = nullptr;

    bool m_min_size_calculated = false;

    YGAlign m_self_align        = YGAlign::YGAlignAuto;
    YGAlign m_align_items       = YGAlign::YGAlignStretch;
    YGAlign m_align_content     = YGAlign::YGAlignFlexStart;
    YGJustify m_justify_content = YGJustify::YGJustifyFlexStart;
    Margins m_margin;
    Paddings m_padding;
    float m_flex_grow              = 0;
    float m_flex_shrink            = 1;
    float m_aspect_ratio           = YGUndefined;
    float m_gap                    = 0;
    YGPositionType m_position_type = YGPositionType::YGPositionTypeRelative;
    bool m_debug_border            = false;
    float m_z                      = 0;
    float m_last_width             = 0;
    float m_last_height            = 0;

    Orientation m_orientation        = Orientation::Horizontal;
    YGFlexDirection m_flex_direction = YGFlexDirectionRow;
    YGDirection m_direction          = YGDirectionLTR;

    std::vector<Item*> m_children_render_order;
    std::vector<Item*> m_removed_children;

#ifdef DEBUG
    static Item* m_debug_item;
#endif

private:
    /**
     * @warning SPARSE field, objects which are not items are in this vector as well
     */
    std::vector<Item*> m_children_items;
    Item* m_parent_item = nullptr;
    bool m_has_items    = false;
    bool m_enabled      = true;
    bool m_visible      = true;
};

/**
 * The Passthrough class is a handy container for "passing through"
 * changing ownership of primarily of ItemPtr. It utilizes load and unload methods,
 * unloanding still keeps set m_raw pointer so even though the ownership
 * was already transfered a Passthrough instance still provides pointer
 * to the once owned instance.
 */
template <class T>
class Passthrough
{
public:
    Passthrough() {}

    Passthrough(std::unique_ptr<T>&& unique)
    {
        reset(std::move(unique));
    }

    T* operator->() const
    {
        return m_raw;
    }

    void reset(std::unique_ptr<T> unique)
    {
        m_raw    = unique.get();
        m_unique = std::move(unique);
    }

    std::unique_ptr<T> release()
    {
        ASSERT(m_unique, "releasing empty unique_ptr");
        return std::move(m_unique);
    }

    T* get() const
    {
        return m_raw;
    }

private:
    std::unique_ptr<T> m_unique;
    T* m_raw = nullptr;
};

template <class T>
std::unique_ptr<T> unique_dynamic_cast(ObjectPtr item)
{
    return std::unique_ptr<T>(dynamic_cast<T*>(item.release()));
}

template <class T>
std::unique_ptr<T> unique_dynamic_cast(ItemPtr item)
{
    return std::unique_ptr<T>(dynamic_cast<T*>(item.release()));
}

} // namespace Slic3r::App::Yoga
