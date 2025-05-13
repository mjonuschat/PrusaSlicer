///|/ Copyright (c) Prusa Research 2018 - 2025 Oleksandra Iushchenko @YuSanka, Nikita Vanku @Zaraka
///|/
///|/ PrusaSlicer is released under the terms of the AGPLv3 or higher
///|/
#include "Slic3r/App/Yoga/ToolbarButton.hpp"

#include "Slic3r/App/Yoga/Tooltip.hpp"
#include "Slic3r/App/Imgui/ImguiExtension.hpp"
#include "Slic3r/App/Yoga/Toolbar.hpp"
#include "Slic3r/Assert.hpp"
#include "Slic3r/Log.hpp"

#include <iostream>

namespace Slic3r::App::Yoga {

ToolbarButton::ToolbarButton(wchar_t icon, const std::string& tooltip, Toolbar* parent)
    : AbstractButton(icon, tooltip, nullptr)
{
    if (parent) {
        parent->append(this);
    }
}

void ToolbarButton::render(Vec2f pos, Vec2f size)
{
    render_item_begin(pos, size);

    float rounding = GImGui->Style.WindowRounding; // { 4.f };
    ImVec2 button_size = to_im(size);
    ImRect button_bb(to_im(pos), to_im(pos) + button_size);

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    const float arrow_h = draw_list->_Data->FontSize * 0.5f;

    ImVec2 arrow_size = ImVec2(1.f, 1.f) * 2.5f * arrow_h;
    ImVec2 arrow_pos = to_im(pos) + button_size - arrow_size;
    ImRect arrow_bb(arrow_pos, arrow_pos + arrow_size);

    // Draw button background with custom rounding corner(s)
    draw_list->AddRectFilled(
        button_bb.Min, button_bb.Max, ImGui::GetColorU32(ImGuiCol_WindowBg), rounding,
        ImDrawFlags_RoundCornersAll
        // TODO: resolve rounding rounding_corners
    );
    button_bb.Expand(-rounding);
    if (m_enabled) {
        ImU32 col = ImGui::GetColorU32(
            m_hovered ? (m_checked ? ImGuiCol_Button : ImGuiCol_ButtonHovered)
                      : (m_checked ? ImGuiCol_ButtonHovered : ImGuiCol_Button)
        );
        draw_list
            ->AddRectFilled(button_bb.Min, button_bb.Max, col, rounding, ImDrawFlags_RoundCornersAll);
    }

    // Render icon in the center of the button
    ImGui::SetCursorScreenPos(button_bb.Min);
    Imgui::icon_image(m_icon, button_bb.GetSize(), !m_enabled);

    if (m_has_arrow) {
        // Draw button background with custom rounding on only one corner
        ImU32 arrow_col = ImGui::GetColorU32(ImGuiCol_Text);

        // draw arrow
        ImVec2 corner_pos = arrow_bb.GetCenter() + ImVec2(1.f, 1.f) * 0.5f * arrow_h;
        draw_list->AddTriangleFilled(
            corner_pos + ImVec2(0.f, -arrow_h), corner_pos, corner_pos + ImVec2(-arrow_h, 0.f),
            arrow_col
        );
    }

    render_item_end(pos, size);
}

void ToolbarButton::style_node()
{
    constexpr float gap = 10;

    if (m_subtoolbar) {
        ASSERT(m_parent);
        if (m_parent->orientation() == Yoga::Orientation::Vertical) {
            const float our_height = height();
            const float toolbar_height = m_subtoolbar->height();

            const float top = our_height * 0.5 - toolbar_height * 0.5;

            m_subtoolbar->set_right(-width() - gap);
            m_subtoolbar->set_top(top);
        } else {
            const float our_width = width();
            const float toolbar_width = m_subtoolbar->width();

            const float left = our_width * 0.5 - toolbar_width * 0.5;

            m_subtoolbar->set_left(left);
            m_subtoolbar->set_top(height() + gap);
        }
    }

    Toolbar* parent_toolbar = dynamic_cast<Toolbar*>(m_parent);
    m_tooltip->set_visible(parent_toolbar && parent_toolbar->show_tooltips());

    AbstractButton::style_node();
}

void ToolbarButton::set_subtoolbar_buttons(const std::vector<ToolbarButton*>& buttons)
{
    if (buttons.empty()) {
        if (m_subtoolbar) {
            m_subtoolbar->clear();
            remove(m_subtoolbar);
            delete m_subtoolbar;
            m_subtoolbar = nullptr;
        }
    } else {
        if (!m_subtoolbar) {
            m_subtoolbar = new Toolbar("subtoolbar", this);
            m_subtoolbar->set_orientation(Orientation::Vertical);
            m_subtoolbar->set_position_type(YGPositionType::YGPositionTypeAbsolute);
            m_subtoolbar->set_button_aspect_ratio(m_aspect_ratio);
            m_subtoolbar->set_button_min_size(m_min_size);
            m_subtoolbar->set_button_max_size(m_max_size);
            m_subtoolbar->set_visible(false);

            m_callbacks.action = [this] {
                if (m_subtoolbar) {
                    m_subtoolbar->set_visible(!m_subtoolbar->is_visible());

                    Toolbar* parent_toolbar = dynamic_cast<Toolbar*>(m_parent);
                    if (parent_toolbar && parent_toolbar->callbacks().subtoolbar_opened) {
                        parent_toolbar->callbacks().subtoolbar_opened();
                    }
                }
            };
        }
        m_subtoolbar->set(buttons);
    }
}

Toolbar* ToolbarButton::subtoolbar() const { return m_subtoolbar; }

} // namespace Slic3r::App::Yoga
