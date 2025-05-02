#include "Slic3r/App/Yoga/Text.hpp"

#include "Slic3r/App/Render/ImguiRender.hpp"

namespace Slic3r::App::Yoga {

Text::Text(const std::string& text, Item* parent) : Item(parent), m_text(text) {}

void Text::render(Vec2f pos, Vec2f size)
{
    if (!m_parent) {
        style_node();
        resize(size);
    }

    ImGui::SetCursorScreenPos(to_im(pos));

    ImGui::PushFont(m_imgui_render->font(m_font_type));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(m_text_color));

    ImGui::TextUnformatted(m_text.c_str());

    ImGui::PopStyleColor();
    ImGui::PopFont();

    render_internal(pos, size);
}

const std::string& Text::text() const { return m_text; }

void Text::set_text(const std::string& text) { m_text = text; }

Vec2f Text::get_item_size() { return from_im(ImGui::CalcTextSize(m_text.c_str())); }

const ImColor& Text::text_color() const { return m_text_color; }

void Text::set_text_color(const ImColor& text_color) { m_text_color = text_color; }

Render::ImguiFontType Text::font_type() const { return m_font_type; }

void Text::set_font_type(Render::ImguiFontType font_type) { m_font_type = font_type; }

} // namespace Slic3r::App::Yoga
