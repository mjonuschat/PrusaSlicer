///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/Yoga/Item.hpp"

namespace Slic3r::App::Yoga {

class Rectangle : public Item
{
public:
    explicit Rectangle();

    void render(Vec2f pos, Vec2f size) override;

    const ImColor& fill() const;
    std::optional<ImColor> disabled_fill() const;
    const ImColor& border_color() const;
    const ImColor& border_color_disabled() const;
    float border_width() const;
    float rounding() const;
    ImDrawFlags flags() const;

    void set_fill(const ImColor& fill);
    void set_disabled_fill(const ImColor& fill);
    void set_border_color(const ImColor& border_color);
    void set_border_color_disabled(const ImColor& border_color_disabled);
    void set_border_width(float border_width);
    void set_rounding(float rounding);
    void set_flags(ImDrawFlags flags);

protected:
    Vec2f get_item_size() override;

private:
    ImColor m_fill = IM_COL32_WHITE;
    std::optional<ImColor> m_disabled_fill;
    ImColor m_border_color = IM_COL32_WHITE;
    ImColor m_border_color_disabled = ImColor(95, 95, 95);
    float m_border_width = 0;
    float m_rounding = 5.f;
    ImDrawFlags m_flags = ImDrawFlags_RoundCornersAll;
};

} // namespace Slic3r::App::Yoga
