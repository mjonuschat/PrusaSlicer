#include "Slic3r/App/Plater/MMPaintingColorDropdowns.hpp"

#include "Slic3r/App/Yoga/Circle.hpp"
#include "Slic3r/App/Yoga/ImGuiUtils.hpp"

#include <imgui_internal.h>

#include <algorithm>

namespace Slic3r::App::Plater {

ColorMenuItem::ColorMenuItem(
    const std::string& label,
    const Domain::ColorRGBA& color,
    bool selectable,
    bool dropdown_indicator
)
    : m_dropdown_indicator(dropdown_indicator)
{
    set_min_size({0, 24_px});
    set_width_percent(100);
    set_content_padding(m_dropdown_indicator ? Yoga::Paddings{8_px, 3_px, 24_px, 3_px} :
                                              Yoga::Paddings{8_px, 3_px});
    set_content_align_items(YGAlignCenter);
    set_content_justify_content(YGJustifyFlexStart);
    set_background_color(Platform::Color::ButtonTransparent);
    if (selectable) {
        set_background_color_checked(Platform::Color::Button);
        set_checkable(true);
    }

    m_swatch = emplace_back<Yoga::Circle>();
    m_swatch->set_width(12_px);
    m_swatch->set_height(12_px);

    auto* spacer = emplace_back<Yoga::Item>();
    spacer->set_flex_grow(1);

    set_entry(label, color);
}

void ColorMenuItem::set_entry(const std::string& label, const Domain::ColorRGBA& color)
{
    m_label = label;
    const ImColor imgui_color{color.r_uchar(), color.g_uchar(), color.b_uchar(), color.a_uchar()};
    m_swatch->set_fill(imgui_color);
}

void ColorMenuItem::render(Yoga::Vec2f pos, Yoga::Vec2f size)
{
    Yoga::RectangleButton::render(pos, size);

    const ImRect bb{to_im(pos), to_im(pos + size)};

    const ImVec2 swatch_pos{to_im(m_swatch->get_global_pos())};
    const float swatch_right{swatch_pos.x + m_swatch->width()};

    const float text_left{swatch_right + 6_px};
    const float right_padding{4_px};
    float text_right{bb.Max.x - right_padding};

    if (!m_dropdown_indicator) {
        ImRect text_rect{ImVec2(text_left, bb.Min.y), ImVec2(text_right, bb.Max.y)};
        ImGui::RenderTextClipped(
            text_rect.Min,
            text_rect.Max,
            m_label.c_str(),
            nullptr,
            nullptr,
            ImVec2(0.f, 0.5f),
            &text_rect
        );
        return;
    }

    ImDrawList* draw_list{ImGui::GetWindowDrawList()};
    if (draw_list == nullptr) {
        return;
    }

    const ImGuiStyle& style{GImGui->Style};
    const float arrow_size{ImGui::GetFrameHeight()};
    const float value_x2{ImMax(bb.Min.x, bb.Max.x - arrow_size)};

    text_right = value_x2 - style.FramePadding.x;
    ImRect text_rect{ImVec2(text_left, bb.Min.y), ImVec2(text_right, bb.Max.y)};
    ImGui::RenderTextClipped(
        text_rect.Min,
        text_rect.Max,
        m_label.c_str(),
        nullptr,
        nullptr,
        ImVec2(0.f, 0.5f),
        &text_rect
    );

    const ImU32 text_col{ImGui::GetColorU32(hovered() ? ImGuiCol_Text : ImGuiCol_TextDisabled)};
    if (value_x2 + arrow_size - style.FramePadding.x <= bb.Max.x) {
        const float w{arrow_size - 2.f * style.FramePadding.x};
        const float h{GImGui->FontSize};
        Yoga::YGRenderArrow(
            draw_list,
            ImVec2(value_x2 + style.FramePadding.x, bb.Min.y + style.FramePadding.y),
            ImVec2(w, h),
            text_col,
            ImGuiDir_Down,
            1.0f
        );
    }
}

ColorDropdown::ColorDropdown(Render::Icon icon, const ImColor& color, float spacing)
{
    set_gap(spacing);
    set_align_items(YGAlignCenter);
    emplace_icon(this, icon, {16_px, 16_px}, color);

    m_trigger = emplace_back<ColorMenuItem>(std::string{}, Domain::ColorRGBA{}, true, true);
    m_trigger->set_flex_grow(1);
    m_trigger->set_background_color(Platform::Color::Button);
    m_trigger->set_background_color_checked(
        m_theme->color_imgui(Platform::Color::Button, Platform::ColorGroup::Hovered)
    );

    m_popup = m_trigger->emplace_back<Yoga::ContextPopup>("MMPaintingColorDropdownPopup");
    m_popup->set_orientation(Yoga::Orientation::Vertical);
    m_popup->set_width_percent(100);
    m_popup->set_padding({2_px, 2_px});
    m_popup->set_gap(2_px);
    m_popup->set_offset(2_px);
    m_popup->set_position(Yoga::Position::Bottom);

    m_trigger->callbacks().action = [this]()
    {
        if (m_popup->opened()) {
            m_popup->close();
        } else {
            m_popup->open();
        }
    };

    m_popup->callbacks().opened = [this]() { m_trigger->set_checked(true); };
    m_popup->callbacks().closed = [this]() { m_trigger->set_checked(false); };
}

void
ColorDropdown::set_items(const std::vector<std::string>& names, const std::vector<Domain::ColorRGBA>& colors)
{
    ASSERT(!names.empty() && !colors.empty());

    const std::size_t count{std::min(names.size(), colors.size())};
    ASSERT(count > 0);

    m_names.assign(names.begin(), names.begin() + count);
    m_colors.assign(colors.begin(), colors.begin() + count);

    if (m_current_index >= count) {
        m_current_index = 0;
    }

    rebuild_popup_items();
    update_trigger_label();
}

void ColorDropdown::set_current_index(std::size_t index)
{
    if (m_names.empty()) {
        return;
    }

    const std::size_t clamped_index{std::min(index, m_names.size() - 1)};
    if (m_current_index == clamped_index) {
        return;
    }

    m_current_index = clamped_index;
    update_trigger_label();

    for (std::size_t i{}; i < m_popup_items.size(); ++i) {
        m_popup_items[i]->set_checked(i == m_current_index);
    }
}

std::size_t ColorDropdown::current_index() const
{
    return m_current_index;
}

void ColorDropdown::style_node()
{
    if (m_popup != nullptr && m_trigger != nullptr) {
        m_popup->set_width(m_trigger->width());
    }

    Yoga::Item::style_node();
}

void ColorDropdown::rebuild_popup_items()
{
    while (!m_popup->items().empty()) {
        m_popup->remove(m_popup->items().back());
    }

    m_popup_items.clear();

    for (std::size_t index{}; index < m_names.size(); ++index) {
        ColorMenuItem* item{
            m_popup->emplace_back<ColorMenuItem>(m_names[index], m_colors[index], true, false)
        };
        item->set_checked(index == m_current_index);
        m_popup_items.push_back(item);
        item->callbacks().action = [this, index]()
        {
            set_current_index(index);
            on_color_selected(index);
            m_popup->close();
        };
    }
}

void ColorDropdown::update_trigger_label()
{
    if (m_names.empty()) {
        m_trigger->set_entry({}, Domain::ColorRGBA{});
        return;
    }

    m_trigger->set_entry(m_names[m_current_index], m_colors[m_current_index]);
}

} // namespace Slic3r::App::Plater
