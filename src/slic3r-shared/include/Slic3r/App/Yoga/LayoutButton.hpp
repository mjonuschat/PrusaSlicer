///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/Yoga/AbstractButton.hpp"
#include "Slic3r/App/Render/ImguiTypes.hpp"

namespace Slic3r::App::Yoga {

class Rectangle;
class Icon;
class Text;

class LayoutButton : public AbstractButton
{
public:
    LayoutButton(const std::string& label);
    LayoutButton(const std::string& label, wchar_t icon);
    LayoutButton(const std::string& label, wchar_t icon, const std::string& tooltip);

    void process_events(Vec2f pos, Vec2f size) override;

    const std::string& label() const;
    void set_label(const std::string& label);

    const ImColor& background_color() const;
    void set_background_color(const ImColor& color);

    Render::ImguiFontType label_font_type() const;
    void set_label_font_type(Render::ImguiFontType label_font_type);

    const Paddings& content_padding();
    void set_content_padding(const Paddings& padding);

private:
    Rectangle* m_background = nullptr;
    Icon* m_icon = nullptr;
    Text* m_text = nullptr;

    ImColor m_background_color = IM_COL32_WHITE;
    ImColor m_background_color_hover = IM_COL32_WHITE;
};

} // namespace Slic3r::App::Yoga
