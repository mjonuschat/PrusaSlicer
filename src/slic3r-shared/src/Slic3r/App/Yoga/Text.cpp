///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/Yoga/Text.hpp"

#include "Slic3r/App/Render/ImguiRender.hpp"

#include <cmath>

namespace Slic3r::App::Yoga {

Text::Text(const std::string& text, Render::ImguiFontType font_type) :
    Item(),
    m_text(text),
    m_font_type(font_type)
{}

void Text::render(Vec2f pos, Vec2f size)
{
    render_item_begin(pos, size);

    ImGui::SetCursorScreenPos(to_im(pos));

    ImGui::PushFont(m_imgui_render->font(m_font_type));
    ImGui::PushStyleColor(
        ImGuiCol_Text,
        enabled() ? ImVec4(m_text_color) : ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled)
    );

    // TODO: Resolve elipsis/elide
    if (m_wrap) {
        // When text wrapping is applied, the text gets truncated on the right due to its offset from the parent window.
        // To fix this, we need to increase wrapping_size by ImGui::GetCursorPos().x.
        // Note: This feels like a hack, but a similar use of PushTextWrapPos can be found in imgui_demo.cpp (line 1281).
        ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + width());
        ImGui::TextUnformatted(m_text.c_str());
        ImGui::PopTextWrapPos();
    } else {
        ImGui::TextUnformatted(m_text.c_str());
    }

    ImGui::PopStyleColor();
    ImGui::PopFont();

    render_item_end(pos, size);
}

const std::string& Text::text() const
{
    return m_text;
}

void Text::set_text(const std::string& text)
{
    if (m_text != text) {
        m_text = text;
        invalidate_min_size_calculation();
        set_style_dirty();
    }
}

Vec2f Text::get_item_size()
{
    if (m_wrap) {
        // While wrapping enforce ONLY Y axis, let as assume the width that is set by parent or
        // external sources
        if (std::isnan(width())) {
            return {0, ImGui::CalcTextSize(m_text.c_str()).y};
        } else {
            return {0, from_im(ImGui::CalcTextSize(m_text.c_str(), nullptr, false, width())).y()};
        }
    } else {
        return from_im(ImGui::CalcTextSize(m_text.c_str()));
    }
}

void Text::on_resized()
{
    if (m_wrap) {
        set_min_size(get_item_size());
    }
}

bool Text::wrap() const
{
    return m_wrap;
}

void Text::set_wrap(bool wrap)
{
    if (m_wrap != wrap) {
        m_wrap = wrap;
        if (m_min_size_calculated) {
            set_min_size(get_item_size());
        }
    }
}

const ImColor& Text::text_color() const
{
    return m_text_color;
}

void Text::set_text_color(const ImColor& text_color)
{
    m_text_color = text_color;
}

Render::ImguiFontType Text::font_type() const
{
    return m_font_type;
}

void Text::set_font_type(Render::ImguiFontType font_type)
{
    m_font_type = font_type;
}

} // namespace Slic3r::App::Yoga
