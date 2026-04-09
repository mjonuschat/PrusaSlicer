///|/ Copyright (c) Prusa Research 2018 - 2025 Oleksandra Iushchenko @YuSanka, Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/Yoga/Window.hpp"

#include <vector>

namespace Slic3r::App {

class ToolBarButton;

class ToolBar : public Yoga::Window
{
public:
    explicit ToolBar(const std::string& name);

    std::unique_ptr<ToolBarButton> remove(ToolBarButton* button);

    ToolBarButton* button_at(int index) const;
    int button_count() const;
    std::optional<size_t> index_of(ToolBarButton* button) const;
    bool contains(ToolBarButton* button) const;

    float button_width() const;
    void set_button_width(float button_width);

    float button_height() const;
    void set_button_height(float button_height);

    float available_size() const;
    void set_available_size(float available_size);

    bool collapsible() const;
    void set_collapsible(bool collapsible);

    void style_node() override;

    bool hovered() const;

private:
    // Hide these methods
    void append(Yoga::ObjectPtr child) override;
    void insert(Yoga::ObjectPtr child, size_t index) override;
    Yoga::ObjectPtr remove(Object* child) override;

private:
    std::vector<ToolBarButton*> m_buttons;
    ToolBarButton* m_button_more = nullptr;
    float m_button_width         = 0;
    float m_button_height        = 0;
    float m_available_size       = YGUndefined;
    bool m_collapsible           = false;
    bool m_hovered               = false;
};

} // namespace Slic3r::App::Yoga
