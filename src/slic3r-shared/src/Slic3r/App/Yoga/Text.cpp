///|/ Copyright (c) Prusa Research 2025 Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/Yoga/Text.hpp"

#include "Slic3r/App/Render/ImguiRender.hpp"
#include "Slic3r/App/Platform/AbstractTheme.hpp"

#include <imgui_internal.h>

#include <cmath>

namespace Slic3r::App::Yoga {

class TextInternal : public Yoga::Item
{
public:
    TextInternal() : m_font_size(GImGui->FontSizeBase)
    {
        set_object_name("TextInternal");
    }

    void render(Vec2f pos, Vec2f size) override
    {
        render_item_begin(pos, size);

        ImGui::SetCursorScreenPos(to_im(pos));

        ImGui::PushFont(m_imgui_render->font(m_font_type), m_font_size);
        ImGui::PushStyleColor(
            ImGuiCol_Text,
            enabled() ? ImVec4(m_text_color) :
                        ImVec4(m_theme->color_imgui(
                            Platform::Color::Text,
                            Platform::ColorGroup::Disabled
                        ))
        );

        switch (m_wrap_mode) {
        case Text::WrapMode::Wrap:
        case Text::WrapMode::WrapElide:
            // When text wrapping is applied, the text gets truncated on the right due to its offset from the parent window.
            // To fix this, we need to increase wrapping_size by ImGui::GetCursorPos().x.
            // Note: This feels like a hack, but a similar use of PushTextWrapPos can be found in imgui_demo.cpp (line 1281).
            ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + width());
            ImGui::TextUnformatted(m_rendered_text.c_str());
            ImGui::PopTextWrapPos();
            break;
        case Text::WrapMode::NoWrap:
            ImGui::TextUnformatted(m_rendered_text.c_str());
            break;
        }

        ImGui::PopStyleColor();
        ImGui::PopFont();

        render_item_end(pos, size);
    }

    Text::WrapMode wrap_mode() const
    {
        return m_wrap_mode;
    }

    void set_wrap_mode(Text::WrapMode wrap_mode)
    {
        if (m_wrap_mode != wrap_mode) {
            m_wrap_mode = wrap_mode;
            invalidate_size();
        }
    }

    const ImColor& text_color() const
    {
        return m_text_color;
    }

    void set_text_color(const ImColor& text_color)
    {
        m_text_color = text_color;
    }

    Render::ImguiFontType font_type() const
    {
        return m_font_type;
    }

    void set_font_type(Render::ImguiFontType font_type)
    {
        if (m_font_type != font_type) {
            m_font_type = font_type;
            invalidate_size();
        }
    }

    float font_size() const
    {
        return m_font_size;
    }

    void set_font_size(float font_size)
    {
        if (!Domain::fuzzy_compare(m_font_size, font_size)) {
            m_font_size = font_size;
            invalidate_size();
        }
    }

    const std::string& source_text() const
    {
        return m_source_text;
    }

    void set_source_text(const std::string& source_text)
    {
        if (m_source_text != source_text) {
            m_source_text = source_text;
            invalidate_min_size_calculation();
            set_style_dirty();
        }
    }

    void invalidate_size()
    {
        invalidate_min_size_calculation();
        set_style_dirty();
    }

protected:
    float available_width() const
    {
        return parent_item()->width() - parent_item()->padding().horizontal();
    }

    float available_height() const
    {
        float avail_height = parent_item()->height() - parent_item()->padding().vertical();
        return Domain::fuzzy_compare(0.f, avail_height) ? ImGui::GetFontSize() : avail_height;
    }

