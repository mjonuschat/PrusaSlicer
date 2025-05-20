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
    enum class Align {
        Left,
        Center,
        Right
    };

    LayoutButton(const std::string& label);
    LayoutButton(const std::string& label, wchar_t icon);
    LayoutButton(const std::string& label, wchar_t icon, const std::string& tooltip);

    const std::string& label() const;
    void set_label(const std::string& label);

    const ImColor& background_color() const;
    void set_background_color(const ImColor& color);

    Render::ImguiFontType label_font_type() const;
    void set_label_font_type(Render::ImguiFontType label_font_type);

    const Paddings& content_padding();
    void set_content_padding(const Paddings& padding);

    void set_icon(wchar_t icon);
    void set_icon_size(Vec2f size); // for discussion
    void align_content(Align align);
    void expand_label(bool expand);

protected:
    Item* insert_into_content(std::unique_ptr<Item> item, std::optional<size_t> index = {});
    void checked_updated_internal() override;
    void hovered_updated_internal() override;

private:
    void update_fill();

private:
    Rectangle* m_background = nullptr;
    Icon* m_icon = nullptr;
    Text* m_text = nullptr;

    ImColor m_background_color = IM_COL32_WHITE;
    ImColor m_background_color_hover = IM_COL32_WHITE;
    ImColor m_background_color_checked = IM_COL32_WHITE;
    ImColor m_background_color_checked_hover = IM_COL32_WHITE;
};

} // namespace Slic3r::App::Yoga
