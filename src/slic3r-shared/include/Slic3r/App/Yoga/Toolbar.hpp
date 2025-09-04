///|/ Copyright (c) Prusa Research 2018 - 2025 Oleksandra Iushchenko @YuSanka, Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/Yoga/Window.hpp"

#include <vector>

namespace Slic3r::App::Yoga {

class ToolbarButton;

class Toolbar : public Window
{
public:

    struct Callbacks {
        std::function<void()> hovered_changed{nullptr};
        std::function<void()> subtoolbar_opened{nullptr};
    };

    explicit Toolbar(const std::string& name);

    Callbacks& callbacks();

    void render_body(Vec2f pos, Vec2f size) override;

    void append(std::unique_ptr<ToolbarButton> button);
    std::unique_ptr<ToolbarButton> remove(ToolbarButton* button);

    ToolbarButton* button_at(int index) const;
    int button_count() const;
    std::optional<size_t> index_of(ToolbarButton* button) const;
    bool contains(ToolbarButton* button) const;

    const Vec2f& button_min_size() const;
    void set_button_min_size(const Vec2f& button_min_size);

    const Vec2f& button_max_size() const;
    void set_button_max_size(const Vec2f& button_max_size);

    float button_aspect_ratio() const;
    void set_button_aspect_ratio(float button_aspect_ratio);

    float available_size() const;
    void set_available_size(float available_size);

    bool collapsible() const;
    void set_collapsible(bool collapsible);

    void style_node() override;

    bool show_tooltips() const;
    void set_show_tooltips(bool show_tooltips);

    bool hovered() const;
    bool any_subtoolbar_opened() const;

private:
    // Hide these methods
    void append(ItemPtr child) override;
    void insert(ItemPtr child, size_t index) override;
    ItemPtr remove(Item* child) override;

private:
    Callbacks m_callbacks;

    std::vector<ToolbarButton*> m_buttons;
    ToolbarButton* m_button_more = nullptr;
    Vec2f m_button_min_size;
    Vec2f m_button_max_size = {YGUndefined, YGUndefined};
    float m_button_aspect_ratio = 1.f;
    float m_available_size = YGUndefined;
    bool m_collapsible = false;
    bool m_show_tooltips = false;
    bool m_hovered = false;
};

} // namespace Slic3r::App::Yoga