    Vec2f get_item_size() override
    {
        Vec2f result;
        ImGui::PushFont(m_imgui_render->font(m_font_type), m_font_size);
        if (m_wrap_mode == Text::WrapMode::Wrap) {
            m_rendered_text = m_source_text;

            // While wrapping enforce ONLY Y axis, let as assume the width that is set by parent or
            // external sources
            if (std::isnan(width())) {
                result = Vec2f{0, ImGui::CalcTextSize(m_source_text.c_str()).y};
            } else {
                const float target_width = available_width();
                if (!std::isnan(target_width) && target_width > 0) {
                    result = Vec2f{
                        target_width,
                        from_im(
                            ImGui::CalcTextSize(m_source_text.c_str(), nullptr, false, target_width)
                        )
                            .y()
                    };
                }
            }
        } else if (m_wrap_mode == Text::WrapMode::WrapElide) {
            ImVec2 target_size{available_width(), available_height()};
            if (!std::isnan(target_size.x)
                && !std::isnan(target_size.y)
                && target_size.x > 0
                && target_size.y > 0)
            {
                target_size.y = std::max(m_font_size, target_size.y);

                std::string_view elided_text = m_source_text;
                m_rendered_text              = elided_text;
                ImVec2 current_size          = ImGui::CalcTextSize(
                    elided_text.data(),
                    elided_text.data() + elided_text.size(),
                    false,
                    target_size.x
                );
                while (current_size.y > target_size.y && !elided_text.empty()) {
                    if (std::isspace(elided_text.back())) {
                        while (std::isspace(elided_text.back())) {
                            elided_text.remove_suffix(1);
                        }
                    } else {
                        elided_text.remove_suffix(1);
                    }
                    m_rendered_text = std::string(elided_text) + "...";
                    current_size =
                        ImGui::CalcTextSize(m_rendered_text.c_str(), nullptr, false, target_size.x);
                }

                result = from_im(current_size);
            }
        } else {
            m_rendered_text = m_source_text;
            result          = from_im(ImGui::CalcTextSize(m_source_text.c_str()));
        }
        ImGui::PopFont();
        return result;
    }

    void on_resized() override
    {
        if (m_wrap_mode != Text::WrapMode::NoWrap) {
            set_min_size(get_item_size());
        }
    }

private:
    std::string m_source_text;
    std::string m_rendered_text;
    Text::WrapMode m_wrap_mode        = Text::WrapMode::NoWrap;
    ImColor m_text_color              = IM_COL32_WHITE;
    Render::ImguiFontType m_font_type = Render::ImguiFontType::Regular;
    float m_font_size                 = 0;
};

Text::Text(const std::string& text, Render::ImguiFontType font_type)
{
    set_object_name("Text");
    m_content_item = emplace_back<TextInternal>();
    m_content_item->set_font_type(font_type);
    m_content_item->set_source_text(text);
}

const std::string& Text::text() const
{
    return m_content_item->source_text();
}

void Text::set_text(const std::string& text)
{
    m_content_item->set_source_text(text);
}

const Align& Text::align() const
{
    return m_align;
}

void Text::set_align(const Align& align)
{
    if (m_align != align) {
        m_align = align;
        switch (m_align.horizontal) {
        case Slic3r::App::Yoga::AlignH::Left:
            set_justify_content(YGJustifyFlexStart);
            break;
        case Slic3r::App::Yoga::AlignH::Center:
            set_justify_content(YGJustifyCenter);
            break;
        case Slic3r::App::Yoga::AlignH::Right:
            set_justify_content(YGJustifyFlexEnd);
            break;
        }

        switch (m_align.vertical) {
        case Slic3r::App::Yoga::AlignV::Top:
            m_content_item->set_self_align(YGAlignFlexStart);
            break;
        case Slic3r::App::Yoga::AlignV::Center:
            m_content_item->set_self_align(YGAlignCenter);
            break;
        case Slic3r::App::Yoga::AlignV::Bottom:
            m_content_item->set_self_align(YGAlignFlexEnd);
            break;
        }
    }
}

void Text::on_resized()
{
    m_content_item->invalidate_size();
}

Text::WrapMode Text::wrap_mode() const
{
    return m_content_item->wrap_mode();
}

void Text::set_wrap_mode(WrapMode wrap_mode)
{
    m_content_item->set_wrap_mode(wrap_mode);
}

const ImColor& Text::text_color() const
{
    return m_content_item->text_color();
}

void Text::set_text_color(const ImColor& text_color)
{
    m_content_item->set_text_color(text_color);
}

Render::ImguiFontType Text::font_type() const
{
    return m_content_item->font_type();
}

void Text::set_font_type(Render::ImguiFontType font_type)
{
    m_content_item->set_font_type(font_type);
}

float Text::font_size() const
{
    return m_content_item->font_size();
}

void Text::set_font_size(float font_size)
{
    m_content_item->set_font_size(font_size);
}

} // namespace Slic3r::App::Yoga
