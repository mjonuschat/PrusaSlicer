#pragma once

#include "Slic3r/App/Yoga/Item.hpp"
#include "Slic3r/App/Render/ImguiTypes.hpp"

namespace Slic3r::App::Yoga {

class Text : public Item
{
public:
    Text(const std::string& text, Item* parent = nullptr);

    void render(Vec2f pos, Vec2f size) override;

    const std::string& text() const;
    void set_text(const std::string& text);

    const ImColor& text_color() const;
    void set_text_color(const ImColor& text_color);

    Render::ImguiFontType font_type() const;
    void set_font_type(Render::ImguiFontType font_type);

protected:
    Vec2f get_item_size() override;

private:
    std::string m_text;
    ImColor m_text_color = IM_COL32_WHITE;
    Render::ImguiFontType m_font_type = Render::ImguiFontType::Regular;
};

} // namespace Slic3r::App::Yoga
