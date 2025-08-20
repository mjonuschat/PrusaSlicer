///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#pragma once

#include "Slic3r/App/Yoga/RectangleButton.hpp"
#include "Slic3r/App/Render/ImguiTypes.hpp"

namespace Slic3r::App::Yoga {

class Icon;
class Text;

class LayoutButton : public RectangleButton
{
public:
    LayoutButton(const std::string& label);
    LayoutButton(const std::string& label, Render::Icon icon);
    LayoutButton(const std::string& label, Render::Icon icon, const std::string& tooltip);

    const std::string& label() const;
    void set_label(const std::string& label);

    Render::ImguiFontType label_font_type() const;
    void set_label_font_type(Render::ImguiFontType label_font_type);
    void set_expand_label(bool expand);

    Render::Icon icon() const;
    void set_icon(Render::Icon icon);

private:
    Icon* m_icon = nullptr;
    Text* m_text = nullptr;
};

} // namespace Slic3r::App::Yoga
