///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/Yoga/AbstractButton.hpp"
#include "Slic3r/App/Render/ImguiTypes.hpp"

namespace Slic3r::App::Yoga {

class Rectangle;

class RectangleButton : public AbstractButton {
public:
    enum class Align {
        Left,
        Center,
        Right
    };

    explicit RectangleButton(const std::string& tooltip = {});

    virtual void append(ItemPtr child) override;
    virtual void insert(ItemPtr child, size_t index) override;
    virtual ItemPtr remove(Item* child) override;

    const ImColor& background_color() const;
    void set_background_color(const ImColor& color);

    const ImColor& background_color_checked() const;
    void set_background_color_checked(const ImColor& background_color_checked);

    const Paddings& content_padding();
    void set_content_padding(const Paddings& padding);

    void align_content(Align align);

protected:
    void checked_updated_internal() override;
    void hovered_updated_internal() override;

private:
    void update_fill();

private:
    Rectangle* m_background = nullptr;

    ImColor m_background_color = IM_COL32_WHITE;
    ImColor m_background_color_hover = IM_COL32_WHITE;
    ImColor m_background_color_checked = IM_COL32_WHITE;
    ImColor m_background_color_checked_hover = IM_COL32_WHITE;
};

} //namespace Slic3r::App::Yoga 