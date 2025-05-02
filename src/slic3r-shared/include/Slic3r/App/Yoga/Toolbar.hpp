///|/ Copyright (c) Prusa Research 2018 - 2025 Oleksandra Iushchenko @YuSanka, Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/Yoga/Window.hpp"

#include <vector>

namespace Slic3r::App::Yoga {

class IconButton;

class Toolbar : public Window
{
public:
    explicit Toolbar(const std::string& name, Item* parent = nullptr);

    void append(IconButton* button);
    void remove(IconButton* button);
    IconButton* button_at(int index) const;
    int button_count() const;
    int index_of(IconButton* button) const;
    bool contains(IconButton* button) const;

    const Vec2f& button_min_size() const;
    void set_button_min_size(const Vec2f& button_min_size);

    const Vec2f& button_max_size() const;
    void set_button_max_size(const Vec2f& button_max_size);

    float button_aspect_ratio() const;
    void set_button_aspect_ratio(float button_aspect_ratio);

    // Todo: collapsible
    // Todo: sub-toolbar

private:
    // Hide these methods
    void append(Item* child) override;
    void insert(Item* child, size_t index) override;
    void remove(Item* child) override;

private:
    std::vector<IconButton*> m_buttons;
    Vec2f m_button_min_size;
    Vec2f m_button_max_size = {YGUndefined, YGUndefined};
    float m_button_aspect_ratio = 1.f;
};

} // namespace Slic3r::App::Yoga
