#pragma once

#include "yoga/Yoga.h"
#include "imgui.h"

#include "Slic3r/App/Yoga/Namespace.hpp"

#include <limits>

namespace Slic3r::App::Render {
class ImguiRender;
}

namespace Slic3r::App::Yoga {

class Item
{
public:
    explicit Item(Item* parent = nullptr);
    virtual ~Item();
    Item(const Item& rhs) = delete;
    Item& operator=(Item& rhs) = delete;

    virtual void render(Vec2f pos, Vec2f size);

    virtual void style_node();

    virtual void resize(Vec2f size);

    YGNodeRef node() const;

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
    bool is_visible() const;
    float flex_grow() const;
    float aspect_ratio() const;
    YGPositionType position_type() const;
    bool debug_border() const;
    YGJustify justify_content() const;
    YGAlign align_items() const;
    YGAlign align_content() const;
    const std::string& item_name() const;

    bool enabled();
    void set_enabled(bool enabled);

    void set_parent(Item* parent, size_t index = std::numeric_limits<size_t>::max());
    void set_self_align(YGAlign align);
    void set_margin(const Margins& margin);
    void set_padding(const Paddings& padding);
    void set_min_size(const Vec2f min_size);
    void set_max_size(const Vec2f max_size);
    void set_visible(bool visible);
    void set_flex_grow(float flex_grow);
    void set_aspect_ratio(float aspect_ratio);
    void set_position_type(YGPositionType position_type);
    void set_align_items(YGAlign align_items);
    void set_orientation(Orientation orientation);
    void set_gap(float gap);
    void set_justify_content(YGJustify justify_content);
    void set_align_content(YGAlign align_content);
    void set_item_name(const std::string& item_name);

    virtual void prepend(Item* child);
    virtual void append(Item* child);
    virtual void insert(Item* child, size_t index);
    virtual void remove(Item* child);
    size_t item_count() const;
    Item* get_item(size_t index) const;

    static void set_imgui_render(Render::ImguiRender* imgui_render);

    void set_debug_border(bool show_debug_border);
    virtual std::string debug_dump_tree() const;

protected:
    static ImVec2 to_im(const Vec2f& val);
    static Vec2f from_im(const ImVec2& val);
    static bool is_node_visible(YGNodeRef node);

    virtual Vec2f get_item_size();

    void add_child(Item* child, size_t index);
    void remove_child(Item* child);

    void set_style_dirty();

    YGNodeRef get_node(size_t index);
    size_t get_node_count() const;
    ImVec2 get_node_pos() const;
    void render_internal(Vec2f pos, Vec2f size);
    void render_node(Vec2f pos, Item* child);
    void render_debug(Vec2f pos, Vec2f size);

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
    float m_z = 0; // Todo: Resolve Z-Layer
    bool m_enabled = true;

    Orientation m_orientation = Orientation::Horizontal;
    YGFlexDirection m_flex_direction = YGFlexDirectionRow;

    std::vector<Item*> m_children;
};

} // namespace Slic3r::App::Yoga
