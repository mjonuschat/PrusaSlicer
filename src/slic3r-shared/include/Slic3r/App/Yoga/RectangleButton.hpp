///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/Yoga/AbstractButton.hpp"

namespace Slic3r::App::Yoga {

class Rectangle;

class RectangleButton : public AbstractButton
{
public:
    enum class Align
    {
        Left,
        Center,
        Right
    };

    explicit RectangleButton(const std::string& tooltip = {});

    virtual void append(ObjectPtr child) override;
    virtual void insert(ObjectPtr child, size_t index) override;
    virtual ObjectPtr remove(Object* child) override;

    const ImColor& background_color() const;
    void set_background_color(const ImColor& color);

    const ImColor& background_color_checked() const;
    void set_background_color_checked(const ImColor& background_color_checked);

    const Paddings& content_padding();
    void set_content_padding(const Paddings& padding);

    Orientation content_orientation() const;
    void set_content_orientation(Orientation orientation);

    YGJustify content_justify_content() const;
    void set_content_justify_content(YGJustify justify);

    YGAlign content_align_item() const;
    void set_content_align_items(YGAlign align);

    YGDirection content_direction() const;
    void set_content_direction(YGDirection direction);

    float rounding() const;
    void set_rounding(float rounding);

    ImDrawFlags draw_flags() const;
    void set_draw_flags(ImDrawFlags draw_flags);

protected:
    void checked_updated_internal() override;
    void hovered_updated_internal() override;

private:
    void update_fill();

private:
    Rectangle* m_background = nullptr;

    ImColor m_background_color               = IM_COL32_WHITE;
    ImColor m_background_color_hover         = IM_COL32_WHITE;
    ImColor m_background_color_checked       = IM_COL32_WHITE;
    ImColor m_background_color_checked_hover = IM_COL32_WHITE;
    ImColor m_background_color_checked_disabled = IM_COL32_WHITE;
};

} // namespace Slic3r::App::Yoga
